#ifndef _LOONGSTUB_KERNEL_H_
#define _LOONGSTUB_KERNEL_H_

#include <grub/types.h>

void save_context(grub_addr_t ctx, grub_addr_t start_image, grub_addr_t image_handle, 
  grub_addr_t efi_system_table, grub_addr_t cmd_line_ptr);
void trap_table_entry1();
void trap_table_entry2();
void trap_table_entry3();
void trap_table_entry4();
void trap_table_entry5();
void trap_table_entry6();
void trap_table_entry7();
void trap_table_entry8();
void trap_table_entry9();
void trap_table_entry10();
void trap_table_entry11();
void trap_table_entry12();
void trap_table_entry13();
void trap_table_entry14();
void trap_table_entry15();
void trap_table_entry16();
void trap_table_entry17();
void trap_table_entry18();
void trap_table_entry19();
void trap_table_entry20();
void trap_table_entry21();
void trap_table_entry22();
void trap_table_entry23();
void trap_table_entry24();
void trap_table_entry25(); // invalid

void trap_table_entry64();
void trap_table_entry65();
void trap_table_entry66();
void trap_table_entry67();
void trap_table_entry68();
void trap_table_entry69();
void trap_table_entry70();
void trap_table_entry71();
void trap_table_entry72();
void trap_table_entry73();
void trap_table_entry74();
void trap_table_entry75();
void trap_table_entry76();
void trap_table_entry77(); // invalid

#endif // _LOONGSTUB_KERNEL_H_