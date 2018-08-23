#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
//static struct semaphore *intersectionSem;
static struct cv *S;
static struct cv *E;
static struct cv *N;
static struct cv *W;
static struct lock *cv_l;
static int se;
static int sn;
static int sw;
static int en;
static int ew;
static int es;
static int nw;
static int ns;
static int ne;
static int ws;
static int we;
static int wn;

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  //intersectionSem = sem_create("intersectionSem",1);
  //if (intersectionSem == NULL) {
  //  panic("could not create intersection semaphore");
  //}
  //return;

	//-------------------------------------------------------------------
	cv_l = lock_create("cv_l");
	if (cv_l == NULL) {
		panic("Could not create cv_l lock");
	}
	S = cv_create("S");
	if (S == NULL) {
		panic("Could not create S condition variable");
	}
	E = cv_create("E");
	if (E == NULL) {
                panic("Could not create E condition variable");
        }
	N = cv_create("N");
        if (N == NULL) {
                panic("Could not create N condition variable");
        }
	W = cv_create("W");
        if (W == NULL) {
                panic("Could not create W condition variable");
        }
	se = 0;
	sn = 0;
	sw = 0;
	en = 0;
	ew = 0;
	es = 0;
	nw = 0;
	ns = 0;
	ne = 0;
	ws = 0;
	we = 0;
	wn = 0;
	return;
	//-------------------------------------------------------------------
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  //KASSERT(intersectionSem != NULL);
  //sem_destroy(intersectionSem);

	//----------------------------------------------------------------------
	KASSERT(cv_l != NULL);
	lock_destroy(cv_l);
	KASSERT(S != NULL);
        cv_destroy(S);
     	KASSERT(E != NULL);
        cv_destroy(E);
        KASSERT(N != NULL);
        cv_destroy(N);
        KASSERT(W != NULL);
        cv_destroy(W);
	return;
	//-----------------------------------------------------------------------
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  //(void)origin;  /* avoid compiler complaint about unused parameter */
  //(void)destination; /* avoid compiler complaint about unused parameter */
  //KASSERT(intersectionSem != NULL);
  //P(intersectionSem);

	//----------------------------------------------------------------------
	lock_acquire(cv_l);
	if (origin == south) {
		if (destination == east) {
			while ((we > 0) || (wn > 0) || (ne > 0)) {
				cv_wait(S, cv_l);
			}
			se++; 
		} else if (destination == north) {
			while ((we > 0) || (wn > 0) || (ne > 0) || (ew > 0) || (en > 0) || (es > 0)) {
				cv_wait(S, cv_l);
			}
			sn++;
		} else if (destination == west) {
			while ((we > 0) || (wn > 0) || (ne > 0) || (ew > 0) || (en > 0) || (es > 0) || (ns > 0) || (nw > 0)) {
				cv_wait(S, cv_l);
			}
			sw++;
		}
	} else if (origin == east) {
		if (destination == north) {
			while ((sn > 0) || (sw > 0) || (wn > 0)) {
				cv_wait(E, cv_l);
			}
			en++;
		} else if (destination == west) {
			while ((sn > 0) || (sw > 0) || (wn > 0) || (ns > 0) || (nw > 0) || (ne > 0)) {
				cv_wait(E, cv_l);
			}
			ew++;
		} else if (destination == south) {
			while ((sn > 0) || (sw > 0) || (wn > 0) || (ns > 0) || (nw > 0) || (ne > 0) || (we > 0) || (ws > 0)) {
				cv_wait(E, cv_l);
			}
			es++;
		}
	} else if (origin == north) {
		if (destination == west) {
			while ((ew > 0) || (es > 0) || (sw > 0)) {
				cv_wait(N, cv_l);
			}
			nw++;
		} else if (destination == south) {
			while ((ew > 0) || (es > 0) || (sw > 0) || (we > 0) || (ws > 0) || (wn > 0)) {
				cv_wait(N, cv_l);
			}
			ns++;
		} else if (destination == east) {
			while ((ew > 0) || (es > 0) || (sw > 0) || (we > 0) || (ws > 0) || (wn > 0) || (sn > 0) || (se > 0)) {
				cv_wait(N, cv_l);
			}
			ne++;
		}
	} else if (origin == west) {
		if (destination == south) {
			while ((ns > 0) || (ne > 0) || (es > 0)) {
				cv_wait(W, cv_l);
			}
			ws++;
		} else if (destination == east) {
			while ((ns > 0) || (ne > 0) || (es > 0) || (sn > 0) || (se > 0) || (sw > 0)) {
				cv_wait(W, cv_l);
			}
			we++;
		} else if (destination == north) {
			while ((ns > 0) || (ne > 0) || (es > 0) || (sn > 0) || (se > 0) || (sw > 0) || (ew > 0) || (en > 0)) {
				cv_wait(W, cv_l);
			}
			wn++;
		}
	} 
	lock_release(cv_l);
	return;
	//----------------------------------------------------------------------
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  //(void)origin;  /* avoid compiler complaint about unused parameter */
  //(void)destination; /* avoid compiler complaint about unused parameter */
  //KASSERT(intersectionSem != NULL);
  //V(intersectionSem);

	//------------------------------------------------------------------------
	lock_acquire(cv_l);
	if (origin == south) {
                if (destination == east) {
			se--;
                } else if (destination == north) {
			sn--;
		} else if (destination == west) {
			sw--;
		}
		cv_broadcast(W, cv_l);
		cv_broadcast(E, cv_l);
		cv_broadcast(N, cv_l);
	} else if (origin == east) {
		if (destination == north) {
			en--;
		} else if (destination == west) {
                        ew--;
                } else if (destination == south) {
                        es--;
                }
		cv_broadcast(S, cv_l);
		cv_broadcast(N, cv_l);
		cv_broadcast(W, cv_l);
	} else if (origin == north) {
                if (destination == west) {
                        nw--;
                } else if (destination == south) {
                        ns--;
                } else if (destination == east) {
                        ne--;
                }
		cv_broadcast(E, cv_l);
		cv_broadcast(W, cv_l);
		cv_broadcast(S, cv_l);
        } else if (origin == west) {
                if (destination == south) {
                        ws--;
                } else if (destination == east) {
                        we--;
                } else if (destination == north) {
                        wn--;
                }
		cv_broadcast(N, cv_l);
		cv_broadcast(S, cv_l);
		cv_broadcast(E, cv_l);
        }
	lock_release(cv_l);
       	//------------------------------------------------------------------------
}
