///[*]----------------------------------------------------------------------------------------------------[*]
#ifndef __MPLAYER_MIXER
#define __MPLAYER_MIXER
///[*]----------------------------------------------------------------------------------------------------[*]
#include "libaf/af.h"
#include "libao2/audio_out.h"
///[*]----------------------------------------------------------------------------------------------------[*]
extern char    *mixer_device;
extern char    *mixer_channel;
extern int      soft_vol;
extern float    soft_vol_max;
///[*]----------------------------------------------------------------------------------------------------[*]
typedef struct mixer_s
{
    ao_functions_t *audio_out;
    af_stream_t    *afilter;
    int             volstep;
    int             muted;
    float           last_l,
                    last_r;
} mixer_t;
///[*]----------------------------------------------------------------------------------------------------[*]
void mixer_getvolume(mixer_t * mixer, float *l, float *r);
void mixer_setvolume(mixer_t * mixer, float l, float r);
void mixer_incvolume(mixer_t * mixer);
void mixer_decvolume(mixer_t * mixer);
void mixer_getbothvolume(mixer_t * mixer, float *b);
void mixer_mute(mixer_t * mixer);
void mixer_hwEq(mixer_t *mixer ,int eqMode);
//extern void mixer_setbothvolume( int v );
///[*]----------------------------------------------------------------------------------------------------[*]
#define 		mixer_setbothvolume(m, v) mixer_setvolume(m, v, v)
///[*]----------------------------------------------------------------------------------------------------[*]
#endif
///[*]----------------------------------------------------------------------------------------------------[*]
