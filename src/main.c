#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef HAS_SYSTEMD
#   include <systemd/sd-daemon.h>
#endif // HAS_SYSTEMD

#include <logfanoutd.h>

struct arguments {
	struct logfanoutd_listen listen;
	int verbose;
	char* root_dir;
	int log;
};
void set_default_args(struct arguments* pargs) {
	struct sockaddr* sa = &pargs->listen.value.sa;
	sa->sa_family = AF_INET;
	((struct sockaddr_in*)sa)->sin_port = htons(8000);
	((struct sockaddr_in*)sa)->sin_addr.s_addr = htonl(INADDR_ANY);
	pargs->listen.type = LOGFANOUTD_LISTEN_SOCKADDR;
	pargs->verbose = 0;
	pargs->root_dir = NULL;
	pargs->log = 0;
}

static char argp_doc[] = "logfanoutd - simple HTTP log fanout server";
static struct argp_option argp_options[] = {
	{"port",    'p', "PORT", 0, "Use port number" },
	{"root_dir",'r', "FILE", 0, "Use root dir" },
	{"verbose", 'v', 0,      0, "Produce verbose output" },
	{"log",     'l', 0,      0, "Log requests to stdout" },
	{"listen",  256, "ADDR", 0, "Address to listen on" },
#ifdef HAS_SYSTEMD
	{"systemd", 257, 0,      0, "Use systemd activated socket" },
#endif // HAS_SYSTEMD
	{ 0 }
};

static void x_sockaddr_set_port(struct sockaddr* sa, unsigned short port) {
	switch(sa->sa_family) {
	case AF_INET:
		((struct sockaddr_in*)sa)->sin_port = htons(port);
		break;
	case AF_INET6:
		((struct sockaddr_in6*)sa)->sin6_port = htons(port);
		break;
	default:;
	}
}
static unsigned short x_sockaddr_get_port(struct sockaddr* sa) {
	switch(sa->sa_family) {
	case AF_INET:
		return ntohs(((struct sockaddr_in*)sa)->sin_port);
	case AF_INET6:
		return ntohs(((struct sockaddr_in6*)sa)->sin6_port);
	default:;
	}
	return (unsigned short)(-1);
}

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;
	unsigned short port;
#ifdef HAS_SYSTEMD
	int num_sockets;
#endif // HAS_SYSTEMD

	switch (key) {
	case 'p':
		x_sockaddr_set_port(&arguments->listen.value.sa, atoi(arg));
		break;
	case 'r':
		arguments->root_dir = arg;
		break;
	case 'v':
		arguments->verbose = 1;
		break;
	case 'l':
		arguments->log = 1;
		break;
	case 256:
		port = x_sockaddr_get_port(&arguments->listen.value.sa);
		if(inet_pton(AF_INET, arg, &(((struct sockaddr_in*)(&arguments->listen.value.sa))->sin_addr))) {
			arguments->listen.value.sa.sa_family = AF_INET;
			x_sockaddr_set_port(&arguments->listen.value.sa, port);
			arguments->listen.type = LOGFANOUTD_LISTEN_SOCKADDR;
		} else if(inet_pton(AF_INET6, arg, &(((struct sockaddr_in6*)(&arguments->listen.value.sa))->sin6_addr))) {
			arguments->listen.value.sa.sa_family = AF_INET6;
			x_sockaddr_set_port(&arguments->listen.value.sa, port);
			arguments->listen.type = LOGFANOUTD_LISTEN_SOCKADDR;
		} else {
			return ARGP_ERR_UNKNOWN;
		}
		break;
#ifdef HAS_SYSTEMD
	case 257:
		num_sockets = sd_listen_fds(0);
		if (num_sockets == 1) {
			arguments->listen.type = LOGFANOUTD_LISTEN_FD;
			arguments->listen.value.fd = SD_LISTEN_FDS_START;
		} else {
			if (num_sockets == 0) {
				fprintf(stderr, "No sockets are provided by systemd\n");
			} else if (num_sockets < 0) {
				fprintf(stderr, "An error occured during sd_listen_fds\n");
			} else if (num_sockets > 1) {
				fprintf(stderr, "Too many sockets are provided by systemd\n");
			}
			return ARGP_ERR_UNKNOWN;
		}
		break;
#endif // HAS_SYSTEMD
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
static struct argp argp = { argp_options, parse_opt, 0, argp_doc };

int parse_args(int argc, char** argv, struct arguments* parguments) {
	return argp_parse(&argp, argc, argv, 0, 0, parguments);
}

int main(int argc, char** argv) {
	struct arguments arguments;
	struct logfanoutd_state* plf_state;

	set_default_args(&arguments);
	if(parse_args(argc, argv, &arguments))
		return 1;

	plf_state = logfanoutd_start(&(arguments.listen), arguments.verbose, arguments.log, arguments.root_dir);
	if(plf_state == NULL)
		return 1;

	getchar();

	logfanoutd_stop(plf_state);
	return 0;
}
