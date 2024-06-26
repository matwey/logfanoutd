#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>

#include <range.h>
#include <logfanoutd.h>
#include <vpath.h>

#ifndef MHD_HAS_RFFO64
struct MHD_Response * MHD_create_response_from_fd_at_offset64(uint64_t size, int fd, uint64_t offset) {
	const off_t offset2 = offset;
	if((uint64_t)offset2 != offset) // Overflow here
		return NULL;
	return MHD_create_response_from_fd_at_offset(size, fd, offset2);
}
#endif

#define max(x,y) ((x)>(y)?(x):(y))

size_t unescape_callback(void *cls, struct MHD_Connection *conn, char *uri) {
	return remove_dot_segments(uri, uri);
}

struct dir_list_state {
	char* path;
	DIR* dir;
	size_t item_type;
	struct dirent *dentry;
};

static struct dir_list_state* dir_list_callback_init(struct vpath* pvpath) {
	struct dir_list_state* pdls = malloc(sizeof(struct dir_list_state));
	if(pdls==NULL)
		return NULL;
	pdls->item_type = 0;
	pdls->path = pvpath->ppath;
	if(pdls->path==NULL) {
		free(pdls);
		return NULL;
	}
	pdls->dir = opendir(pdls->path);
	pdls->dentry = NULL;
	if(pdls->dir==NULL) {
		free(pdls);
		return NULL;
	}
	return pdls;
}
static ssize_t dir_list_callback_bra(struct dir_list_state* pdls, char** pbuf, size_t* pmax) {
	int ret = 0;
	if(pdls->item_type == 0){
		ret = snprintf(*pbuf, *pmax, "{");
		if(ret > *pmax)
			return -1;
		*pbuf += ret;
		*pmax -= ret;
		pdls->item_type++;
	}
	return ret;
}
static ssize_t dir_list_callback_item(struct dir_list_state* pdls, char** pbuf, size_t* pmax) {
	int ret = 0;
	if(pdls->dentry != NULL) {
		ret = snprintf(*pbuf, *pmax, "%s\"%s\":{}", (pdls->item_type==1) ? "" : ",", pdls->dentry->d_name);
		if(ret > *pmax)
			return -1;
		*pbuf += ret;
		*pmax -= ret;
		if(pdls->item_type==1)
			pdls->item_type++;
	}
	return ret;
}
static ssize_t dir_list_callback_ket(struct dir_list_state* pdls, char** pbuf, size_t* pmax) {
	int ret = 0;
	if(pdls->dentry == NULL && pdls->item_type > 0) {
		ret = snprintf(*pbuf, *pmax, "}");
		if(ret > *pmax)
			return -1;
		*pbuf += ret;
		*pmax -= ret;
		pdls->item_type=3;
	}
	return ret;
}
static ssize_t dir_list_callback(void *cls, uint64_t pos, char *buf, size_t max) {
	const size_t init_max = max;
	struct dir_list_state* pdls = cls;
	
	if(pdls->item_type==3)
		return MHD_CONTENT_READER_END_OF_STREAM;

	if(dir_list_callback_bra(pdls, &buf, &max) < 0) {
		goto end;
	}
	if(dir_list_callback_item(pdls, &buf, &max) < 0) {
		goto end;
	}
	do {
		pdls->dentry = readdir(pdls->dir);
	} while(pdls->dentry != NULL && pdls->dentry->d_name[0] == '.');
	if(dir_list_callback_ket(pdls, &buf, &max) < 0) {
		goto end;
	}

end:
	return init_max - max;
}
static void dir_list_callback_free(void *cls) {
	struct dir_list_state* pdls = cls;
	closedir(pdls->dir);
	free(pdls);
}
static void get_header_range(struct MHD_Connection *connection, struct range_set* prs) {
	prs->type = range_unknown;
	MHD_get_connection_values(connection, MHD_HEADER_KIND, range_iterator, prs);
}
static int add_header_printf(struct MHD_Response *response, const char* hdr, const char* fmt, ...) {
	char* buf = NULL;
	size_t buflen = 512;
	int msglen,ret;
	va_list ap;
	va_start(ap, fmt);
	do {
		if(buf != NULL) free(buf);
		buflen *= 2;
		buf = calloc(buflen, sizeof(char));
		if(buf == NULL) return MHD_NO;
		msglen = vsnprintf(buf, buflen, fmt, ap);
	} while(msglen >= buflen);

	ret = MHD_add_response_header(response, hdr, buf);
	free(buf);
	return ret;
}
static int add_header_range(struct MHD_Response *response, uint64_t size, uint64_t rsize, uint64_t offset) {
	return add_header_printf(response, MHD_HTTP_HEADER_CONTENT_RANGE, "bytes %" PRIuMAX "-%" PRIuMAX "/%" PRIuMAX,
		(intmax_t)offset, (intmax_t)rsize+(intmax_t)offset-1, (intmax_t)size);
}
static int add_header_range_error(struct MHD_Response *response, uint64_t size) {
	return add_header_printf(response, MHD_HTTP_HEADER_CONTENT_RANGE, "bytes */%" PRIuMAX, (intmax_t)size);
}
static int handle_error(struct MHD_Connection *connection, unsigned int status_code) {
	static char empty[1] = {0};
	struct MHD_Response *response;
	int ret;

	response = MHD_create_response_from_buffer(0, empty, MHD_RESPMEM_PERSISTENT);
	ret = MHD_queue_response (connection, status_code, response);
	MHD_destroy_response (response);
	return ret;
}
static int handle_error_range(struct MHD_Connection *connection, uint64_t size) {
	static char empty[1] = {0};
	struct MHD_Response *response;
	int ret;

	response = MHD_create_response_from_buffer(0, empty, MHD_RESPMEM_PERSISTENT);
	add_header_range_error(response, size);
	ret = MHD_queue_response (connection,
#if MHD_VERSION < 0x00097400
		MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE,
#else
		MHD_HTTP_RANGE_NOT_SATISFIABLE,
#endif // MHD_VERSION
		response);
	MHD_destroy_response (response);
	return ret;
}
static int handle_dir(struct MHD_Connection *connection, struct vpath* pvpath) {
	struct dir_list_state* pdls;
	struct MHD_Response *response;
	int ret;

	pdls = dir_list_callback_init(pvpath);
	if(pdls == NULL) {
		return handle_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 4096, dir_list_callback, pdls, dir_list_callback_free);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}
