/*
 *  drivers/sound/mp2520f-audio.c -- audio interface for the MP2520F 
 *  Copyright (C) 2004,2005 DIGNSYS Inc. (www.dignsys.com)
 *  Kane Ahn < hbahn@dignsys.com >
 *  hhsong < hhsong@dignsys.com >
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * 
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/pm.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/sysrq.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>

#include "mp2520f-audio.h"

//#define DS_DEBUG
#undef DS_DEBUG

#ifdef DS_DEBUG
#  define ds_printk(x...) printk(x)
#else
#  define ds_printk(fmt, args...)
#endif

#define AUDIO_NAME		"mp2520f-audio"
#define AUDIO_NBFRAGS_DEFAULT	16	// hhsong 050907 modify
#define AUDIO_FRAGSIZE_DEFAULT	32768

#define NEXT_BUF(_s_,_b_) { \
	(_s_)->_b_##_idx++; \
	(_s_)->_b_##_idx %= (_s_)->nbfrags; \
	(_s_)->_b_ = (_s_)->buffers + (_s_)->_b_##_idx; }

#define AUDIO_ACTIVE(state)	((state)->rd_ref || (state)->wr_ref)


static void audio_clear_buf(audio_stream_t * s)
{
	ds_printk("\n");

	/* ensure DMA won't run anymore */
	s->active = 0;
	s->stopped = 0;
	mmsp2_dma_flush_all(s->dma_ch);

	if (s->buffers) {
		#if 0 
		int frag;

		for (frag = 0; frag < s->nbfrags; frag++) {
			if (!s->buffers[frag].master)
				continue;
			consistent_free(s->buffers[frag].start,
					s->buffers[frag].master,
					s->buffers[frag].dma_addr);
		}
		#endif
		kfree(s->buffers);
		s->buffers = NULL;
	}

	s->buf_idx = 0;
	s->buf = NULL;
}

/*
 * This function allocates the buffer structure array and buffer data space
 * according to the current number of fragments and fragment size.
 */
int audio_setup_buf(audio_stream_t * s)
{
	int frag;
	int dmasize = 0;
	char *dmabuf = NULL;
	dma_addr_t dmaphys = 0;

	if (s->buffers)
		return -EBUSY;

	s->buffers = kmalloc(sizeof(audio_buf_t) * s->nbfrags, GFP_KERNEL);
	if (!s->buffers)
		goto err;
	memzero(s->buffers, sizeof(audio_buf_t) * s->nbfrags);

	for (frag = 0; frag < s->nbfrags; frag++) {
		audio_buf_t *b = &s->buffers[frag];

		b->start    = (char *)(VA_SOUND_DMA_BASE + frag*s->fragsize);
		b->dma_addr = (PA_SOUND_DMA_BASE + frag*s->fragsize);
		b->stream   = s;
		sema_init(&b->sem, 1);
		ds_printk("buf %d: start %p dma %p\n", frag, b->start, b->dma_addr);
	}

	s->buf_idx = 0;
	s->buf = &s->buffers[0];
	s->bytecount = 0;
	s->fragcount = 0;

	return 0;

err:
	printk(AUDIO_NAME ": unable to allocate audio memory\n ");
	audio_clear_buf(s);
	return -ENOMEM;
}


/*
 * This function yanks all buffers from the DMA code's control and
 * resets them ready to be used again.
 */

void audio_reset_buf(audio_stream_t * s)
{
	int frag;

	s->active = 0;
	s->stopped = 0;
	mmsp2_dma_flush_all(s->dma_ch);
	if (s->buffers) {
		for (frag = 0; frag < s->nbfrags; frag++) {
			audio_buf_t *b = &s->buffers[frag];
			b->size = 0;
			sema_init(&b->sem, 1);
		}
	}
	s->bytecount = 0;
	s->fragcount = 0;
}


/*
 * DMA callback functions
 */
static void audio_dmaout_done_callback(void *buf_id, int size)
{
	audio_buf_t *b = (audio_buf_t *) buf_id;
	audio_stream_t *s = b->stream;

	/* Accounting */
	s->bytecount += size;
	s->fragcount++;

	/* Recycle buffer */
	up(&b->sem);

	/* And any process polling on write. */
	wake_up(&s->wq);
}

