#ifndef _LOGFANOUNTD_H
#define _LOGFANOUNTD_H

#include <microhttpd.h>

struct logfanoutd_state {
	struct MHD_Daemon* MHD_Daemon;
	char* root_dir;
};

struct logfanoutd_state* logfanoutd_start(unsigned short port, int verbose, const char* root_dir);
void logfanoutd_stop(struct logfanoutd_state* state);

#endif // _LOGFANOUNTD_H