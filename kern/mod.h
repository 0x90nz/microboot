#pragma once

void mod_init();
void mod_load(void* module);
void mod_list();
void mod_sym_list();
void mod_symtab_add(void* base, void* symtab, size_t szsymtab);
void mod_ksymtab_add(void* symtab, size_t szsymtab);