static void audio_dmain_done_callback(void *buf_id, int size)
{
	audio_buf_t *b = (audio_buf_t *) buf_id;
	audio_stream_t *s = b->stream;

	/* Accounting */
	s->bytecount += size;
	s->fragcount++;

	/* Recycle buffer */
	b->size = size;
	up(&b->sem);

	/* And any process polling on write. */
	wake_up(&s->wq);
}

static int audio_sync(struct file *file)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *s = state->output_stream;
	audio_buf_t *b;

	ds_printk("\n");

	if (!(file->f_mode & FMODE_WRITE) || !s->buffers /*|| s->mapped*/)
		return 0;

	/*
	 * Send current buffer if it contains data.  Be sure to send
	 * a full sample count.
	 */
	b = s->buf;
	if (b->size &= ~3) {
		down(&b->sem);
		mmsp2_dma_queue_buffer(s->dma_ch, (void *) b,
					 b->dma_addr, b->size);
		b->size = 0;
		NEXT_BUF(s, buf);
	}

	/*
	 * Let's wait for the last buffer we sent i.e. the one before the
	 * current buf_idx.  When we acquire the semaphore, this means either:
	 * - DMA on the buffer completed or
	 * - the buffer was already free thus nothing else to sync.
	 */
	b = s->buffers + ((s->nbfrags + s->buf_idx - 1) % s->nbfrags);
	if (down_interruptible(&b->sem))
		return -EINTR;
	up(&b->sem);
	return 0;
}

static inline int copy_from_user_mono_stereo(char *to, const char *from, 
					     int count)
{
	u_int *dst = (u_int *) to;
	const char *end = from + count;

	if (verify_area(VERIFY_READ, from, count))
		return -EFAULT;

	if ((int) from & 0x2) {
		u_int v;

		__get_user(v, (const u_short *) from);
		from += 2;
		*dst++ = v | (v << 16);
	}

	while (from < end - 2) {
		u_int v, x, y;

		__get_user(v, (const u_int *) from);
		from += 4;
		x = v << 16;
		x |= x >> 16;
		y = v >> 16;
		y |= y << 16;
		*dst++ = x;
		*dst++ = y;
	}

	if (from < end) {
		u_int v;

		__get_user(v, (const u_short *) from);
		*dst = v | (v << 16);
	}

	return 0;
}

static int audio_write(struct file *file, const char *buffer, 
			size_t count, loff_t * ppos)
{
	const char *buffer0 = buffer;
	audio_state_t *state = (audio_state_t *) file->private_data;
	audio_stream_t *s = state->output_stream;
	int chunksize, ret = 0;

	ds_printk("count=%d\n", count);

	if (ppos != &file->f_pos)
		return -ESPIPE;
	if (!s->buffers && audio_setup_buf(s))
		return -ENOMEM;

	while (count > 0) {
		audio_buf_t *b = s->buf;

		/* Wait for a buffer to become free */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&b->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&b->sem))
				break;
		}

		/* Feed the current buffer */
		if (s->channels == 2) {
			chunksize = s->fragsize - b->size;

			if (chunksize > count)
				chunksize = count;
			ds_printk("write %d to %d\n", chunksize, s->buf_idx);
			if (copy_from_user(b->start + b->size, buffer, 
					   chunksize)) {
				up(&b->sem);
				return -EFAULT;
			}
			b->size += chunksize;
		} else {
			chunksize = (s->fragsize - b->size) >> 1;

			if (chunksize > count)
				chunksize = count;
			ds_printk("write %d to %d\n", chunksize * 2, s->buf_idx);
			if (copy_from_user_mono_stereo(b->start + b->size, 
						       buffer, chunksize)) {
				up(&b->sem);
				return -EFAULT;
			}

			b->size += chunksize * 2;
		}

		buffer += chunksize;
		count -= chunksize;

		if (b->size < s->fragsize) {
			up(&b->sem);
			break;
		}

		/* Send current buffer to dma */
		s->active = 1;
		mmsp2_dma_queue_buffer(s->dma_ch, (void *) b,
					 b->dma_addr, b->size);
		b->size = 0;
		NEXT_BUF(s, buf);
	}

	if ((buffer - buffer0))
		ret = buffer - buffer0;

	ds_printk("return=%d\n", ret);

	return ret;
}

