#include <check.h>

#include <string.h>
#include <vpath.h>

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

Suite* vpath_suite(void) {
	Suite *s;
	TCase *tc_core;

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
