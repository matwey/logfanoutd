#include <stdlib.h>

#include <list.h>

struct list_element {
	struct list_element* next;
};

void* init_list(void* list) {
	struct list_element* e = (struct list_element*)list;

	if (e != NULL) {
		e->next = NULL;
	}
	return list;
}

size_t list_size(void* list) {
	struct list_element* e = (struct list_element*)list;
	size_t size = 0;

	while (e != NULL) {
		size++;
		e = e->next;
	}

	return size;
}

void* list_push_back(void* list, void* element) {
	struct list_element* l = (struct list_element*)list;
	struct list_element* e = (struct list_element*)element;

	if (l == NULL) {
		return e;
	}
	while (l->next != NULL) {
		l = l->next;
	}

	l->next = e;
	return l;
}

void free_list(void* list) {
	struct list_element* l = (struct list_element*)list;

	while (l != NULL) {
		struct list_element* e = l;
		l = l->next;
		free(e);
	}
}
