#ifndef _VPATH_H
#define _VPATH_H

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct vpath {
	char* vpath;
	char* ppath;
	struct stat stat;	
};

struct vpath_pair {
	char* vpath;
	char* ppath;
};

struct vpath_match {
	struct vpath_pair pair;
	size_t len;
};

struct vpath_lookup;

int remove_dot_segments(char* input, char* output);
int cmp_vpath_pair(const struct vpath_pair* x, const struct vpath_pair* y);

struct vpath* init_vpath(const struct vpath_lookup* lookup, const char* path);
void free_vpath(struct vpath* pvpath);
int is_directory(struct vpath* pvpath);

struct vpath_lookup* init_vpath_lookup(struct vpath_pair** pairs, size_t size);
struct vpath_match* match_vpath(const struct vpath_lookup* lookup, const char* vpath);
void free_vpath_lookup(struct vpath_lookup* lookup);

#endif // _VPATH_H

