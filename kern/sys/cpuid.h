#pragma once

#include <stdint.h>

void cpuid_get_vendor(char* buf);
int cpuid_check_feature(const char* feature);
int cpuid_get_max_id();
void cpuid_verbose_dump();
void cpuid(int i, uint32_t* regs);