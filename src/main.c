#include <stdlib.h>
#include <argp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAS_SYSTEMD
#   include <systemd/sd-daemon.h>
#endif // HAS_SYSTEMD

#include <logfanoutd.h>

struct aliases {
	struct vpath_pair* aliases;
	size_t size;
	size_t capacity;
};

static void init_aliases(struct aliases* aliases) {
	aliases->aliases = NULL;
	aliases->capacity = 0;
	aliases->size = 0;
}

static void destroy_aliases(struct aliases* aliases) {
	free(aliases->aliases);
	aliases->aliases = NULL;
	aliases->capacity = 0;
	aliases->size = 0;
}

static int grow_aliases(struct aliases* aliases) {
	size_t new_capacity = aliases->capacity ? aliases->capacity * 2 : 1;
	struct vpath_pair* new_aliases = realloc(aliases->aliases, sizeof(struct vpath_pair) * new_capacity);

	if (new_aliases == NULL)
		return -1;

	aliases->aliases = new_aliases;
	aliases->capacity = new_capacity;

	return 0;
}

static struct vpath_pair* append_alias(struct aliases* aliases) {
	if (aliases->capacity == aliases->size && grow_aliases(aliases))
		return NULL;

	return &aliases->aliases[aliases->size++];
}

struct arguments {
	struct logfanoutd_listen listen;
	int verbose;
	int log;
	struct aliases aliases;
};

static void init_arguments(struct arguments* arguments) {
	struct sockaddr* sa = &arguments->listen.value.sa;
	sa->sa_family = AF_INET;
	((struct sockaddr_in*)sa)->sin_port = htons(8000);
	((struct sockaddr_in*)sa)->sin_addr.s_addr = htonl(INADDR_ANY);
	arguments->listen.type = LOGFANOUTD_LISTEN_SOCKADDR;
	arguments->verbose = 0;
	arguments->log = 0;
	init_aliases(&arguments->aliases);
}

static void destroy_arguments(struct arguments* arguments) {
	destroy_aliases(&arguments->aliases);
}

static char argp_doc[] = "logfanoutd - simple HTTP log fanout server";
static struct argp_option argp_options[] = {
	{"port",    'p', "PORT", 0, "Use port number" },
	{"root_dir",'r', "FILE", 0, "Use root dir" },
	{"alias",   'a', "VPATH:PPATH", 0, "Alias physical path PPATH to virtual VPATH" },
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
static char* parse_alias_opt(char* arg) {
	char* ret = NULL;
	char* out = arg;

	while (arg[0] != '\0') {
		switch (arg[0]) {
		case ':':
			ret = ++arg;
			*out = '\0';
			out = ret;
		break;
		case '\\':
			if (arg[1] == ':') {
				arg++; // skip escaping slash
			}
		default:
			*out++ = *arg++;
		}
	}

	return ret;
}

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;
	unsigned short port;
#ifdef HAS_SYSTEMD
	int num_sockets;
#endif // HAS_SYSTEMD
	struct vpath_pair* alias;
	struct alias_list_element* alias_element;

	switch (key) {
	case 'p':
		x_sockaddr_set_port(&arguments->listen.value.sa, atoi(arg));
		break;
	case 'a':
		alias = append_alias(&arguments->aliases);
		if (alias == NULL) {
			return ARGP_ERR_UNKNOWN;
		}
		alias->vpath = arg;
		alias->ppath = parse_alias_opt(arg);
		if (alias->ppath == NULL) {
			return ARGP_ERR_UNKNOWN;
		}
		break;
	case 'r':
		alias = append_alias(&arguments->aliases);
		if (alias == NULL) {
			return ARGP_ERR_UNKNOWN;
		}
		alias->vpath = "/";
		alias->ppath = arg;
		if (alias->ppath == NULL) {
			return ARGP_ERR_UNKNOWN;
		}
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

void handle_signal(int signum) {
}

int main(int argc, char** argv) {
	struct arguments arguments;
	struct logfanoutd_state* plf_state;
	struct vpath_pair** aliases;
	size_t aliases_size;
	struct vpath_lookup* vpath_lookup;
	int ret = 1;

	signal(SIGINT, &handle_signal);
	signal(SIGTERM, &handle_signal);

	init_arguments(&arguments);

	if (parse_args(argc, argv, &arguments)) {
		goto fail_parse_args;
	}

	vpath_lookup = init_vpath_lookup(arguments.aliases.aliases, arguments.aliases.size);
	if (vpath_lookup == NULL) {
		goto fail_alloc_vpath;
	}

	destroy_aliases(&arguments.aliases);

	plf_state = logfanoutd_start(&arguments.listen, arguments.verbose, arguments.log, vpath_lookup);
	if (plf_state == NULL) {
		goto fail_logfanoutd_start;
	}

	pause();

	logfanoutd_stop(plf_state);
	ret = 0;

fail_logfanoutd_start:
	free_vpath_lookup(vpath_lookup);
fail_alloc_vpath:
fail_parse_args:
	destroy_arguments(&arguments);
	return ret;
}
