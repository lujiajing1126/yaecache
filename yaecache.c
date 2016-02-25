#include "yaecache.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/* defaults */
static void settings_init(void);

/** exported globals **/
struct settings settings;

/** signal handler */
#ifndef HAVE_SIGIGNORE
static int sigignore(int sig) {
    struct sigaction sa = { .sa_handler = SIG_IGN, .sa_flags = 0 };

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) {
        return -1;
    }
    return 0;
}
#endif

static void signal_handler(const int sig) {
    printf("Signal handled: %s.\n", strsignal(sig));
    exit(EXIT_SUCCESS);
}

static void settings_init(void) {
	settings.maxconns = 1024;
	settings.verbose = 0;
}

static void usage(void) {
	printf("show usage\n");
}

static void usage_licence(void) {
	printf("show licence\n");
}

static void save_pid(const char *pid_file) {
	
}

int main(int argc, char **argv) {
	/* init local vars */
	int ch;//return val of getopt
	int maxcore = 0;
	struct rlimit rlim;
	char *username = NULL;
	struct passwd *pw;
	bool do_daemonize = false;
	bool lock_memory = false;
	int retval = EXIT_SUCCESS;
	char *pid_file = NULL;

	/* handle SIGINT and SIGTERM */
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);

	settings_init();

	/* set stderr non-buffering (for running under, say, daemontools) */
    setbuf(stderr, NULL);

    // process arguments
    while(-1 != (ch = getopt(argc,argv,
    	"u:"  /* user identity to run as */
    	"hiV" /* help, licence info, version */
    	"d"   /* daemon mode */
    	"v"   /* verbose */
		"k"   /* lock down all paged memory */
		"P:"  /* save PID in file */
    	))) {
    	switch(ch) {
    		case 'h':
            	usage();
            	exit(EXIT_SUCCESS);
            case 'i':
            	usage_licence();
            	exit(EXIT_SUCCESS);
            case 'v':
            	settings.verbose++;
            	break;
            case 'V':
            	printf(PACKAGE " " VERSION "\n");
            	exit(EXIT_SUCCESS);
    		case 'u':
    			username = optarg;
    			break;
    		case 'd':
            	do_daemonize = true;
            	break;
            case 'k':
            	lock_memory = true;
            	break;
            case 'P':
            	pid_file = optarg;
            	break;
    		default:
    			fprintf(stderr, "Illegal argument \"%c\"\n", ch);
            	return 1;
    	}
    }
    
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
    		exit(EX_OSERR);
    	}
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

    /* daemonize if requested */
    /* if we want to ensure our ability to dump core, don't chdir to / */
    if (do_daemonize) {
        if (sigignore(SIGHUP) == -1) {
            perror("Failed to ignore SIGHUP");
        }
        if (daemonize(maxcore, settings.verbose) == -1) {
            fprintf(stderr, "failed to daemon() in order to daemonize\n");
            exit(EXIT_FAILURE);
        }
    }

    /* lock paged memory if needed */
    if (lock_memory) {
#ifdef HAVE_MLOCKALL
        int res = mlockall(MCL_CURRENT | MCL_FUTURE);
        if (res != 0) {
            fprintf(stderr, "warning: -k invalid, mlockall() failed: %s\n",
                    strerror(errno));
        }
#else
        fprintf(stderr, "warning: -k invalid, mlockall() not supported on this platform.  proceeding without.\n");
#endif
    }

    /*
     * ignore SIGPIPE signals; we can use errno == EPIPE if we
     * need that information
     */
    if (sigignore(SIGPIPE) == -1) {
        perror("failed to ignore SIGPIPE; sigaction");
        exit(EX_OSERR);
    }

    if (pid_file != NULL) {
        save_pid(pid_file);
    }

	return retval;
}