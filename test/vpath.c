#include <check.h>

#include <string.h>
#include <stdlib.h>
#include <vpath.h>

struct vpath_lookup* lookup;

void match_setup() {
	struct vpath_pair** pairs = NULL;
	struct vpath_pair pair[] = {
		{"/alias/sub", "/subalias"},
		{"/", "/root"},
		{"/alias", "/alias"}
	};
	size_t i;
	size_t size = sizeof(pair) / sizeof(struct vpath_pair);

	pairs = (struct vpath_pair**)calloc(size, sizeof(struct vpath_pair*));
	for (i = 0; i < size; i++) {
		pairs[i] = &pair[i];
	}

	lookup = init_vpath_lookup(pairs, size);

	free(pairs);
}
void match_teardown() {
	free_vpath_lookup(lookup);
}

#define DO_VPATH_RDS_TEST(N, I, E) \
START_TEST (test_vpath_rds ## N) { \
	char input[] = I; \
	static const char expected[] = E; \
	int newlen = remove_dot_segments(input, input); \
	ck_assert_str_eq(input, expected); \
	ck_assert_int_eq(newlen, strlen(expected)); \
} \
END_TEST

DO_VPATH_RDS_TEST(1, "/a/b/c/./../../g", "/a/g")
DO_VPATH_RDS_TEST(2, "mid/content=5/../6", "mid/6")
DO_VPATH_RDS_TEST(3, "../../../../g", "g")
DO_VPATH_RDS_TEST(4, "/./g", "/g")
DO_VPATH_RDS_TEST(5, "/../g", "/g")
DO_VPATH_RDS_TEST(6, "g.", "g.")
DO_VPATH_RDS_TEST(7, ".g", ".g")
DO_VPATH_RDS_TEST(8, "..g", "..g")
DO_VPATH_RDS_TEST(9, "g..", "g..")
DO_VPATH_RDS_TEST(10, "./../g", "g")
DO_VPATH_RDS_TEST(11, "./g/.", "g/")
DO_VPATH_RDS_TEST(12, "./g/", "g/")
DO_VPATH_RDS_TEST(13, "g/./h", "g/h")
DO_VPATH_RDS_TEST(14, "g/../h", "/h")

#define DO_VPATH_MATCH_TEST(N, I, E) \
START_TEST (test_vpath_match ## N) { \
	static const char input[] = I; \
	static const char expected[] = E; \
	struct vpath_match* m = match_vpath(lookup, input); \
	ck_assert(m != NULL); \
	ck_assert_str_eq(m->pair.ppath, expected); \
	ck_assert_int_eq(m->len, strlen(m->pair.vpath)); \
} \
END_TEST

DO_VPATH_MATCH_TEST(1, "/abc", "/root")
DO_VPATH_MATCH_TEST(2, "/", "/root")
DO_VPATH_MATCH_TEST(3, "/alias", "/alias")
DO_VPATH_MATCH_TEST(4, "/alias/", "/alias")
DO_VPATH_MATCH_TEST(5, "/alia", "/root")
// The following is expected behaviour
DO_VPATH_MATCH_TEST(6, "/alias2", "/alias")
DO_VPATH_MATCH_TEST(7, "/alias/abc", "/alias")
DO_VPATH_MATCH_TEST(8, "/alias/sub", "/subalias")
DO_VPATH_MATCH_TEST(9, "/alias/sub/", "/subalias")
DO_VPATH_MATCH_TEST(10, "/alias/su", "/alias")
DO_VPATH_MATCH_TEST(11, "/alias/sub/abc", "/subalias")

START_TEST (test_vpath_notmatch1) {
	struct vpath_match* m = match_vpath(lookup, "abc");
	ck_assert(m == NULL);
}
END_TEST

Suite* vpath_suite(void) {
	Suite *s;
	TCase *tc_core;
	TCase *tc_match;

	s = suite_create("vpath");

	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_vpath_rds1);
	tcase_add_test(tc_core, test_vpath_rds2);
	tcase_add_test(tc_core, test_vpath_rds3);
	tcase_add_test(tc_core, test_vpath_rds4);
	tcase_add_test(tc_core, test_vpath_rds5);
	tcase_add_test(tc_core, test_vpath_rds6);
	tcase_add_test(tc_core, test_vpath_rds7);
	tcase_add_test(tc_core, test_vpath_rds8);
	tcase_add_test(tc_core, test_vpath_rds9);
	tcase_add_test(tc_core, test_vpath_rds10);
	tcase_add_test(tc_core, test_vpath_rds11);
	tcase_add_test(tc_core, test_vpath_rds12);
	tcase_add_test(tc_core, test_vpath_rds13);
	tcase_add_test(tc_core, test_vpath_rds14);
	suite_add_tcase(s, tc_core);

	tc_match = tcase_create("Match");

	tcase_add_unchecked_fixture(tc_match, match_setup, match_teardown);
	tcase_add_test(tc_match, test_vpath_match1);
	tcase_add_test(tc_match, test_vpath_match2);
	tcase_add_test(tc_match, test_vpath_match3);
	tcase_add_test(tc_match, test_vpath_match4);
	tcase_add_test(tc_match, test_vpath_match5);
	tcase_add_test(tc_match, test_vpath_match6);
	tcase_add_test(tc_match, test_vpath_match7);
	tcase_add_test(tc_match, test_vpath_match8);
	tcase_add_test(tc_match, test_vpath_match9);
	tcase_add_test(tc_match, test_vpath_match10);
	tcase_add_test(tc_match, test_vpath_match11);
	tcase_add_test(tc_match, test_vpath_notmatch1);
	suite_add_tcase(s, tc_match);

	return s;
}

int main(void) {
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = vpath_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
