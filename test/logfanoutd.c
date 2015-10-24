#include <check.h>

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include <logfanoutd.h>

#define min(x,y) ((x)<(y)?(x):(y))

struct buffer {
	char* buf;
	size_t size;
	size_t capacity;
};

static struct buffer* pbuf;

static struct buffer* init_buffer(size_t capacity) {
	struct buffer* pbuf;
	pbuf = malloc(sizeof(struct buffer));
	if(pbuf == NULL)
		return NULL;
	pbuf->size = 0;
	pbuf->capacity = capacity;
	pbuf->buf = calloc(pbuf->capacity, sizeof(char));
	if(pbuf->buf == NULL) {
		free(pbuf);
		return NULL;
	}
	return pbuf;
}
static void free_buffer(struct buffer* pbuf) {
	free(pbuf->buf);
	free(pbuf);
}
static size_t fill_buffer(void* buf, size_t size, size_t nmemd, void* userdata) {
	struct buffer* pbuf = userdata;
	size_t newsize = min(pbuf->capacity - pbuf->size, size * nmemd);

	memcpy(pbuf->buf + pbuf->size, buf, newsize);
	pbuf->size += newsize;
	return newsize;
}

static size_t http_get_request_with_headers(const char* root_dir, const char* url, long* pretcode, struct buffer* pbuf, struct curl_slist *hdr) {
	struct logfanoutd_state* plf_state;
	CURL *c;
	CURLcode errornum;

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(7999);
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	plf_state = logfanoutd_start((struct sockaddr*)&sa, 1, 1, root_dir);
	if(plf_state == NULL)
		ck_abort_msg("Can not start daemon");
	c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_URL, url);
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, &fill_buffer);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, pbuf);
	curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdr);
	if((errornum = curl_easy_perform(c)) != CURLE_OK) {
		curl_easy_cleanup(c);
        	logfanoutd_stop(plf_state);
		ck_abort_msg(curl_easy_strerror(errornum));
	}
	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, pretcode);
	pbuf->buf[pbuf->size] = 0;

	curl_easy_cleanup(c);
        logfanoutd_stop(plf_state);
}
static size_t http_get_request(const char* root_dir, const char* url, long* pretcode, struct buffer* pbuf) {
	return http_get_request_with_headers(root_dir, url, pretcode, pbuf, NULL);
}
static size_t http_get_request_range(const char* root_dir, const char* url, long* pretcode, struct buffer* pbuf, const char* range_header) {
	struct curl_slist *hdr = NULL;
	hdr = curl_slist_append(hdr, range_header);
	return http_get_request_with_headers(root_dir, url, pretcode, pbuf, hdr);
}

void setup() {
	pbuf = init_buffer(1024);
	if(pbuf == NULL) {
		ck_abort_msg("Failed to setup buffer");
	}
}
void teardown() {
	free_buffer(pbuf);
}

START_TEST (test_empty_dir) {
	long retcode;
	http_get_request(PROJECT_ROOT "/test/data/empty", "http://127.0.0.1:7999/", &retcode, pbuf);
	ck_assert_str_eq(pbuf->buf, "{}");
	ck_assert_int_eq(retcode, 200);
}
END_TEST
START_TEST (test_empty_dir_404) {
	long retcode;
	http_get_request(PROJECT_ROOT "/test/data/empty", "http://127.0.0.1:7999/not_found", &retcode, pbuf);
	ck_assert_int_eq(retcode, 404);
}
END_TEST
START_TEST (test_singlefile_dir) {
	long retcode;
	http_get_request(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/", &retcode, pbuf);
	ck_assert_str_eq(pbuf->buf, "{\"file\":{}}");
	ck_assert_int_eq(retcode, 200);
}
END_TEST
START_TEST (test_singlefile_file) {
	static char expected[257];
	size_t i = 0;
	for(i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i) expected[i] = i;
	long retcode;
	http_get_request(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/file", &retcode, pbuf);
	ck_assert_str_eq(pbuf->buf+1, expected+1);
	ck_assert_int_eq(retcode, 200);
}
END_TEST
START_TEST (test_singlefile_file_range1) {
	static char expected[3] = {0xFE, 0xFF, 0x00};
	long retcode;
	http_get_request_range(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/file", &retcode, pbuf, "Range: bytes=-2");
	ck_assert_str_eq(pbuf->buf, expected);
	ck_assert_int_eq(pbuf->size, 2);
	ck_assert_int_eq(retcode, 206);
}
END_TEST
START_TEST (test_singlefile_file_range2) {
	static char expected[3] = {0x01, 0x02, 0x00};
	long retcode;
	http_get_request_range(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/file", &retcode, pbuf, "Range: bytes=1-2");
	ck_assert_str_eq(pbuf->buf, expected);
	ck_assert_int_eq(pbuf->size, 2);
	ck_assert_int_eq(retcode, 206);
}
END_TEST
START_TEST (test_singlefile_file_range3) {
	static char expected[3] = {0xFE, 0xFF, 0x00};
	long retcode;
	http_get_request_range(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/file", &retcode, pbuf, "Range: bytes=254-");
	ck_assert_str_eq(pbuf->buf, expected);
	ck_assert_int_eq(pbuf->size, 2);
	ck_assert_int_eq(retcode, 206);
}
END_TEST
START_TEST (test_singlefile_file_range_case) {
	static char expected[3] = {0x01, 0x02, 0x00};
	long retcode;
	http_get_request_range(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/file", &retcode, pbuf, "range: bytes=1-2");
	ck_assert_str_eq(pbuf->buf, expected);
	ck_assert_int_eq(pbuf->size, 2);
	ck_assert_int_eq(retcode, 206);
}
END_TEST
START_TEST (test_singlefile_file_range_case_64) {
	long retcode;
	http_get_request_range(PROJECT_ROOT "/test/data/single", "http://127.0.0.1:7999/file", &retcode, pbuf, "range: bytes=36893488147419103232-");
	ck_assert_int_eq(pbuf->size, 0);
#ifdef MHD_HAS_RFFO64
	ck_assert_int_eq(retcode, 206);
#else
	ck_assert_int_eq(retcode, 500);
#endif
}
END_TEST

Suite* logfanoutd_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("logfanoutd");

	tc_core = tcase_create("Core");

	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_empty_dir);
	tcase_add_test(tc_core, test_empty_dir_404);
	tcase_add_test(tc_core, test_singlefile_dir);
	tcase_add_test(tc_core, test_singlefile_file);
	tcase_add_test(tc_core, test_singlefile_file_range1);
	tcase_add_test(tc_core, test_singlefile_file_range2);
	tcase_add_test(tc_core, test_singlefile_file_range3);
	tcase_add_test(tc_core, test_singlefile_file_range_case);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = logfanoutd_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
