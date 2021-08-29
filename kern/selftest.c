#include "selftest.h"

#include <stdint.h>
#include "stdlib.h"
#include "alloc.h"

#define TEST_LOG(msg) debug(msg); printf("%s\n", msg);
#define TEST_LOGF(msg, ...) debugf(msg, __VA_ARGS__); printf(msg "\n", __VA_ARGS__);
#define TEST_PASS(test) TEST_LOG("[OK]   " test);
#define TEST_FAIL(test, reason) TEST_LOG("[FAIL] " test ", reason: " reason);

void test_memcpy()
{
    uint8_t* src = kalloc(1024);
    uint8_t* dst = kalloc(2048);

    // fill src with a pattern
    for (int i = 0; i < 1024; i++) {
        src[i] = i % 2 == 0 ? 0x55 : 0xaa;
    }

    dst[1024] = 0x12; // canary to check for overwrite
    dst[2000] = 0x34;

    memcpy(dst, src, 1024);

    // check that the pattern was transferred
    for (int i = 0; i < 1024; i++) {
        if (dst[i] != (i % 2 == 0 ? 0x55 : 0xaa)) {
            TEST_FAIL("memcpy", "incorrect value");
            goto cleanup;
        }
    }

    // check that the canary is still in tact (we didn't overrun the buffer)
    if (dst[1024] != 0x12 || dst[2000] != 0x34) {
        TEST_FAIL("memcpy", "canary overwritten");
    } else {
        TEST_PASS("memcpy");
    }

cleanup:
    kfree(src);
    kfree(dst);
}

void test_memset()
{
    uint8_t* buf = kalloc(2048);

    // canary to check for overwrite
    buf[1024] = 0x12;
    buf[2000] = 0x34;

    memset(buf, 0x55, 1024);

    for (int i = 0; i < 1024; i++) {
        if (buf[i] != 0x55) {
            TEST_FAIL("memset", "incorrect value");
            goto cleanup;
        }
    }

    if (buf[1024] != 0x12 || buf[2000] != 0x34) {
        TEST_FAIL("memset", "canary overwritten");
    } else {
        TEST_PASS("memset");
    }

cleanup:
    kfree(buf);
}

void test_kallocz()
{
    uint8_t* buf = kallocz(1024);

    for (int i = 0; i < 1024; i++) {
        if (buf[i] != 0) {
            TEST_FAIL("kallocz", "memory not zero");
            goto cleanup;
        }
    }

    memset(buf, 0x55, 1024);
    kfree(buf);

    // try reallocating the freed buffer, it's likely to get the same one so if
    // by fluke it was zero on the first try but it wasn't /zeroed/ out, then
    // this should hopefully catch it
    buf = kallocz(1024);
    for (int i = 0; i < 1024; i++) {
        if (buf[i] != 0) {
            TEST_FAIL("kallocz", "reallocated memory not zero");
            goto cleanup;
        }
    }

    TEST_PASS("kallocz");

cleanup:
    kfree(buf);
}

void selftest(int argc, char** argv)
{
    test_memcpy();
    test_memset();
    test_kallocz();
}

