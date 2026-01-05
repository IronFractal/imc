#ifndef IMC_LANG_VM_H
#define IMC_LANG_VM_H
struct imc_lang_vm;

struct imc_lang_vm *IMC_VM_new();

bool IMC_VM_run_src(struct imc_lang_vm *vm, const char *src);

bool IMC_VM_run_src_file(struct imc_lang_vm *vm, const char *filename);

bool IMC_VM_write_png(struct imc_lang_vm *vm, const char *filename);

bool IMC_VM_write_jpg(struct imc_lang_vm *vm, const char *filename);

void IMC_VM_free(struct imc_lang_vm *vm);

#endif
