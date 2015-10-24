#ifndef _RANGE_H
#define _RANGE_H

#include <inttypes.h>
#include <microhttpd.h>

struct range_set {
	enum { range_unknown, range_suffix, range_first, range_interval } type;
	union {
		struct {
			uint64_t first;
			uint64_t last;
		} interval;
		uint64_t length;
	} range;
};

int range_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
int parse_range(const char* value, struct range_set* pset);
void clear_range(struct range_set* pset);
int is_valid_range(struct range_set* pset, uint64_t size);
void range_to_size_and_offset(struct range_set* pset, uint64_t size, uint64_t* newsize, uint64_t* offset);

#endif // _RANGE_H
