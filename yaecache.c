#include "yaecache.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sysexits.h>

static void signal_handler(const int sig) {
    printf("Signal handled: %s.\n", strsignal(sig));
    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
	/* init local vars */
	int maxcore = 0;
	struct rlimit rlim;

	/* handle SIGINT and SIGTERM */
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);

	/* set stderr non-buffering (for running under, say, daemontools) */
    setbuf(stderr, NULL);

    // process arguments
    
    // init limit resources
    if (maxcore != 0) {
    	struct rlimit rlim_new;
    	if(getrlimit(RLIMIT_CORE, &rlim) == 0) {
    		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
    		if(setrlimit(RLIMIT_CORE,&rlim_new) != 0) {//set fail
    			// reset to old max
    			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
    			setrlimit(RLIMIT_CORE,&rlim_new);
    		}
    	}
    	if((getrlimit(RLIMIT_CORE,&rlim) != 0) || (rlim.rlim_cur == 0)) {
    		fprintf(stderr, "failed to ensure corefile creation\n");
    		exit(EX_OSERR);
    	}
    }


	exit(EXIT_SUCCESS);
}