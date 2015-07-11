#include <check.h>

#include <string.h>
#include <range.h>

START_TEST (test_range_parse1) {
	static const char value[] = "robots=";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_parse2) {
	static const char value[] = "bytes=";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_parse3) {
	static const char value[] = "bytes=-123";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), 0);
	ck_assert_int_eq(rs.range.length, 123);
	ck_assert_int_eq(rs.type, range_suffix);
}
END_TEST
START_TEST (test_range_parse4) {
	static const char value[] = "bytes=-123,";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_parse5) {
	static const char value[] = "bytes=abc";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_parse6) {
	static const char value[] = "bytes=123";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_parse7) {
	static const char value[] = "bytes=123-";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), 0);
	ck_assert_int_eq(rs.range.interval.first, 123);
	ck_assert_int_eq(rs.type, range_first);
}
END_TEST
START_TEST (test_range_parse8) {
	static const char value[] = "bytes=123-,";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_parse9) {
	static const char value[] = "bytes=123-124";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), 0);
	ck_assert_int_eq(rs.range.interval.first, 123);
	ck_assert_int_eq(rs.range.interval.last, 124);
	ck_assert_int_eq(rs.type, range_interval);
}
END_TEST
START_TEST (test_range_parse10) {
	static const char value[] = "bytes=123-124,";
	struct range_set rs;
	ck_assert_int_eq(parse_range(value,&rs), -1);
}
END_TEST
START_TEST (test_range_validation1) {
	struct range_set rs;
	rs.type = range_unknown; // unknown is always valid
	ck_assert_int_eq(is_valid_range(&rs, 500), 1);
}
END_TEST
START_TEST (test_range_validation2) {
	struct range_set rs;
	rs.type = range_suffix;
	rs.range.length = 0; // invalid
	ck_assert_int_eq(is_valid_range(&rs, 500), 0);
}
END_TEST
START_TEST (test_range_validation3) {
	struct range_set rs;
	rs.type = range_suffix;
	rs.range.length = 1; // valid!
	ck_assert_int_eq(is_valid_range(&rs, 500), 1);
}
END_TEST
START_TEST (test_range_validation4) {
	struct range_set rs;
	rs.type = range_first;
	rs.range.interval.first = 501; // invalid
	ck_assert_int_eq(is_valid_range(&rs, 500), 0);
}
END_TEST
START_TEST (test_range_validation5) {
	struct range_set rs;
	rs.type = range_first;
	rs.range.interval.first = 499; // valid!
	ck_assert_int_eq(is_valid_range(&rs, 500), 1);
}
END_TEST
START_TEST (test_range_validation6) {
	struct range_set rs;
	rs.type = range_interval;
	rs.range.interval.first = 501; //invalid
	rs.range.interval.last  = 502;
	ck_assert_int_eq(is_valid_range(&rs, 500), 0);
}
END_TEST
START_TEST (test_range_validation7) {
	struct range_set rs;
	rs.type = range_interval;
	rs.range.interval.first = 480;
	rs.range.interval.last  = 479;
	ck_assert_int_eq(is_valid_range(&rs, 500), 0);
}
END_TEST
START_TEST (test_range_validation8) {
	struct range_set rs;
	rs.type = range_interval;
	rs.range.interval.first = 480;
	rs.range.interval.last  = 481;
	ck_assert_int_eq(is_valid_range(&rs, 500), 1);
}
END_TEST
START_TEST (test_range_size_offset1) {
	uint64_t newsize;
	off_t offset;
	struct range_set rs;
	rs.type = range_suffix;
	rs.range.length = 1;
	range_to_size_and_offset(&rs, 500, &newsize, &offset);
	ck_assert_int_eq(newsize, 1);
	ck_assert_int_eq(offset, 499);
}
END_TEST
START_TEST (test_range_size_offset2) {
	uint64_t newsize;
	off_t offset;
	struct range_set rs;
	rs.type = range_suffix;
	rs.range.length = 520;
	range_to_size_and_offset(&rs, 500, &newsize, &offset);
	ck_assert_int_eq(newsize, 500);
	ck_assert_int_eq(offset, 0);
}
END_TEST
START_TEST (test_range_size_offset3) {
	uint64_t newsize;
	off_t offset;
	struct range_set rs;
	rs.type = range_first;
	rs.range.interval.first = 400;
	range_to_size_and_offset(&rs, 500, &newsize, &offset);
	ck_assert_int_eq(newsize, 100);
	ck_assert_int_eq(offset, 400);
}
END_TEST
START_TEST (test_range_size_offset4) {
	uint64_t newsize;
	off_t offset;
	struct range_set rs;
	rs.type = range_interval;
	rs.range.interval.first = 400;
	rs.range.interval.last = 550;
	range_to_size_and_offset(&rs, 500, &newsize, &offset);
	ck_assert_int_eq(newsize, 100);
	ck_assert_int_eq(offset, 400);
}
END_TEST
START_TEST (test_range_size_offset5) {
	uint64_t newsize;
	off_t offset;
	struct range_set rs;
	rs.type = range_interval;
	rs.range.interval.first = 400;
	rs.range.interval.last = 449;
	range_to_size_and_offset(&rs, 500, &newsize, &offset);
	ck_assert_int_eq(newsize, 50);
	ck_assert_int_eq(offset, 400);
}
END_TEST
START_TEST (test_range_size_offset6) {
	uint64_t newsize;
	off_t offset;
	struct range_set rs;
	rs.type = range_interval;
	rs.range.interval.first = 400;
	rs.range.interval.last = 400;
	range_to_size_and_offset(&rs, 500, &newsize, &offset);
	ck_assert_int_eq(newsize, 1);
	ck_assert_int_eq(offset, 400);
}
END_TEST

Suite* range_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("range");

	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_range_parse1);
	tcase_add_test(tc_core, test_range_parse2);
	tcase_add_test(tc_core, test_range_parse3);
	tcase_add_test(tc_core, test_range_parse4);
	tcase_add_test(tc_core, test_range_parse5);
	tcase_add_test(tc_core, test_range_parse6);
	tcase_add_test(tc_core, test_range_parse7);
	tcase_add_test(tc_core, test_range_parse8);
	tcase_add_test(tc_core, test_range_parse9);
	tcase_add_test(tc_core, test_range_parse10);
	tcase_add_test(tc_core, test_range_validation1);
	tcase_add_test(tc_core, test_range_validation2);
	tcase_add_test(tc_core, test_range_validation3);
	tcase_add_test(tc_core, test_range_validation4);
	tcase_add_test(tc_core, test_range_validation5);
	tcase_add_test(tc_core, test_range_validation6);
	tcase_add_test(tc_core, test_range_validation7);
	tcase_add_test(tc_core, test_range_validation8);
	tcase_add_test(tc_core, test_range_size_offset1);
	tcase_add_test(tc_core, test_range_size_offset2);
	tcase_add_test(tc_core, test_range_size_offset3);
	tcase_add_test(tc_core, test_range_size_offset4);
	tcase_add_test(tc_core, test_range_size_offset5);
	tcase_add_test(tc_core, test_range_size_offset6);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = range_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
