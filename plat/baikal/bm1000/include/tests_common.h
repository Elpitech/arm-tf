#ifndef __TESTS_COMMON_H__
#define __TESTS_COMMON_H__

#define TEST_STATUS_DONE		(1ULL << 63)

#define TEST_PRINT(...) printf("TEST: " __VA_ARGS__)

typedef void (*bl1_test_run) (void) ;

typedef struct _bl1_test {
	const char*	desc;
	uint64_t	status;
	bl1_test_run	run;
} bl1_test;

#define TEST_DEC(name, run, desc) \
	bl1_test _test_##name __section("bl1_test_desc")= { desc, 0, run }

void dump32(void* ptr, size_t size);
#endif
