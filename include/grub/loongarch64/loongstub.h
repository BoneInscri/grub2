#ifndef _LOONGSTUB_H_
#define _LOONGSTUB_H_

#include <grub/types.h>
#include <grub/file.h>
#include <grub/loongarch64/loongstub_kernel.h>

// ======================================================================================
// ============================ boot stub for loongvisor ================================
// ======================================================================================
struct newc_head
{
  char magic[6];
  char ino[8];
  char mode[8];
  char uid[8];
  char gid[8];
  char nlink[8];
  char mtime[8];
  char filesize[8];
  char devmajor[8];
  char devminor[8];
  char rdevmajor[8];
  char rdevminor[8];
  char namesize[8];
  char check[8];
} GRUB_PACKED;

#define loop while(1) {}

#define NAME_MAX 20
#define BOOT_TYPE_LOONGVISOR 1
#define BOOT_TYPE_ROOT_LINUX 0 
typedef struct {
	char name[NAME_MAX];
	void * image_addr;
	grub_size_t image_size;
	char* image_args;
	grub_uint32_t image_cmdline_size;
	grub_addr_t image_cmdline_addr;
	int boot_type;
	grub_addr_t image_load_addr;
	grub_uint64_t image_load_size;
	grub_addr_t boot_ctx_addr;
	grub_uint64_t boot_ctx_load_size;
	grub_addr_t image_trap_vector_addr;
} loongstub_boot_struct;

struct dir
{
  char *name;
  struct dir *next;
  struct dir *child;
};

struct grub_root_initrd_component
{
  grub_file_t file;
  char *newc_name;
  grub_off_t size;
};

struct grub_root_initrd_context
{
  int nfiles;
  struct grub_root_initrd_component *components;
  grub_size_t size;
};

struct boot_context {
	unsigned long ra;   // return address，0
	unsigned long sp;   // stack pointer，8
	unsigned long tp;   // threads pointer，16
	unsigned long s0;   
	unsigned long s1;   
	unsigned long s2;   
	unsigned long s3;   
	unsigned long s4;   
	unsigned long s5;   
	unsigned long s6;   
	unsigned long s7;   
	unsigned long s8;   
	unsigned long fp;   
	unsigned long start_image;  
	unsigned long image_handle;
	// unsigned long t0;
	unsigned long efi_system_table;
	// unsigned long t1;
	unsigned long cmd_line_ptr;
	unsigned long t2;
	unsigned long t3;
	unsigned long t4;
	unsigned long t5;
	unsigned long t6;
	unsigned long t7;
	unsigned long t8;
	unsigned long crmd;
	unsigned long prmd;
	unsigned long euen;
	unsigned long misc;
	unsigned long ecfg;
	unsigned long estat;
	unsigned long era;
	unsigned long badv;
	unsigned long badi;
	unsigned long eentry;
	unsigned long tlbidx;
	unsigned long tlbehi;
	unsigned long tlbelo0;
	unsigned long tlbelo1;
	unsigned long asid;
	unsigned long pgdl;
	unsigned long pgdh;
	unsigned long pwcl;
	unsigned long pwch;
	unsigned long stlbps;
	unsigned long rvacfg;
	unsigned long cpuid;
	unsigned long prcfg1;
	unsigned long prcfg2;
	unsigned long prcfg3;
	unsigned long save0;
	unsigned long save1;
	unsigned long save2;
	unsigned long save3;
	unsigned long save4;
	unsigned long save5;
	unsigned long save6;
	unsigned long save7;
	unsigned long tid;
	unsigned long tcfg;
	unsigned long tval;
	unsigned long cntc;
	unsigned long ticlr;
	unsigned long tlbrentry;
	unsigned long tlbrbadv;
	unsigned long tlbrera;
	unsigned long tlbrsave;
	unsigned long tlbrelo0;
	unsigned long tlbrelo1;
	unsigned long tlbrehi;
	unsigned long tlbrprmd;
	unsigned long dmw0;
	unsigned long dmw1;
	unsigned long dmw2;
	unsigned long dmw3;

	unsigned long second_crmd;
	unsigned long second_prmd;
	unsigned long second_euen;
	unsigned long second_misc;
	unsigned long second_ecfg;
	unsigned long second_estat;
	unsigned long second_era;
	unsigned long second_badv;
	unsigned long second_badi;
	unsigned long second_eentry;
	unsigned long second_tlbidx;
	unsigned long second_tlbehi;
	unsigned long second_tlbelo0;
	unsigned long second_tlbelo1;
	unsigned long second_asid;
	unsigned long second_pgdl;
	unsigned long second_pgdh;
	unsigned long second_pwcl;
	unsigned long second_pwch;
	unsigned long second_stlbps;
	unsigned long second_rvacfg;
	unsigned long second_cpuid;
	unsigned long second_prcfg1;
	unsigned long second_prcfg2;
	unsigned long second_prcfg3;
	unsigned long second_save0;
	unsigned long second_save1;
	unsigned long second_save2;
	unsigned long second_save3;
	unsigned long second_save4;
	unsigned long second_save5;
	unsigned long second_save6;
	unsigned long second_save7;
	unsigned long second_tid;
	unsigned long second_tcfg;
	unsigned long second_tval;
	unsigned long second_cntc;
	unsigned long second_ticlr;
	unsigned long second_tlbrentry;
	unsigned long second_tlbrbadv;
	unsigned long second_tlbrera;
	unsigned long second_tlbrsave;
	unsigned long second_tlbrelo0;
	unsigned long second_tlbrelo1;
	unsigned long second_tlbrehi;
	unsigned long second_tlbrprmd;
	unsigned long second_dmw0;
	unsigned long second_dmw1;
	unsigned long second_dmw2;
	unsigned long second_dmw3;
};

grub_err_t grub_arch_efi_loongvisor_boot_image(grub_addr_t addr, grub_size_t size, char *args);
grub_err_t grub_general_relocate(char* name, grub_addr_t src_addr, grub_addr_t dst_addr, grub_size_t size);
grub_err_t grub_root_initrd_init (int argc, char *argv[], struct grub_root_initrd_context *ctx);
grub_err_t grub_root_initrd_load (struct grub_root_initrd_context *root_initrd_ctx, void *target);
grub_size_t grub_root_get_initrd_size(struct grub_root_initrd_context *root_initrd_ctx);
void grub_root_initrd_close(struct grub_root_initrd_context *initrd_ctx);

#endif // _LOONGSTUB_H_