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

int remove_dot_segments(char* input, char* output);

struct vpath* init_vpath(const char* root_dir, const char* path);
void free_vpath(struct vpath* pvpath);
int is_directory(struct vpath* pvpath);

#endif // _VPATH_H

