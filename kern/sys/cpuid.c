/**
 * @file cpuid.c
 * @brief functions to access processor specific information obtained through
 * the cpuid instruction
 */

#include "cpuid.h"
#include "../stdlib.h"

static const char* cpuid_features[] = {
    "fpu",
    "vme",
    "de",
    "pse",
    "tsc",
    "msr",
    "pae",
    "mce",
    "cx8",
    "apic",
    "!res!",
    "sep",
    "mtrr",
    "pge",
    "mca",
    "cmov",
    "pat",
    "pse-36",
    "psn",
    "clfsh",
    "!res!",
    "ds",
    "acpi",
    "mmx",
    "fxsr",
    "sse",
    "sse2",
    "ss",
    "htt",
    "tm",
    "ia64",
    "pbe"
};

/**
 * @brief Get the vendor string of this processor
 * 
 * @param buf a buffer into which to place the vendor string. must be at least
 * 13 bytes large
 */
void cpuid_get_vendor(char* buf)
{
    uint32_t res[4];
    cpuid(0, res);
    memcpy(buf, res + 1, 4);        // ebx
    memcpy(buf + 4, res + 3, 4);    // edx
    memcpy(buf + 8, res + 2, 4);    // ecx
    buf[12] = '\0';
}

/**
 * @brief Check for a given feature
 * 
 * @param feature the feature to check for (e.g. "mmx")
 * @return int non zero if present, zero if not present
 */
int cpuid_check_feature(const char* feature)
{
    int feature_bit = -1;
    for (int i = 0; i < 32; i++) {
        if (strcmp(feature, cpuid_features[i]) == 0) {
            feature_bit = i;
        }
    }

    if (feature_bit == -1)
        return -1;

    uint32_t res[4];
    cpuid(1, res);

    return (1 << feature_bit) & res[3];
}

/**
 * @brief Get the maximum supported cpuid on this processor
 * 
 * @return int the max cpuid
 */
int cpuid_get_max_id()
{
    uint32_t res[4];
    cpuid(0, res);
    return res[0];
}

/**
 * @brief Dump the full info obtained from cpuinfo
 * 
 */
void cpuid_verbose_dump()
{
    uint32_t res[4];
    cpuid(1, res);
    printf("stepping id    %d\n", res[0] & 0x0f);
    printf("model          %d\n", (res[0] >> 4) & 0x0f);
    printf("family id      %d\n", (res[0] >> 8) & 0x0f);
    printf("processor type %d\n", (res[0] >> 12) & 0x03);
    printf("ext. model     %d\n", (res[0] >> 16) & 0x0f);
    printf("ext. family id %d\n", (res[0] >> 20) & 0xff);

    for (int i = 0; i < 32; i++) {
        if (res[3] & (1 << i)) {
            printf("%s ", cpuid_features[i]);
        }
    }
    printf("\n");
}

/**
 * @brief Execute the cpuid instruction and return the resultant registers
 * 
 * @param i the value of the register eax for cpuid
 * @param regs 4 32 bit integers, eax, ebx, ecx, and edx
 */
void cpuid(int i, uint32_t* regs)
{
    asm volatile(
        "cpuid" 
        : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
        : "a" (i), "c" (0)
    );
}
