#ifndef _LOGFANOUNTD_H
#define _LOGFANOUNTD_H

#include <netinet/in.h>

#include <microhttpd.h>

struct logfanoutd_state {
	struct MHD_Daemon* MHD_Daemon;
	char* root_dir;
};

struct logfanoutd_listen {
	enum {
		LOGFANOUTD_LISTEN_SOCKADDR
	} type;
	union {
		struct sockaddr sa;
	} value;
};

struct logfanoutd_state* logfanoutd_start(struct logfanoutd_listen* listen, int verbose, int log, const char* root_dir);
void logfanoutd_stop(struct logfanoutd_state* state);

#endif // _LOGFANOUNTD_H
