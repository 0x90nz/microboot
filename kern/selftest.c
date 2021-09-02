#include "selftest.h"

#include <stdint.h>
#include "stdlib.h"
#include "alloc.h"
#include "htbl.h"

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

void test_htbl()
{
    htbl_t* table = htbl_create();
    htbl_put(table, "foo", "value 1");
    htbl_put(table, "bar", "value 2");

    const char* k1_res = htbl_get(table, "foo");
    const char* k2_res = htbl_get(table, "bar");

    debugf("k1_res: %p, k2_res: %p", k1_res, k2_res);

    if (k1_res == NULL || k2_res == NULL) {
        TEST_FAIL("htbl_get", "returned NULL mapping for present key");
        goto cleanup;
    }

    if (strcmp(k1_res, "value 1") != 0 || strcmp(k2_res, "value 2") != 0) {
        TEST_FAIL("htbl_get", "returned incorrect mapping for key");
        goto cleanup;
    }

    TEST_PASS("htbl");

cleanup:
    htbl_destroy(table);
}

void test_htbl_expand()
{
    htbl_t* table = htbl_create();

    // fill the table up to such a point that it will need to expand at least
    // once to fit everything
    for (int i = 0; i < 100; i++) {
        char buf[64];
        sprintf(buf, "key_%d", i);
        htbl_put(table, buf, (void*)i);
    }

    for (int i = 0; i < 100; i++) {
        char buf[64];
        sprintf(buf, "key_%d", i);
        void* v = htbl_get(table, buf);
        if ((int)v != i) {
            TEST_FAIL("htbl_expand", "key mapped to incorrect value");
            goto cleanup;
        }
    }

    TEST_PASS("htbl_expand");

cleanup:
    htbl_destroy(table);
}

void selftest(int argc, char** argv)
{
    test_memcpy();
    test_memset();
    test_kallocz();
    test_htbl();
    test_htbl_expand();
}

