/*
 * IPVS:        Shortest Expected Delay scheduling module
 *
 * Version:     $Id$
 *
 * Authors:     Wensong Zhang <wensong@linuxvirtualserver.org>
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Changes:
 *
 */

/*
 * The SED algorithm attempts to minimize each job's expected delay until
 * completion. The expected delay that the job will experience is
 * (Ci + 1) / Ui if sent to the ith server, in which Ci is the number of
 * jobs on the the ith server and Ui is the fixed service rate (weight) of
 * the ith server. The SED algorithm adopts a greedy policy that each does
 * what is in its own best interest, i.e. to join the queue which would
 * minimize its expected delay of completion.
 *
 * See the following paper for more information:
 * A. Weinrib and S. Shenker, Greed is not enough: Adaptive load sharing
 * in large heterogeneous systems. In Proceedings IEEE INFOCOM'88,
 * pages 986-994, 1988.
 *
 * Thanks must go to Marko Buuri <marko@buuri.name> for talking SED to me.
 *
 * The difference between SED and WLC is that SED includes the incoming
 * job in the cost function (the increment of 1). SED may outperform
 * WLC, while scheduling big jobs under larger heterogeneous systems
 * (the server weight varies a lot).
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <net/ip_vs.h>


static int
ip_vs_sed_init_svc(struct ip_vs_service *svc)
{
	return 0;
}


static int
ip_vs_sed_done_svc(struct ip_vs_service *svc)
{
	return 0;
}


static int
ip_vs_sed_update_svc(struct ip_vs_service *svc)
{
	return 0;
}


static inline unsigned int
ip_vs_sed_dest_overhead(struct ip_vs_dest *dest)
{
	/*
	 * We only use the active connection number in the cost
	 * calculation here.
	 */
	return atomic_read(&dest->activeconns) + 1;
}


/*
 *	Weighted Least Connection scheduling
 */
static struct ip_vs_dest *
ip_vs_sed_schedule(struct ip_vs_service *svc, struct iphdr *iph)
{
	register struct list_head *l, *e;
	struct ip_vs_dest *dest, *least;
	unsigned int loh, doh;

	IP_VS_DBG(6, "ip_vs_sed_schedule(): Scheduling...\n");

	/*
	 * We calculate the load of each dest server as follows:
	 *	(server expected overhead) / dest->weight
	 *
	 * Remember -- no floats in kernel mode!!!
	 * The comparison of h1*w2 > h2*w1 is equivalent to that of
	 *		  h1/w1 > h2/w2
	 * if every weight is larger than zero.
	 *
	 * The server with weight=0 is quiesced and will not receive any
	 * new connections.
	 */

	l = &svc->destinations;
	for (e=l->next; e!=l; e=e->next) {
		least = list_entry(e, struct ip_vs_dest, n_list);
		if (atomic_read(&least->weight) > 0) {
			loh = ip_vs_sed_dest_overhead(least);
			goto nextstage;
		}
	}
	return NULL;

	/*
	 *    Find the destination with the least load.
	 */
  nextstage:
	for (e=e->next; e!=l; e=e->next) {
		dest = list_entry(e, struct ip_vs_dest, n_list);
		doh = ip_vs_sed_dest_overhead(dest);
		if (loh * atomic_read(&dest->weight) >
		    doh * atomic_read(&least->weight)) {
			least = dest;
			loh = doh;
		}
	}

	IP_VS_DBG(6, "SED: server %u.%u.%u.%u:%u "
		  "activeconns %d refcnt %d weight %d overhead %d\n",
		  NIPQUAD(least->addr), ntohs(least->port),
		  atomic_read(&least->activeconns),
		  atomic_read(&least->refcnt),
		  atomic_read(&least->weight), loh);

	return least;
}


static struct ip_vs_scheduler ip_vs_sed_scheduler =
{
	.name =			"sed",
	.refcnt =		ATOMIC_INIT(0),
	.module =		THIS_MODULE,
	.init_service =		ip_vs_sed_init_svc,
	.done_service =		ip_vs_sed_done_svc,
	.update_service =	ip_vs_sed_update_svc,
	.schedule =		ip_vs_sed_schedule,
};


static int __init ip_vs_sed_init(void)
{
	INIT_LIST_HEAD(&ip_vs_sed_scheduler.n_list);
	return register_ip_vs_scheduler(&ip_vs_sed_scheduler);
}

static void __exit ip_vs_sed_cleanup(void)
{
	unregister_ip_vs_scheduler(&ip_vs_sed_scheduler);
}

module_init(ip_vs_sed_init);
module_exit(ip_vs_sed_cleanup);
MODULE_LICENSE("GPL");
