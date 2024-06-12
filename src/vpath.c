#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vpath.h>

struct vpath_lookup {
	size_t alias_num;
	size_t prefix_len;
	size_t dest_len;
	char* prefix;
	char* dest;
	struct vpath_match* match;
};

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

struct vpath* init_vpath(const struct vpath_lookup* lookup, const char* path) {
	struct vpath_match* m = NULL;
	struct vpath* ret = NULL;

	m = match_vpath(lookup, path);
	if(m == NULL)
		goto ret;

	ret = malloc(sizeof(struct vpath));
	if(ret == NULL)
		goto ret;
	ret->vpath = strdup(path);
	if(ret->vpath == NULL) {
		goto ret_vpath;
	}
	ret->ppath = cat_path(m->pair.ppath, path + m->len);
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

int cmp_vpath_pair(const struct vpath_pair* x, const struct vpath_pair* y) {
	return strcmp(x->vpath, y->vpath);
}

static int init_vpath_lookup_cmp(const void* x, const void* y) {
	const struct vpath_pair* x1 = (const struct vpath_pair*)x;
	const struct vpath_pair* y1 = (const struct vpath_pair*)y;
	return cmp_vpath_pair(x1, y1);
}

struct vpath_lookup* init_vpath_lookup(struct vpath_pair* pairs, size_t size) {
	struct vpath_lookup* lookup = NULL;
	size_t i;

	qsort(pairs, size, sizeof(struct vpath_pair), &init_vpath_lookup_cmp);

	lookup = (struct vpath_lookup*)malloc(sizeof(struct vpath_lookup));
	if (lookup == NULL)
		goto ret;

	lookup->alias_num = size;

	lookup->match = (struct vpath_match*)calloc(size, sizeof(struct vpath_match));
	if (lookup->match == NULL)
		goto ret_match;

	lookup->prefix_len = 0;
	lookup->dest_len = 0;
	for (i = 0; i < size; ++i) {
		lookup->match[i].len = strlen(pairs[i].vpath);
		const size_t vpath_len = lookup->match[i].len + 1;
		const size_t ppath_len = strlen(pairs[i].ppath) + 1;
		lookup->prefix_len = (lookup->prefix_len < vpath_len ? vpath_len : lookup->prefix_len);
		lookup->dest_len = (lookup->dest_len < ppath_len ? ppath_len : lookup->dest_len);
	}

	lookup->prefix = (char*)calloc(size * (lookup->prefix_len + lookup->dest_len), sizeof(char));
	if (lookup->prefix == NULL)
		goto ret_prefix;
	lookup->dest = lookup->prefix + size * lookup->prefix_len;

	for (i = 0; i < size; ++i) {
		char* new_vpath = lookup->prefix + i * lookup->prefix_len;
		char* new_ppath = lookup->dest + i * lookup->dest_len;
		strcpy(new_vpath, pairs[i].vpath);
		strcpy(new_ppath, pairs[i].ppath);
		lookup->match[i].pair.vpath = new_vpath;
		lookup->match[i].pair.ppath = new_ppath;
	}

	return lookup;

ret_prefix:
	free(lookup->match);
ret_match:
	free(lookup);
ret:
	return NULL;
}

struct vpath_match* match_vpath(const struct vpath_lookup* lookup, const char* vpath) {
	struct vpath_match* matched = NULL;
	size_t i;
	size_t lower = 0;
	size_t upper = lookup->alias_num;

	for (i = 0; i < lookup->prefix_len && lower < upper; ++i) {
		if (lookup->prefix[i + lower * lookup->prefix_len] == '\0') {
			matched = lookup->match + lower;
			lower++;
		}

		for (; lookup->prefix[i + lower * lookup->prefix_len] != vpath[i] && lower < upper; ++lower);

		for (; lookup->prefix[i + (upper - 1) * lookup->prefix_len] != vpath[i] && lower < upper; --upper);

		if (vpath[i] == '\0') {
			break;
		}
	}

	return matched;
};

void free_vpath_lookup(struct vpath_lookup* lookup) {
	free(lookup->prefix);
	free(lookup->match);
	free(lookup);
}
