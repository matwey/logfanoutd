#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <range.h>

#define min(x,y) ((x)<(y)?(x):(y))

int x_strcasecmp(const char* s1, const char* s2) {
	while(*s1 != '\0' && toupper(*s1++) == toupper(*s2++));
	return toupper(*(s1-1)) - toupper(*(s2-1));
}
int x_strncasecmp(const char *s1, const char *s2, size_t n) {
	while(n-- && *s1 != '\0' && toupper(*s1++) == toupper(*s2++));
	return toupper(*(s1-1)) - toupper(*(s2-1));
}

char* parse_uint64(const char* value, uint64_t* x) {
	uintmax_t tmpint;
	char* endptr;

	tmpint = strtoumax(value, &endptr, 10);
	if(tmpint == 0 && endptr == value) // nothing found to convert
		return NULL;
	if(tmpint == UINTMAX_MAX && errno == ERANGE) // overflow occured
		return NULL;
	*x = tmpint;
	return endptr;
}
int parse_range(const char* value, struct range_set* pset) {
	static const char unit[] = "bytes=";
	clear_range(pset);

	if(x_strncasecmp(value, unit, 6) != 0) {
	// unsupported unit or wrong format
		return -1;
	}
	value += 6;

	if(*value == '-') {
	// suffix format
		value++;
		if((value = parse_uint64(value, &(pset->range.length))) == NULL)
			return -1;
		if(*value != '\0') // unsupported list format of wrong format
			return -1;
		pset->type = range_suffix;
	} else {
	// interval format
		if((value = parse_uint64(value, &(pset->range.interval.first))) == NULL)
			return -1;
		if(*value++ != '-') // wrong format
			return -1;
		pset->type = range_first;

		if(*value != '\0') {
			if((value = parse_uint64(value, &(pset->range.interval.last))) == NULL)
				return -1;
			if(*value != '\0') // wrong format
				return -1;
			pset->type = range_interval;
		}
	}

	return 0;
}
void clear_range(struct range_set* pset) {
	memset(pset, 0, sizeof(struct range_set));
	pset->type = range_unknown;
}

#if MHD_VERSION < 0x00097100
int range_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
#else
enum MHD_Result range_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
#endif // MHD_VERSION

	struct range_set* pset = cls;

	if(x_strcasecmp(key, MHD_HTTP_HEADER_RANGE) == 0) {
		if(parse_range(value, pset)<0)
			clear_range(pset);
		return MHD_NO;
	}
	return MHD_YES;
}
int is_valid_range(struct range_set* pset, uint64_t size) {
	switch(pset->type) {
	case range_suffix:
		if(pset->range.length == 0)
			return 0;
		break;
	case range_interval:
		// A byte-range-spec is invalid if the last-byte-pos value is present and less than the first-byte-pos.
		if(pset->range.interval.last < pset->range.interval.first)
			return 0;
	case range_first:
		if(pset->range.interval.first >= size)
			return 0;
	default:;
	}
	return 1;
}
void range_to_size_and_offset(struct range_set* pset, uint64_t size, uint64_t* newsize, uint64_t* offset) {
	switch(pset->type) {
	case range_interval:
		*offset  = pset->range.interval.first;
		*newsize = min(size,pset->range.interval.last+1) - *offset;
		break;
	case range_first:
		*offset  = pset->range.interval.first;
		*newsize = size - *offset;
		break;
	case range_suffix:
		*offset  = (size > pset->range.length ? size - pset->range.length : 0);
		*newsize = size - *offset;
		break;
	default:
		*offset  = 0;
		*newsize = size;
	}
}
