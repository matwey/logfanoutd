#include <check.h>

#include <stdlib.h>
#include <string.h>
#include <list.h>

struct element {
	struct element* next;
	size_t data;
};

START_TEST (test_list_push1) {
	struct element list;
	struct element new;
	init_list(&list);
	list_push_back(&list, &new);
	ck_assert(list.next == &new);
}
END_TEST

START_TEST (test_list_push2) {
	struct element list;
	struct element new;
	struct element new2;
	init_list(&list);
	init_list(&new);
	init_list(&new2);
	list_push_back(&list, &new);
	list_push_back(&list, &new2);
	ck_assert(list.next == &new);
	ck_assert(new.next == &new2);
}
END_TEST

START_TEST (test_list_push3) {
	struct element* list = NULL;
	struct element new;
	init_list(&new);

	list = list_push_back(list, &new);
	ck_assert(list == &new);
	ck_assert(new.next == NULL);
}
END_TEST

START_TEST (test_list_size1) {
	struct element* empty = NULL;
	ck_assert_int_eq(list_size(empty), 0);
}
END_TEST

START_TEST (test_list_size2) {
	struct element list;
	init_list(&list);
	ck_assert_int_eq(list_size(&list), 1);
}
END_TEST

START_TEST (test_list_size3) {
	struct element list;
	struct element new;
	init_list(&list);
	init_list(&new);
	list_push_back(&list, &new);
	ck_assert_int_eq(list_size(&list), 2);
}
END_TEST

START_TEST (test_list_free1) {
	size_t i=0;
	struct element* list;
	list = (struct element*)init_list(malloc(sizeof(struct element)));
	for (i = 0; i < 5; ++i) {
		list_push_back(list, init_list(malloc(sizeof(struct element))));
	}
	free_list(list);
}
END_TEST

START_TEST (test_list_init1) {
	ck_assert(init_list(NULL) == NULL);
}
END_TEST
START_TEST (test_list_init2) {
	struct element list;
	init_list(&list);
	ck_assert(list.next == NULL);
}
END_TEST

Suite* list_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("list");

	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_list_push1);
	tcase_add_test(tc_core, test_list_push2);
	tcase_add_test(tc_core, test_list_push3);
	tcase_add_test(tc_core, test_list_size1);
	tcase_add_test(tc_core, test_list_size2);
	tcase_add_test(tc_core, test_list_size3);
	tcase_add_test(tc_core, test_list_free1);
	tcase_add_test(tc_core, test_list_init1);
	tcase_add_test(tc_core, test_list_init2);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = list_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
