#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include <logfanoutd.h>

struct arguments {
	unsigned short port;
	int verbose;
	char* root_dir;
};
void set_default_args(struct arguments* pargs) {
	pargs->port = 8000;
	pargs->verbose = 0;
	pargs->root_dir = NULL;
}

static char argp_doc[] = "logfanoutd - simple HTTP log fanout server";
static struct argp_option argp_options[] = {
	{"port",    'p', "PORT", 0, "Use port number" },
	{"root_dir",'r', "FILE", 0, "Use root dir" },
	{"verbose", 'v', 0,      0, "Produce verbose output" },
	{ 0 }
};
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
	case 'p':
		arguments->port = atoi(arg);
		break;
	case 'r':
		arguments->root_dir = arg;
		break;
	case 'v':
		arguments->verbose = 1;
		break;
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

	printf("Using port: %d\n", arguments.port);

	plf_state = logfanoutd_start(arguments.port, arguments.verbose, arguments.root_dir);
	if(plf_state == NULL)
		return 1;

	getchar();

	logfanoutd_stop(plf_state);
	return 0;
}