static void audio_prime_dma(audio_stream_t *s)
{
	int i;

	s->active = 1;
	for (i = 0; i < s->nbfrags; i++) {
		audio_buf_t *b = s->buf;
		down(&b->sem);
		mmsp2_dma_queue_buffer(s->dma_ch, (void *) b,
					b->dma_addr, s->fragsize);
		NEXT_BUF(s, buf);
	}
}

static int audio_read(struct file *file, char *buffer, 
			size_t count, loff_t * ppos)
{
	const char *buffer0 = buffer;
	audio_state_t *state = (audio_state_t *) file->private_data;
	audio_stream_t *s = state->input_stream;
	int chunksize, ret = 0;

	ds_printk("count=%d\n", count);

	if (ppos != &file->f_pos)
		return -ESPIPE;

	/* pre dma */
	if (!s->active) {
		if (!s->buffers && audio_setup_buf(s))
			return -ENOMEM;

		audio_prime_dma(s);
	}

	while (count > 0) {
		audio_buf_t *b = s->buf;

		/* Wait for a buffer to become full */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&b->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&b->sem)) {
				break;
			}
		}

		/* Grab data from the current buffer */
		chunksize = b->size;
		if (chunksize > count)
			chunksize = count;
		ds_printk("read %d from %d\n", chunksize, s->buf_idx);
		if (copy_to_user(buffer,
				 b->start + s->fragsize - b->size, chunksize)) {
			up(&b->sem);
			return -EFAULT;
		}
		b->size -= chunksize;
		buffer += chunksize;
		count -= chunksize;
		if (b->size > 0) {
			up(&b->sem);
			break;
		}

		/* Make current buffer available for DMA again */
		mmsp2_dma_queue_buffer(s->dma_ch, (void *) b,
					b->dma_addr, s->fragsize);
		NEXT_BUF(s, buf);
	}

	if ((buffer - buffer0))
		ret = buffer - buffer0;

	ds_printk("return=%d\n", ret);

	return ret;
}

static unsigned int audio_poll(struct file *file, 
				struct poll_table_struct *wait)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *is = state->input_stream;
	audio_stream_t *os = state->output_stream;
	unsigned int mask = 0;
	int i;

	ds_printk("mode=%s\n",
		(file->f_mode & FMODE_READ) ? "r" : "",
		(file->f_mode & FMODE_WRITE) ? "w" : "");

	if (file->f_mode & FMODE_READ) {
		/* Start audio input if not already active */
		if (!is->active) {
			if (!is->buffers && audio_setup_buf(is))
				return -ENOMEM;

			audio_prime_dma(is);
		}
		poll_wait(file, &is->wq, wait);
	}

	if (file->f_mode & FMODE_WRITE) {
		if (!os->buffers && audio_setup_buf(os))
			return -ENOMEM;
		poll_wait(file, &os->wq, wait);
	}

	if (file->f_mode & FMODE_READ) {
		for (i = 0; i < is->nbfrags; i++) {
			if (atomic_read(&is->buffers[i].sem.count) > 0) {
				mask |= POLLIN | POLLRDNORM;
				break;
			}
		}
	}
	if (file->f_mode & FMODE_WRITE) {
		for (i = 0; i < os->nbfrags; i++) {
			if (atomic_read(&os->buffers[i].sem.count) > 0) {
				mask |= POLLOUT | POLLWRNORM;
				break;
			}
		}
	}

	ds_printk("returned mask of %s\n",
		(mask & POLLIN) ? "r" : "",	
		(mask & POLLOUT) ? "w" : "");

	return mask;
}

