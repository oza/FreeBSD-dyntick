/*-
 * Copyright (c) 2010 Tsuyoshi Ozawa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



/* including for "kldloading", "kldunloading"  */
#include <sys/param.h> 
#include <sys/module.h> 
#include <sys/kernel.h>
#include <sys/systm.h>
/* including for handling IRQ  */
#include <sys/bus.h> 
#include <machine/resource.h> /*To use SYS_RES_IRQ*/
#include <sys/rman.h> /*To use RF_ACTIVE*/

#include <sys/callout.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include <machine/pcpu.h>
#include <machine/frame.h>

/* prototype declaration of device functions. */
/*
static int test_probe(device_t dev);
static int test_attach(device_t dev);
static int test_detach(device_t dev);
*/

//struct callout_cpu cc_cpu;

extern int (*mycallback)(void);
extern int callwheelmask;
extern void (*__tick_sched)(void);
extern void (*lapic_handler)(struct trapframe *frame);
extern void lapic_handle_timer(struct trapframe *frame);
extern void dynamic_lapic_handle_timer(struct trapframe *frame);
extern void __lapic_handle_timer(struct trapframe *frame);

void lapic_timer_periodic(u_int count);
void lapic_timer_oneshot(u_int count);

int test_intr(void *v);
/*
#define CC_CPU(cpu)     &cc_cpu
#define CC_SELF()       &cc_cpu
*/

struct callout_cpu {
	struct mtx		cc_lock;
	struct callout		*cc_callout;
	struct callout_tailq	*cc_callwheel;
	struct callout_list	cc_callfree;
	struct callout		*cc_next;
	struct callout		*cc_curr;
	void			*cc_cookie;
	int 			cc_softticks;
	int			cc_cancel;
	int			cc_waiting;
};

extern struct callout_cpu cc_cpu;
extern int callwheelsize;

static int
callout_get_next_event(void)
{
	struct callout_cpu *cc;
	struct callout *c;
	struct callout_tailq *sc;
	//struct callout *c_start;
	//struct callout_tailq *bucket;
	int curticks;
	//int basetick; // scan start ticks
	//int cnt = 0;
	int skip = 1;

	cc = &cc_cpu;
	curticks = cc->cc_softticks;
#if 0
	bucket = &cc->cc_callwheel[ curticks & callwheelmask ];
	c = TAILQ_FIRST(bucket);
	printf("curtick %d basetick %d cnt %d c %p\n",curticks,basetick,cnt,c);
#else
	uprintf("current time is %d\n",curticks);

	while( skip < ncallout ) {
		sc = &cc->cc_callwheel[ (curticks+skip) & callwheelmask ];
		/* search scanning ticks */
		TAILQ_FOREACH( c, sc, c_links.tqe ){
			printf("c %p ", c );
			if( c && c->c_time <= curticks + ncallout ){
				if( c->c_time > 0 ){
					uprintf("There is a event at %d \n",c->c_time);
#if 0
					break;
#else
					goto out;
#endif
				}
			}
		}
		skip++;
	}

out:
	uprintf("next event tick is %d",skip);
	uprintf("\n");
#endif
	
#if 0
	while( 1 ){
		printf("cc %p softticks %d callwheel %p\n", cc, curticks, bucket );
		printf("c->cc_time %d cnt %d\n",c->c_time,cnt);

		if( c == NULL ){
			c = cc->cc_next;
			continue;
		}

		if( (c->c_time <= basetick + ncallout) && (cnt < loop_max) ){
			goto out;
		}
		cnt++;
		c = cc->cc_next;
	}
out:
#else
#endif

	return 0;
}

static void 
say_hello(void)
{
	printf("hoge");
}

/* need for "kldloading", "kldunloading"  */
static int 
test_modevent( struct module *module, int event, void *args) 
{
        int e = 0; 

        switch (event) {
        case MOD_LOAD:
                uprintf("Hello Kernel Module in FreeBSD!\n");
		//mycallback = callout_get_next_event;
		lapic_handler=dynamic_lapic_handle_timer;
		callout_get_next_event();
		__tick_sched=say_hello;
                break;
        case MOD_UNLOAD:
                uprintf("Bye ,Kernel Module!\n");
		lapic_handler=__lapic_handle_timer;
		__tick_sched = NULL;
                break;
        default:
                e = EOPNOTSUPP;
        }   
        return (e);
}

static moduledata_t test_conf ={
        "test",
        test_modevent,
        NULL
};

DECLARE_MODULE(test, test_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);

#if 0
/* Interrupt Handler */
int
test_intr(void *v) 
{
        uprintf("Wow! A interrupt occured!\n");
        return (1);
}

/* need for using IRQ */
static device_method_t test_methods[] = {
        DEVMETHOD(device_probe,test_probe),
        DEVMETHOD(device_attach,test_attach),
        DEVMETHOD(device_detach,test_detach),
        { 0, 0 }
};

struct test_softc {
        device_t test_dev;
        struct resource     *irq;
        int                 irq_rid;
        void                *irq_handle;

        void                *cookie;
};

static driver_t test_driver = {
        "test",
        test_methods,
        sizeof(struct test_softc),
};

static int
test_probe(device_t self)
{
        struct resource *res;
        struct test_softc *test_softc;

        uprintf("proving...\n");

        uprintf("start to examine whether IRQ is free or not...");
        test_softc = device_get_softc(self);
        res = bus_alloc_resource_any(self,SYS_RES_IRQ,&test_softc->irq_rid,RF_ACTIVE);
        //res = BUS_ALLOC_RESOURCE_ANY(self,SYS_RES_IRQ,&test_softc->irq_rid,RF_ACTIVE); 

        if ( res == NULL ){
                uprintf("failed.\n");
                return (1);
        }

        uprintf("succeeded.\n");
        uprintf("Next, release resource...");
        bus_release_resource(self,SYS_RES_IRQ,test_softc->irq_rid,res);
        //BUS_RELEASE_RESOURCE(self,SYS_RES_IRQ,test_softc->irq_rid,res);
        uprintf("succeeded.\n");

        return (0);
}

static int
test_attach(device_t self)
{
        struct test_softc *test_softc;

        uprintf("attacing...\n");
        test_softc = device_get_softc(self);
        test_softc->irq = bus_alloc_resource_any(self,SYS_RES_IRQ,&test_softc->irq_rid,RF_ACTIVE);

        if ( test_softc->irq == NULL ){
                uprintf("failed.\n");
                return (1);
        }
        uprintf("succeeded.\n");
        uprintf("Next, start to register interrput handler...");
        bus_setup_intr(self,test_softc->irq,INTR_TYPE_MISC,NULL,(driver_intr_t*)test_intr,test_softc,&test_softc->cookie);
        uprintf("succeeded.\n");

        return (0);
}

static int
test_detach(device_t self)
{
        struct test_softc *test_softc;
        uprintf("detaching...\n");
        test_softc = device_get_softc(self);
        uprintf("Teardowning interrput handler...\n");
        bus_teardown_intr(self,test_softc->irq,test_softc->cookie);
        uprintf("succeeded.");
        uprintf("Next, start to release IRQ...");
        bus_release_resource(self,SYS_RES_IRQ,test_softc->irq_rid,test_softc->irq);

        return (0);
}

devclass_t test_devclass;

DRIVER_MODULE(test, pci, test_driver, test_devclass, 0, 0);
#endif

