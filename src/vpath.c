#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vpath.h>

static char* cat_path(const char* base, const char* path) {
	size_t baselen = strlen(base);
	size_t pathlen = strlen(path);
	char* newpath = calloc(baselen + pathlen + 2, sizeof(char));
	if(newpath == NULL)
		return NULL;
	strcpy(newpath, base);
	newpath[baselen] = '/';
	strcpy(newpath + baselen + 1, path);
	return newpath;
}

#define RDS_DO_A(x) {input+=(x); continue;}
#define RDS_DO_B(x) {input+=(x)-1; *input='/'; continue;}
#define RDS_DO_C(x) {input+=(x)-1; *input='/'; while(output_end != output && *(--output_end) != '/') { \
	}; continue;}
#define RDS_DO_D(x) {input+=(x); continue;}
#define RDS_DO_E { do { \
	*(output_end++) = *(input++); \
	} while(*input != '\0' && *input != '/'); continue; }
int remove_dot_segments(char* input, char* output) {
// The following code is according to
// RFC 3986 Section 5.2.4. Remove Dot Segments
	char* output_end = output;
	while (*input != '\0') {
		switch(input[0]) {
		case '.':
			switch(input[1]) {
			case '\0': RDS_DO_D(1); // .$
			case '.':
				switch(input[2]) {
					case '\0':RDS_DO_D(2); // ..$
					case '/': RDS_DO_A(3); // ../
					default:  RDS_DO_E;    // ..X
				}
				break;
			case '/': RDS_DO_A(2);  // ./
			default:  RDS_DO_E;     //.X
			}
			break;
		case '/':
			switch(input[1]) {
			case '\0': RDS_DO_E;    // /$
			case '.':
				switch(input[2]) {
				case '\0': RDS_DO_B(2); // /.$
				case '.':
					switch(input[3]) {
					case '\0':RDS_DO_C(3); // /..$
					case '/': RDS_DO_C(4); // /../
					default:  RDS_DO_E;    // /..X
					}
					break;
				case '/': RDS_DO_B(3);  // /./
				default:  RDS_DO_E;     // /.X
				}
				break;
			default: RDS_DO_E;     // /X
			}
			break;
		default:  RDS_DO_E;
		}
	}

	*output_end = '\0';
	return output_end - output;
}
#undef RDS_DO_A
#undef RDS_DO_B
#undef RDS_DO_C
#undef RDS_DO_D
#undef RDS_DO_E

struct vpath* init_vpath(const char* root_dir, const char* path) {
	struct vpath* ret = malloc(sizeof(struct vpath));
	if(ret == NULL)
		goto ret;
	ret->vpath = strdup(path);
	if(ret->vpath == NULL) {
		goto ret_vpath;
	}
	ret->ppath = cat_path(root_dir, path);
	if(ret->ppath == NULL) {
		goto ret_ppath;
	}
	if(stat(ret->ppath, &ret->stat)<0) {
		goto ret_stat;
	}
	return ret;

ret_stat:
	free(ret->ppath);
ret_ppath:
	free(ret->vpath);
ret_vpath:
	free(ret);
ret:
	return NULL;
}
void free_vpath(struct vpath* pvpath) {
	free(pvpath->ppath);
	free(pvpath->vpath);
	free(pvpath);
}
int is_directory(struct vpath* pvpath) {
	return S_ISDIR(pvpath->stat.st_mode);
}