static int audio_set_fragments(audio_stream_t *s, int val)
{
	if (s->active)
		return -EBUSY;
	if (s->buffers)
		audio_clear_buf(s);
	s->nbfrags = (val >> 16) & 0x7FFF;
	val &= 0xffff;
	if (val < 4) 
		val = 4;
	if (val > 15)
		val = 15;
	s->fragsize = 1 << val;
	if (s->nbfrags < 2)
		s->nbfrags = 2;
	if (s->nbfrags * s->fragsize > 128 * 1024)
		s->nbfrags = 128 * 1024 / s->fragsize;
	if (audio_setup_buf(s))
		return -ENOMEM;
	return val|(s->nbfrags << 16);
}

static int audio_ioctl(struct inode *inode, struct file *file, 
			uint cmd, ulong arg)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;
	long val;

	/* dispatch based on command */
	switch (cmd) {
		case OSS_GETVERSION:
			return put_user(SOUND_VERSION, (int *)arg);

		case SNDCTL_DSP_GETBLKSIZE:
			if (file->f_mode & FMODE_WRITE)
				return put_user(os->fragsize, (long *) arg);
			else
				return put_user(is->fragsize, (int *)arg);

		case SNDCTL_DSP_SETFRAGMENT:
			if (get_user(val, (long *) arg))
				return -EFAULT;
			if (file->f_mode & FMODE_READ) {
				int ret = audio_set_fragments(is, val);
				if (ret < 0)
					return ret;
				ret = put_user(ret, (int *)arg);
				if (ret)
					return ret;
			}
			if (file->f_mode & FMODE_WRITE) {
				int ret = audio_set_fragments(os, val);
				if (ret < 0)
					return ret;
				ret = put_user(ret, (int *)arg);
				if (ret)
					return ret;
			}
			return 0;				

		case SNDCTL_DSP_SYNC:
			return audio_sync(file);

		case SNDCTL_DSP_GETOSPACE:
		{
			audio_buf_info *inf = (audio_buf_info *) arg;
			int err = verify_area(VERIFY_WRITE, inf, sizeof (*inf));
			int i;
			int frags = 0, bytes = 0, dma_send_bytes = 0;

			int count=0;

			if (!(file->f_mode & FMODE_WRITE))
				return -EINVAL;

			if (err)
				return err;

			if (!os->buffers && audio_setup_buf(os))
				return -ENOMEM;
			for (i = 0; i < os->nbfrags; i++) {
				if (atomic_read(&os->buffers[i].sem.count) > 0)
				{
					if (os->buffers[i].size == 0)
						frags++;
					bytes += os->fragsize -
						 os->buffers[i].size;
						 count++;
				}
			}
			
			if( frags < os->nbfrags  )
			{
				dma_send_bytes = mmsp2_dma_get_send_size(os->dma_ch);
				if( dma_send_bytes >= 0 )
				{
					bytes += dma_send_bytes;
				}
			}
			put_user(frags, &inf->fragments);
			put_user(os->nbfrags, &inf->fragstotal);
			put_user(os->fragsize, &inf->fragsize);
			put_user(bytes, &inf->bytes);
			break;
		}

		case SNDCTL_DSP_GETISPACE:
		{
			audio_buf_info *inf = (audio_buf_info *) arg;
			int err = verify_area(VERIFY_WRITE, inf, sizeof (*inf));
			int i;
			int frags = 0, bytes = 0;

			if (!(file->f_mode & FMODE_READ))
				return -EINVAL;

			if (err)
				return err;

			if (!is->buffers && audio_setup_buf(is))
				return -ENOMEM;
			for (i = 0; i < is->nbfrags; i++) {
				if (atomic_read(&is->buffers[i].sem.count) > 0)
				{
					if (is->buffers[i].size == is->fragsize)
						frags++;
					bytes += is->buffers[i].size;
				}
			}
			put_user(frags, &inf->fragments);
			put_user(is->nbfrags, &inf->fragstotal);
			put_user(is->fragsize, &inf->fragsize);
			put_user(bytes, &inf->bytes);
			break;
		}
		
		case SNDCTL_DSP_RESET:
			if (file->f_mode & FMODE_READ) {
				audio_reset_buf(is);
			}
			if (file->f_mode & FMODE_WRITE) {
				audio_reset_buf(os);
			}
			return 0;

		case SNDCTL_DSP_NONBLOCK:
			file->f_flags |= O_NONBLOCK;
			return 0;

		case SNDCTL_DSP_POST:
		case SNDCTL_DSP_SUBDIVIDE:
		case SNDCTL_DSP_GETCAPS:
		case SNDCTL_DSP_GETTRIGGER:
		case SNDCTL_DSP_SETTRIGGER:
		case SNDCTL_DSP_GETIPTR:
		case SNDCTL_DSP_GETOPTR:
		case SNDCTL_DSP_MAPINBUF:
		case SNDCTL_DSP_MAPOUTBUF:
		case SNDCTL_DSP_SETSYNCRO:
		case SNDCTL_DSP_SETDUPLEX:
			return -ENOSYS;
		default:
			return state->client_ioctl(inode, file, cmd, arg);
	}

	return 0;
}

