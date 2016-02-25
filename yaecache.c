#include "yaecache.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sysexits.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

/* defaults */
static void settings_init(void);

/** exported globals **/
struct settings settings;

static void signal_handler(const int sig) {
    printf("Signal handled: %s.\n", strsignal(sig));
    exit(EXIT_SUCCESS);
}

static void settings_init(void) {
	settings.maxconns = 1024;
}


int main(int argc, char **argv) {
	/* init local vars */
	int maxcore = 0;
	struct rlimit rlim;
	char *username = NULL;
	struct passwd *pw;

	/* handle SIGINT and SIGTERM */
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);

	settings_init();
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

    if (getrlimit(RLIMIT_NOFILE,&rlim) != 0) { // get rlimit fail
    	fprintf(stderr, "failed to getrlimit number of files\n");  
    	exit(EX_OSERR);
    } else {
    	rlim.rlim_cur = settings.maxconns;
    	rlim.rlim_max = settings.maxconns;
    	if(setrlimit(RLIMIT_NOFILE,&rlim) != 0) {
    		fprintf(stderr, "failed to set rlimit for open files. Try starting as root or requesting smaller maxconns value.\n");
    	}
    	exit(EX_OSERR);
    }

    // init users and groups things
    if(getuid() == 0 || geteuid() == 0) {
    	if(username == 0 || *username == '\0') {
    		fprintf(stderr, "yaecache cannot run as root, you should specify a user with startup\n");
    		exit(EX_USAGE);
    	}
    	if((pw = getpwnam(username)) == 0) {// null pointer
    		fprintf(stderr, "user %s doesnot exist\n", username);
    		exit(EX_USAGE);
    	}

    	if(setgid(pw->pw_gid) < 0 || setuid(pw->pw_uid) < 0) {
    		 fprintf(stderr, "failed to assume identity of user %s\n", username);  
        	exit(EX_OSERR); 
    	}
    }

	exit(EXIT_SUCCESS);
}