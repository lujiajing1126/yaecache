#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* When adding a setting, be sure to update process_stat_settings */
/**
 * Globally accessible settings as derived from the commandline.
 */
struct settings {
	int maxconns;
	int verbose;
};

/*
 * Functions
 */

extern int daemonize(int nochdir, int noclose);