static int audio_release(struct inode *inode, struct file *file)
{
	audio_state_t *state = (audio_state_t *)file->private_data;

	ds_printk("mp2520f audio_release\n");

	down(&state->sem);

	if (file->f_mode & FMODE_READ) {
		audio_clear_buf(state->input_stream);
		mmsp2_free_dma(state->input_stream->dma_ch);
		state->rd_ref = 0;
	}

	if (file->f_mode & FMODE_WRITE) {
		audio_sync(file);
		audio_clear_buf(state->output_stream);
		mmsp2_free_dma(state->output_stream->dma_ch);
		state->wr_ref = 0;
	}

	up(&state->sem);
	return 0;
}

int mp2520f_audio_attach(struct inode *inode, struct file *file,
			audio_state_t * state)
{
	audio_stream_t *is = state->input_stream;
	audio_stream_t *os = state->output_stream;
	int err, need_tx_dma;

	ds_printk("mp2520f audio_open\n");

	down(&state->sem);

	/* access control */
	err = -ENODEV;
	if ((file->f_mode & FMODE_WRITE) && !os)
		goto out;
	if ((file->f_mode & FMODE_READ) && !is)
		goto out;
	err = -EBUSY;
	if ((file->f_mode & FMODE_WRITE) && state->wr_ref)
		goto out;
	if ((file->f_mode & FMODE_READ) && state->rd_ref)
		goto out;

	if (file->f_mode & FMODE_WRITE)
		os->dma_ch = mmsp2_request_dma(DMA_AC97_LRPCM_OUT,
			(dma_callback_t)audio_dmaout_done_callback);
	if (file->f_mode & FMODE_READ)
	{
		is->dma_ch = mmsp2_request_dma(/*DMA_AC97_MIC_IN*/DMA_AC97_LRPCM_IN,
			(dma_callback_t)audio_dmain_done_callback);
	}


	file->private_data = state;
	file->f_op->release = audio_release;
	file->f_op->write = audio_write;
	file->f_op->read = audio_read;
	file->f_op->poll = audio_poll;
	file->f_op->ioctl = audio_ioctl;
	file->f_op->llseek = no_llseek;

	if ((file->f_mode & FMODE_WRITE)) {
		state->wr_ref = 1;
		audio_clear_buf(os);
		os->fragsize = AUDIO_FRAGSIZE_DEFAULT;
		os->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		init_waitqueue_head(&os->wq);
	}
	if ((file->f_mode & FMODE_READ)) {
		state->rd_ref = 1;
		audio_clear_buf(is);
		is->fragsize = AUDIO_FRAGSIZE_DEFAULT;
		is->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		init_waitqueue_head(&is->wq);
	}

	err = 0;

out:
	up(&state->sem);
	return err;
}

EXPORT_SYMBOL(mp2520f_audio_attach);
EXPORT_SYMBOL(audio_reset_buf);
EXPORT_SYMBOL(audio_setup_buf);

MODULE_AUTHOR("DIGNSYS Inc.(www.dignsys.com)");
MODULE_DESCRIPTION("audio interface for the MP2520F");
