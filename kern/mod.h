#pragma once

void mod_init();
void mod_load(void* module);
void mod_list();
void mod_sym_list();
void mod_symtab_add(void* base, void* symtab, size_t szsymtab);
void mod_ksymtab_early_init(void* symtab, size_t symtab_size);
void mod_ksymtab_add(void* symtab, size_t szsymtab);
void* mod_sym_get(const char* name);