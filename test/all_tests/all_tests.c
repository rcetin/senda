#include <stdio.h>
#include <setjmp.h>
#include <cmocka.h>

#include "all_tests.h"

static void dummy_test(void **state)
{
    (void) state; /* unused */

	assert_int_equal(0, 0);
}

static void test_readfromfile(void **state)
{
	(void) state; /* unused */
	const char *filename = "/home/rcetin/workspace/repos/senda/test/read_from_file.txt";

	const char *buf = read_data_from_file(filename);
	printf("buffer=%p\n\n", buf);

	assert_non_null(buf);
}

int main(int argc, char const *argv[])
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(dummy_test),
		cmocka_unit_test(test_readfromfile),

	};
	return cmocka_run_group_tests(tests, NULL, NULL);
    return 0;
}
