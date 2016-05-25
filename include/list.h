#ifndef _LIST_H
#define _LIST_H

void* init_list(void* list);
size_t list_size(void* list);
void* list_push_back(void* list, void* element);
void free_list(void* list);

#endif // _LIST_H