static int handle_file_full(struct MHD_Connection *connection, int fd, uint64_t size) {
	struct MHD_Response *response;
	int ret;

	response = MHD_create_response_from_fd(size, fd);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}
static int handle_file_range(struct MHD_Connection *connection, int fd, uint64_t size, struct range_set* prs) {
	int ret;
	struct MHD_Response *response;
	uint64_t rsize;
	uint64_t offset;

	if(!is_valid_range(prs, size)) {
		close(fd);
		return handle_error_range(connection, size);
	}

	range_to_size_and_offset(prs, size, &rsize, &offset);
	response = MHD_create_response_from_fd_at_offset64(rsize, fd, offset);
	if(response == NULL)
		goto fail;
	if(add_header_range(response, size, rsize, offset) != MHD_YES)
		goto fail_destroy;
	ret = MHD_queue_response(connection, MHD_HTTP_PARTIAL_CONTENT, response);
	MHD_destroy_response(response);
	return ret;

fail_destroy:
	MHD_destroy_response(response);
fail:
	return handle_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
}
static int handle_file(struct MHD_Connection *connection, struct vpath* pvpath) {
	int fd;
	struct range_set rs;

	fd = open(pvpath->ppath, O_RDONLY);
	if(fd < 0) {
		return handle_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	get_header_range(connection, &rs);
	if(rs.type == range_unknown) {
		return handle_file_full(connection, fd, pvpath->stat.st_size);
	}
	return handle_file_range(connection, fd, pvpath->stat.st_size, &rs);
}

#if MHD_VERSION < 0x00097100
int request_handler(void *cls, struct MHD_Connection *connection,
#else
enum MHD_Result request_handler(void *cls, struct MHD_Connection *connection,
#endif // MHD_VERSION
	const char *url, const char *method, const char *version,
	const char *upload_data, size_t *upload_data_size, void **con_cls) {

	struct logfanoutd_state* plf_state = cls;
	struct vpath* pvpath;
	int ret;

	pvpath = init_vpath(plf_state->lookup, url);
	if(pvpath == NULL)
		return handle_error(connection, MHD_HTTP_NOT_FOUND);

	if(is_directory(pvpath)) {
		ret = handle_dir(connection, pvpath);
	} else {
		ret = handle_file(connection, pvpath);
	}

	free_vpath(pvpath);
	return ret;
}

static const char* x_inet_ntop(struct sockaddr* addr, char* buf, socklen_t size) {
	switch(addr->sa_family) {
	case AF_INET:
		return inet_ntop(AF_INET, &(((struct sockaddr_in*)addr)->sin_addr), buf, size);
	case AF_INET6:
		return inet_ntop(AF_INET6, &(((struct sockaddr_in6*)addr)->sin6_addr), buf, size);
	}
	return NULL;
}

#if MHD_VERSION > 0x00093000
static void* log_callback(void * cls, const char * uri, struct MHD_Connection *con) {
#else
static void* log_callback(void * cls, const char * uri) {
#endif // MHD_VERSION > 0x00093000
	char remote_addr[max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)] = {0};
	char time_str[32] = {0};
	time_t t;
	struct tm tm;
	struct tm *ptm = NULL;
	const union MHD_ConnectionInfo* info = NULL;

	t = time(NULL);
	ptm = localtime_r(&t, &tm);
#if MHD_VERSION > 0x00093000
	info = MHD_get_connection_info(con, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
#endif // MHD_VERSION > 0x00093000
	fprintf(stderr, "%s %s %s\n",
		(ptm && strftime(time_str, sizeof(time_str), "%a, %d %b %Y %T %z", &tm) ? time_str : "UNKNOWN"),
		(info && x_inet_ntop(info->client_addr, remote_addr, sizeof(remote_addr)) ? remote_addr : "UNKNOWN"),
		uri);
	return NULL;
}

static unsigned short logfanountd_listen_famity(struct logfanoutd_listen* listen) {
	struct sockaddr sa;
	socklen_t salen = sizeof(sa);
	switch (listen->type) {
		case LOGFANOUTD_LISTEN_SOCKADDR:
			return listen->value.sa.sa_family;
			break;
		case LOGFANOUTD_LISTEN_FD:
			if (getsockname(listen->value.fd, &sa, &salen) == 0) {
				return sa.sa_family;
			}
			break;
	};
	return -1;
}

struct logfanoutd_state* logfanoutd_start(struct logfanoutd_listen* listen, int verbose, int log, struct vpath_lookup* vpath_lookup) {
	struct logfanoutd_state* newstate = malloc(sizeof(struct logfanoutd_state));
	if(newstate == NULL) {
		return NULL;
	}

	newstate->lookup = vpath_lookup;

	newstate->MHD_Daemon = MHD_start_daemon(
		MHD_USE_SELECT_INTERNALLY |
			(verbose ? MHD_USE_DEBUG : MHD_NO_FLAG) |
			(logfanountd_listen_famity(listen) == AF_INET6 ? MHD_USE_IPv6 : MHD_NO_FLAG) |
			(listen->type == LOGFANOUTD_LISTEN_FD ? MHD_USE_PIPE_FOR_SHUTDOWN : MHD_NO_FLAG),
		0,
		NULL, // MHD_AcceptPolicyCallback apc
		NULL, // void *apc_cls 
		request_handler, // MHD_AccessHandlerCallback dh
		newstate,        // void *dh_cls
		MHD_OPTION_SOCK_ADDR, (listen->type == LOGFANOUTD_LISTEN_SOCKADDR ? &listen->value.sa : NULL),
		MHD_OPTION_LISTEN_SOCKET, (listen->type == LOGFANOUTD_LISTEN_FD ? listen->value.fd : -1),
		MHD_OPTION_UNESCAPE_CALLBACK, unescape_callback, NULL,
		MHD_OPTION_URI_LOG_CALLBACK, (log ? log_callback : NULL), NULL,
		MHD_OPTION_END);
	if(newstate->MHD_Daemon == NULL) {
		free(newstate);
		return NULL;
	}

	return newstate;
}
void logfanoutd_stop(struct logfanoutd_state* state) {
	MHD_stop_daemon(state->MHD_Daemon);
	free(state);
}
