// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2025 BoneInscri <boneinscri@outlook.com>
// This script is used to load loongvisor in board of Loongarch with UEFI

#include <grub/cache.h>
#include <grub/charset.h>
#include <grub/command.h>
#include <grub/err.h>
#include <grub/file.h>
#include <grub/fdt.h>
#include <grub/list.h>
#include <grub/loader.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/efi/efi.h>
#include <grub/efi/fdtload.h>
#include <grub/efi/memory.h>
#include <grub/i18n.h>
#include <grub/lib/cmdline.h>
#include <grub/loongarch64/loongstub.h>
#include <grub/loongarch64/loongarch.h>
#include <grub/safemath.h>
#include <grub/acpi.h>
#include <grub/pci.h>

GRUB_MOD_LICENSE ("GPLv3+");

// ======================================================================================
// ============================ boot stub for loongvisor ================================
// ======================================================================================
static grub_dl_t my_mod;
static int loongvisor_loaded;
static int rootlinux_loaded;

static loongstub_boot_struct loongvisor_helper, rootlinux_helper;

static grub_command_t cmd_loongvisor;
static grub_command_t cmd_root_linux, cmd_root_initrd;

static grub_addr_t root_initrd_start;
static grub_addr_t root_initrd_end;

static grub_efi_handle_t initrd_lf2_handle = NULL;
static bool initrd_use_loadfile2 = false;

static grub_guid_t load_file2_guid = GRUB_EFI_LOAD_FILE2_PROTOCOL_GUID;
static grub_guid_t device_path_guid = GRUB_EFI_DEVICE_PATH_GUID;

static struct grub_root_initrd_context root_initrd_ctx = {0, 0, 0};

#define FDT_NODE_NAME_MAX_SIZE  (49)

static grub_efi_status_t __grub_efi_api
grub_efi_initrd_load_file2 (grub_efi_load_file2_t *this,
                            grub_efi_device_path_t *device_path,
                            grub_efi_boolean_t boot_policy,
                            grub_efi_uintn_t *buffer_size,
                            void *buffer);

static grub_efi_load_file2_t initrd_lf2 = {
  grub_efi_initrd_load_file2
};

static initrd_media_device_path_t initrd_lf2_device_path = {
  {
    {
      GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE,
      sizeof(grub_efi_vendor_media_device_path_t),
    },
    LINUX_EFI_INITRD_MEDIA_GUID
  }, {
    GRUB_EFI_END_DEVICE_PATH_TYPE,
    GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    sizeof(grub_efi_device_path_t)
  }
};

grub_size_t
grub_root_get_initrd_size (struct grub_root_initrd_context *root_initrd_ctx)
{
  return root_initrd_ctx->size;
}

static grub_efi_status_t __grub_efi_api
grub_efi_initrd_load_file2 (grub_efi_load_file2_t *this,
                            grub_efi_device_path_t *device_path,
                            grub_efi_boolean_t boot_policy,
                            grub_efi_uintn_t *buffer_size,
                            void *buffer)
{
  grub_efi_status_t status = GRUB_EFI_SUCCESS;
  grub_efi_uintn_t initrd_size;

  if (this != &initrd_lf2 || buffer_size == NULL)
    return GRUB_EFI_INVALID_PARAMETER;

  if (device_path->type != GRUB_EFI_END_DEVICE_PATH_TYPE ||
      device_path->subtype != GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)
    return GRUB_EFI_NOT_FOUND;

  if (boot_policy)
    return GRUB_EFI_UNSUPPORTED;

  initrd_size = grub_root_get_initrd_size (&root_initrd_ctx);
  if (buffer == NULL || *buffer_size < initrd_size)
  {
    *buffer_size = initrd_size;
    return GRUB_EFI_BUFFER_TOO_SMALL;
  }

  grub_printf ("linux", "Providing initrd via EFI_LOAD_FILE2_PROTOCOL\n");

  if (grub_root_initrd_load (&root_initrd_ctx, buffer))
    status = GRUB_EFI_DEVICE_ERROR;

  grub_root_initrd_close (&root_initrd_ctx);
  return status;
}

static void dump_inst_memory(grub_uint64_t addr, int inst_line)
{
    grub_printf("Dumping first 64 bytes from %p:\n", (void *)addr);
    for (int i = 0; i < inst_line; i++) {  
        grub_printf("%08lx: %02x %02x %02x %02x\n", 
                   addr + i*4,
                   *(grub_uint8_t *)(addr + i*4),
                   *(grub_uint8_t *)(addr + i*4 + 1),
                   *(grub_uint8_t *)(addr + i*4 + 2),
                   *(grub_uint8_t *)(addr + i*4 + 3));
    }
}

void *
grub_efi_allocate_fixed_protected (grub_efi_physical_address_t address,
			 grub_efi_uintn_t pages)
{
  return grub_efi_allocate_pages_real (address, pages,
				       GRUB_EFI_ALLOCATE_ADDRESS,
				       GRUB_EFI_RESERVED_MEMORY_TYPE);
}


static grub_err_t
finalize_params_loongvisor_boot (void) {
  void *loongvisor_boot_fdt;
  grub_size_t additional_size = 0x1000;

  additional_size += FDT_NODE_NAME_MAX_SIZE;

  loongvisor_boot_fdt = grub_fdt_load (additional_size);
  if (!loongvisor_boot_fdt)
    return grub_error (GRUB_ERR_IO, "failed to get FDT");

  grub_printf("loongvisor_boot_fdt: %p\n", loongvisor_boot_fdt);

  /* Set initrd info */
  if (root_initrd_start && root_initrd_end > root_initrd_start)
  {
    int node, retval;
    void *fdt;
  
    fdt = grub_fdt_load (GRUB_EFI_LINUX_FDT_EXTRA_SPACE);

    grub_printf("fdt : %p\n", fdt);
    if (!fdt)
      goto failure;

    node = grub_fdt_find_subnode (fdt, 0, "chosen");
    if (node < 0)
      node = grub_fdt_add_subnode (fdt, 0, "chosen");

    if (node < 1)
      goto failure;

    grub_printf ("linux Initrd @ %p-%p\n", (void *) root_initrd_start, (void *) root_initrd_end);

    retval = grub_fdt_set_prop64 (fdt, node, "linux,initrd-start", root_initrd_start);
    if (retval) {
      goto failure;
    }
    retval = grub_fdt_set_prop64 (fdt, node, "linux,initrd-end", root_initrd_end);
    if (retval) {
      goto failure;
    }
  }

  if (grub_fdt_install() == GRUB_ERR_NONE)
    return GRUB_ERR_NONE;

failure:
  grub_fdt_unload ();

  return grub_error (GRUB_ERR_IO, "failed to install/update FDT");
}

static void copy_trap_table(grub_uint64_t page_number) {
  #define VS 7
  grub_uint64_t trap_handler_size = 4 << VS;
  // UINTN DMW_prefix = 0x9000000000000000ULL;
  // UINTN page_number = 0x9000000100074000ULL; 

  void (*trap_table[76 + 1 + 1])() = {
#include "grub/loongarch64/trap_table_entry.h"
  };
  page_number &= 0xFFFFFFFFFFFFULL;
  for (int i = 1; i <= 76; i++) {
    if (i > 24 && i < 64) {
    // if (i > 25 && i < 64) {
      continue;
    }
    grub_uint64_t offset = trap_handler_size * i;
    grub_uint64_t addr_dst = page_number + offset;
    grub_uint64_t handler_size = (grub_uint64_t)trap_table[i + 1] - (grub_uint64_t)trap_table[i];

    char entry_name[30];
    // grub_snprintf(entry_name, sizeof(entry_name), "trap_table_entry%d", i);
    // grub_general_relocate(entry_name, (void *)addr_dst, (void *)trap_table[i], handler_size);

    // UINTN addr_dst = DMW_prefix | offset;
    // UINTN addr_dst = offset;
    grub_printf("trap_table_entry %d, from 0x%lx, copy to 0x%lx, size %ld\n", i,
          (grub_uint64_t)trap_table[i], (grub_uint64_t)addr_dst, handler_size);
    grub_memcpy((void *)addr_dst, (void *)trap_table[i], handler_size);
    // memcpy2((void *)addr_dst, (void *)trap_table[i], handler_size);

    // Print(L"Post-copy content at 0x%lx:\n", addr_dst);
    // for (int j = 0; j < 16; j++) { // 遍历4*4字节
    //     UINT8 *byte_ptr = (UINT8 *)(page_number + j);
    //     Print(L"%02x ", *byte_ptr); // 按字节输出十六进制值
    //     if ((j+1) % 4 == 0) Print(L"\n"); // 每4字节换行
    // }
    // Print(L"\n");
  }
  // grub_printf("after copy trap table\n");
  dump_inst_memory(page_number + trap_handler_size * 1, 4);
  grub_printf("trap table size : %ld\n", trap_handler_size * 76);
}

static grub_err_t 
grub_relocate_image_real(grub_addr_t* image_addr,
  grub_size_t alloc_size,
  grub_addr_t preferred_addr)
{
  grub_addr_t cur_image_addr;
  grub_addr_t new_addr = 0;
  grub_err_t error = GRUB_ERR_NONE;
  grub_size_t nr_pages;
  grub_addr_t efi_addr = preferred_addr;

  if (!image_addr || !alloc_size)
    return GRUB_EFI_INVALID_PARAMETER;
  
  cur_image_addr = *image_addr;

  efi_addr = grub_efi_allocate_fixed_protected(efi_addr, GRUB_EFI_BYTES_TO_PAGES(alloc_size));

  if (!efi_addr)
  {
    // grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
    grub_printf("error, grub_efi_allocate_fixed_protected failed\n");
    loop
  }
  new_addr = efi_addr;
  
  /*
  * We know source/dest won't overlap since both memory ranges
  * have been allocated by UEFI, so we can safely use memcpy.
  */
  // grub_memset((void*)new_addr, 0, alloc_size);
  grub_memcpy((void*)new_addr, (void*)cur_image_addr, alloc_size);

  /* Return the new address of the relocated image. */
  *image_addr = new_addr;

  return error;
}

static grub_err_t
grub_relocate_image(grub_addr_t *image_addr, grub_size_t kernel_asize,
  grub_addr_t image_base, grub_addr_t preferred_address)
{
  grub_err_t err = GRUB_ERR_NONE;
  unsigned long kernel_addr = 0;

  kernel_addr = image_base;

  err = grub_relocate_image_real(&kernel_addr, kernel_asize,
    preferred_address);

 *image_addr = kernel_addr;

  return err;
}

typedef void (*kernel_entry_t)(void);

static inline void csr_write64(grub_uint64_t value, unsigned int csr_num) {
  __asm__ __volatile__(
      "csrwr %0, %1\n"
      : 
      : "r"(value), "i"(csr_num)   
      : "memory"
  );
}

static inline unsigned int csr_read64(unsigned int csr_num) {
  unsigned int value;
  __asm__ __volatile__(
      "csrrd %0, %1\n"
      : "=r"(value)
      : "i"(csr_num)
      : "memory"
  );
  return value;
}

grub_err_t grub_general_relocate(char* name, grub_addr_t src_addr, 
  grub_addr_t dst_addr, grub_size_t size) {

  grub_err_t err = GRUB_ERR_NONE;
  grub_printf("[%s] before relocate, start = %p, end = %lx, size = %lu\n", 
    name, (void *)src_addr, src_addr + size, size);
  dump_inst_memory(src_addr, 1);

  err = grub_relocate_image(&src_addr, size, src_addr, dst_addr);
  
  grub_printf("[%s] after relocate, start = %p, end = %lx, size = %lu\n",
    name, (void *)src_addr, src_addr + size, size);
  dump_inst_memory(src_addr, 1);
}

static grub_err_t
grub_arch_efi_rootlinux_boot_image (grub_addr_t addr, grub_size_t size, char *args, int args_len)
{
  grub_printf("\n=========create boot linux context, start=========\n");
  grub_printf("kernel_addr: %p, kernel_size: %ld\n", (void *) addr, size);
  grub_printf("[attention] linux_args: %s\n", args);
  grub_printf("[attention] linux_args address: %p\n", args);

  grub_efi_memory_mapped_device_path_t *mempath;
  grub_efi_handle_t image_handle;
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;
  grub_efi_loaded_image_t *loaded_image;
  int len;

  mempath = grub_malloc (2 * sizeof (grub_efi_memory_mapped_device_path_t));
  if (!mempath) {
    grub_printf("error, [grub_arch_efi_rootlinux_boot_image] failed to allocate mempath\n");
    loop
  }
  mempath[0].header.type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE;
  mempath[0].header.subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE;
  mempath[0].header.length = grub_cpu_to_le16_compile_time (sizeof (*mempath));
  mempath[0].memory_type = GRUB_EFI_LOADER_DATA;
  mempath[0].start_address = addr;
  mempath[0].end_address = addr + size;

  grub_printf("mempath[0].start_address: %p\n", (void *) mempath[0].start_address);
  grub_printf("mempath[0].end_address: %p\n", (void *) mempath[0].end_address);

  mempath[1].header.type = GRUB_EFI_END_DEVICE_PATH_TYPE;
  mempath[1].header.subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  mempath[1].header.length = sizeof (grub_efi_device_path_t);

  grub_printf("mempath[1].header.type: %d\n", mempath[1].header.type);
  grub_printf("mempath[1].header.subtype: %d\n", mempath[1].header.subtype);
  grub_printf("mempath[1].header.length: %d\n", mempath[1].header.length);

  b = grub_efi_system_table->boot_services;
  status = b->load_image (0, grub_efi_image_handle,
			  (grub_efi_device_path_t *) mempath,
			  (void *) addr, size, &image_handle);
  if (status != GRUB_EFI_SUCCESS) {
    grub_printf("cannot load image");
    loop
  }
  grub_printf("load_image done\n");
  
  /* Convert command line to UCS-2 */
  loaded_image = grub_efi_get_loaded_image (image_handle);
  if (loaded_image == NULL)
  {
    grub_printf("missing loaded_image proto");
    loop
  }
  loaded_image->load_options_size = len =
    (grub_strlen (args) + 1) * sizeof (grub_efi_char16_t);
  loaded_image->load_options =
    grub_efi_allocate_any_pages (GRUB_EFI_BYTES_TO_PAGES (loaded_image->load_options_size));
  if (!loaded_image->load_options) {
    grub_printf("cannot allocate load_options");
    loop
  }
    
  grub_printf("load_options_size : %d\n", loaded_image->load_options_size);
  grub_printf("load_options: %p\n", loaded_image->load_options);
  
  loaded_image->load_options_size =
  2 * grub_utf8_to_utf16 (loaded_image->load_options, len,
        (grub_uint8_t *) args, len, NULL);
  grub_printf("load_options : %p, load_options_size : %d\n", loaded_image->load_options, loaded_image->load_options_size); 

  grub_printf("ready to call start_image\n");
  grub_printf("[Attention!!!] b->start_image : %p, image_handle: %p\n", b->start_image, image_handle);

  // check image_handle and efi_system_table
  grub_printf("grub_efi_image_handle: %p, grub_efi_system_table: %p\n", grub_efi_image_handle, grub_efi_system_table);

  grub_addr_t boot_context_addr = loongvisor_helper.boot_ctx_addr;
  // grub_addr_t ctx = grub_efi_allocate_fixed_protected(boot_context_addr, 
  //   GRUB_EFI_BYTES_TO_PAGES(sizeof(struct boot_context)));
  if (loongvisor_helper.boot_ctx_load_size < sizeof(struct boot_context)) {
    grub_printf("the size of boot context is too small, check it, boot_context_size : %llx, load_size : %llx\n", 
      sizeof(struct boot_context), loongvisor_helper.boot_ctx_load_size);
    loop
  }
  grub_addr_t ctx = grub_efi_allocate_fixed_protected(boot_context_addr, 
    GRUB_EFI_BYTES_TO_PAGES(loongvisor_helper.boot_ctx_load_size));
  grub_printf("save boot context: %p\n", (void *)ctx);

  save_context(ctx, b->start_image, image_handle, grub_efi_system_table, args);

  grub_printf("\n=========create boot linux context, ok=========\n");
}

grub_err_t
grub_arch_efi_loongvisor_boot_image (grub_addr_t addr, grub_size_t size, char *args)
{
  grub_err_t err = GRUB_ERR_NONE;
  
  if (size > loongvisor_helper.image_load_size) {
    grub_printf("the size of loongvisor is too large, check it, size %llx, image_load_size %llx\n", 
      size, loongvisor_helper.image_load_size);
    loop
  }
  
  // relocate loongvisor
  // grub_general_relocate("loongvisor", addr, loongvisor_helper.image_load_addr, size);

  grub_printf("\n=========relocate loongvisor start=========\n");
  grub_general_relocate("loongvisor", addr, loongvisor_helper.image_load_addr, loongvisor_helper.image_load_size);
  grub_printf("=========relocate loongvisor over=========\n\n");

  void *root_linux_addr = rootlinux_helper.image_addr;
  grub_size_t root_linux_size = rootlinux_helper.image_size;
  char *root_linux_args = rootlinux_helper.image_args;
  int root_linux_args_len = rootlinux_helper.image_cmdline_size;
  grub_arch_efi_rootlinux_boot_image ((grub_addr_t) root_linux_addr, root_linux_size, 
  root_linux_args, root_linux_args_len);
  
  kernel_entry_t real_kernel_entry = (void *)loongvisor_helper.image_load_addr;
  // bool efi, unsigned long cmdline, unsigned long systab

  grub_printf("\n=========copy trap table start=========\n");
  copy_trap_table(loongvisor_helper.image_trap_vector_addr);
  grub_printf("=========copy trap table over=========\n\n");

  grub_printf("ready to jump to loongvisor\n");
  real_kernel_entry();

  loop

  return err;
}

static grub_err_t
grub_loongvisor_boot (void)
{
  if (!loongvisor_loaded) {
    grub_printf("error, loongvisor is not loaded\n");
    loop
  }
  if (!rootlinux_loaded) {
    grub_printf("error, rootlinux is not loaded\n");
    loop
  }
  grub_err_t err = finalize_params_loongvisor_boot ();
  if (err)
    return err;
  
  void *loongvisor_addr = loongvisor_helper.image_addr;
  grub_size_t loongvisor_size = loongvisor_helper.image_size;
  char *loongvisor_args = loongvisor_helper.image_args;
  return grub_arch_efi_loongvisor_boot_image ((grub_addr_t) loongvisor_addr, loongvisor_size, loongvisor_args);
}

static grub_err_t
grub_loongvisor_unload (void)
{
  // TODO
  grub_printf("error, grub_loongvisor_unload not tested");
  loop
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_arch_efi_rootlinux_load_image_header (grub_file_t file,
                                      struct linux_arch_kernel_header * lh)
{
  grub_file_seek (file, 0);
  if (grub_file_read (file, lh, sizeof (*lh)) < (grub_ssize_t) sizeof (*lh))
    return grub_error(GRUB_ERR_FILE_READ_ERROR, "failed to read Linux image header");

  if ((lh->code0 & 0xffff) != GRUB_DOS_MAGIC)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       N_("plain image kernel not supported - rebuild with CONFIG_(U)EFI_STUB enabled"));

  grub_printf ("linux", "UEFI stub kernel:\n");
  grub_printf ("linux", "PE/COFF header @ %08x\n", lh->hdr_offset);

  /*
   * The PE/COFF spec permits the COFF header to appear anywhere in the file, so
   * we need to double check whether it was where we expected it, and if not, we
   * must load it from the correct offset into the pe_image_header field of
   * struct linux_arch_kernel_header.
   */
  if ((grub_uint8_t *) lh + lh->hdr_offset != (grub_uint8_t *) &lh->pe_image_header)
    {
      if (grub_file_seek (file, lh->hdr_offset) == (grub_off_t) -1
          || grub_file_read (file, &lh->pe_image_header,
                             sizeof (struct grub_pe_image_header))
             != sizeof (struct grub_pe_image_header))
        return grub_error (GRUB_ERR_FILE_READ_ERROR, "failed to read COFF image header");
    }

  if (lh->pe_image_header.optional_header.magic != GRUB_PE32_NATIVE_MAGIC)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "non-native image not supported");

  /*
   * Linux kernels built for any architecture are guaranteed to support the
   * LoadFile2 based initrd loading protocol if the image version is >= 1.
   */
  if (lh->pe_image_header.optional_header.major_image_version >= 1)
    initrd_use_loadfile2 = true;
  else
    initrd_use_loadfile2 = false;

  grub_printf ("linux", "LoadFile2 initrd loading %sabled\n",
                initrd_use_loadfile2 ? "en" : "dis");

  return GRUB_ERR_NONE;
}

static grub_err_t 
grub_cmd_load_kernel(int argc, char *argv[], loongstub_boot_struct* boot_helper) 
{
  grub_file_t file = 0;
  grub_err_t err;
    
  grub_dl_ref (my_mod);

  grub_printf("\nready to load %s ...\n", boot_helper->name);

  if (!argc)
  {
    grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
    goto fail;
  }
  for (int i = 0; i < argc; i++)
  {
    grub_printf("argv[%d]: %s\n", i, argv[i]);
  }

  file = grub_file_open (argv[0], GRUB_FILE_TYPE_LINUX_KERNEL);
  if (!file)
    goto fail;
  grub_printf("%s file opened\n", boot_helper->name);

  boot_helper->image_size = grub_file_size (file);
  grub_printf("%s file size: %ld\n", boot_helper->name, boot_helper->image_size);

  // for Root linux
  if (boot_helper->boot_type == BOOT_TYPE_ROOT_LINUX) {
    struct linux_arch_kernel_header lh;
    if (grub_arch_efi_rootlinux_load_image_header (file, &lh) != GRUB_ERR_NONE) {
      grub_printf("error, [grub_arch_efi_linux_load_image_header] failed\n");
      loop
    }  
  }
  if (boot_helper->boot_type == BOOT_TYPE_LOONGVISOR) {
    grub_loader_unset();
    grub_printf("%s grub_loader_unset done\n", boot_helper->name);  
  }
  
  boot_helper->image_addr = grub_efi_allocate_any_pages (GRUB_EFI_BYTES_TO_PAGES (boot_helper->image_size));
  if (!boot_helper->image_addr)
  {
    goto fail;
  }
  grub_printf("%s addr: %p\n", boot_helper->name, boot_helper->image_addr);

  if (grub_file_seek (file, 0) < 0)
    goto fail;
  if (grub_file_read (file, boot_helper->image_addr, boot_helper->image_size) < (grub_int64_t) (boot_helper->image_size))
  {
    if (!grub_errno) {
      grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"), argv[0]);
    }
    goto fail;
  }
  grub_printf("%s file read done\n", boot_helper->name);

  boot_helper->image_cmdline_size = grub_loader_cmdline_size (argc, argv) + sizeof (boot_helper->name);
  boot_helper->image_args = grub_malloc (boot_helper->image_cmdline_size);
  if (!boot_helper->image_args)
  {
    grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
    goto fail;
  }
  grub_memcpy (boot_helper->image_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  // grub_memcpy (boot_helper->image_args + sizeof (boot_helper->name) - 1, argv[0], grub_strlen(argv[0]) + 1);
  err = grub_create_loader_cmdline(argc, argv, boot_helper->image_args + sizeof(LINUX_IMAGE) - 1,
                                   boot_helper->image_cmdline_size, GRUB_VERIFY_KERNEL_CMDLINE);
  if (err)
    goto fail;
  grub_printf("%s grub_create_loader_cmdline done\n", boot_helper->name);
  grub_printf("=>> [Attention!!!] %s cmd_line: %s, cmd_line_size: %d, cmd_line pointer: %p\n", 
    boot_helper->name, boot_helper->image_args, boot_helper->image_cmdline_size, boot_helper->image_args);

  grub_printf("==========================\n");
  grub_printf("%s_start: %p\n", boot_helper->name, boot_helper->image_addr);
  grub_printf("%s_end: 0x%lx\n", boot_helper->name, (grub_addr_t) (boot_helper->image_addr) + boot_helper->image_size);
  grub_printf("%s_size: %ld\n", boot_helper->name, boot_helper->image_size);
  grub_printf("==========================\n\n");
  if (grub_errno == GRUB_ERR_NONE)
  {
    if (boot_helper->boot_type == BOOT_TYPE_LOONGVISOR) {
      loongvisor_loaded = 1;
      grub_loader_set(grub_loongvisor_boot, grub_loongvisor_unload, 0);
    } else if (boot_helper->boot_type == BOOT_TYPE_ROOT_LINUX) {
      rootlinux_loaded = 1;
    } else {
      grub_printf("error, unknown boot type\n");
      loop
    }
  }
  return 0;

fail:
  if (file)
    grub_file_close (file);
    if (grub_errno != GRUB_ERR_NONE)
    {
      grub_dl_unref (my_mod);
      loongvisor_loaded = 0;
    }
  if (boot_helper->image_args && !loongvisor_loaded)
    grub_free (boot_helper->image_args);

  if (boot_helper->image_addr && !loongvisor_loaded)
    grub_efi_free_pages ((grub_addr_t) (boot_helper->image_addr), 
    GRUB_EFI_BYTES_TO_PAGES (boot_helper->image_size));
  return grub_errno;
}

static grub_uint64_t my_strcspn(const char *s1, const char *s2) {
  const char *p1 = s1;
  const char *p2;

  while (*p1) {
      p2 = s2;
      while (*p2) {
          if (*p1 == *p2) {
              return p1 - s1;
          }
          p2++;
      }
      p1++;
  }
  return p1 - s1;
}

#define PAGE_SHIFT  12                     // 4KB = 2^12
#define PAGE_SIZE   (1UL << PAGE_SHIFT)    // 0x1000
#define PAGE_MASK   (~(PAGE_SIZE - 1))     // 0xFFFF_FFFF_FFFF_F000
#define PAGE_LEVELS 3  // for 9+9+9+9+12 splitting

static int valid(grub_uint64_t value) {
  if(value != 0x0000000000000000 && value != 0xafafafafafafafaf) {
    return 1;
  } else {
    return 0;
  }
}
static int is_leaf(grub_uint64_t entry) {
  if (entry & 0xfff) {
    return 1;
  } else {
    return 0;
  }
}

static void walk_pagetable(grub_addr_t table_addr, int level, grub_addr_t vaddr_parent) {
  int entries = 512;  // 2^9 entries per level
  int bits_per_level = 9;
  grub_addr_t vaddr_mask = (1UL << bits_per_level) - 1;
  
  for (int i = 0; i < entries; i++) {
      grub_addr_t *entry_ptr = (grub_addr_t *)(table_addr + i * sizeof(grub_addr_t));
      grub_addr_t entry = *entry_ptr;
      grub_addr_t vaddr = vaddr_parent | (i << (PAGE_SHIFT + bits_per_level * (PAGE_LEVELS - 1 - level)));

      if (!valid(entry)) {
          continue;
      }

      // for (int l = 0; l < level; l++) grub_printf("  ");
      // if (is_leaf(entry)) {
      //   grub_printf("%d[%03d]: 0x%016lx -> 0x%016lx\n", level, i, vaddr, entry);      
      // } else {  
      //   grub_printf("%d[%03d]: 0x%016lx \n", level, i, entry);
      // }
      for (int l = 0; l < level; l++) grub_printf("  ");
      grub_printf("%d[%03d]: 0x%016lx \n", level, i, entry);
      if (level < PAGE_LEVELS - 1) {
        if(is_leaf(entry)) {
          grub_printf("error, can't walk leaf page\n");
        }
        walk_pagetable(entry & PAGE_MASK, level + 1, vaddr);
      } else {
          // For the last level, print the full virtual address range
          // grub_addr_t start = vaddr;
          // grub_addr_t end = vaddr + PAGE_SIZE - 1;
          // for (int l = 0; l < level; l++) grub_printf("  ");
          // grub_printf("  Maps: 0x%016lx - 0x%016lx\n", start, end);
      }
  }
}

static void print_pagetable() {
  grub_addr_t pgd = csr_read64(LOONGARCH_CSR_PGDL);
  grub_printf("Top-level PGD (L0): 0x%016lx\n", pgd);
  walk_pagetable(pgd, 0, 0);
}

static void 
print_acpi_table_header(const struct grub_acpi_table_header *header) {
  grub_printf("ACPI Table Header:\n");
  grub_printf("  Signature: %.4s\n", header->signature);
  grub_printf("  Length: %u bytes\n", header->length);
  grub_printf("  Revision: %u\n", header->revision);
  grub_printf("  Checksum: 0x%02X\n", header->checksum);
}

static grub_err_t
grub_cmd_loongvisor (grub_command_t cmd __attribute__ ((unused)),
			 int argc, char *argv[])
{
  // print_pagetable();

  // test xsdt
  // grub_addr_t xsdt_addr = 0xf9bb0000;
  // struct grub_acpi_table_header *xsdt = (struct grub_acpi_table_header *) xsdt_addr;
  // print_acpi_table_header(xsdt);

  // while(1) {}
  
  if (argc != 6) {
    grub_printf("error, loongvisor cmd number should be 6\n");
    loop
  }
  char *name = "LOONGVISOR";
  loongvisor_helper.boot_type = BOOT_TYPE_LOONGVISOR;
  grub_memcpy(loongvisor_helper.name, name, NAME_MAX);

  grub_printf("argv[1]: %s\n", argv[1]);
  grub_printf("argv[2]: %s\n", argv[2]);
  grub_printf("argv[3]: %s\n", argv[3]);
  grub_printf("argv[4]: %s\n", argv[1]);
  grub_printf("argv[5]: %s\n", argv[2]);

  // argv[1] : kernel load address (hypervisor .bin)
  loongvisor_helper.image_load_addr = (grub_addr_t) grub_strtoul(argv[1], NULL, 16);

  // argv[2] : kernel load size
  // assert kernel load size !=0
  loongvisor_helper.image_load_size = (grub_size_t)grub_strtoul(argv[2], NULL, 16);

  // argv[3] : boot context address (boot context)
  loongvisor_helper.boot_ctx_addr = (grub_addr_t) grub_strtoul(argv[3], NULL, 16);

  // argv[4] : boot context load size
  // assert kernel load size !=0
  loongvisor_helper.boot_ctx_load_size = (grub_size_t) grub_strtoul(argv[4], NULL, 16);

  // argv[5] : trap_verctor address file
  grub_file_t file = 0;
  file = grub_file_open (argv[5], GRUB_FILE_TYPE_LINUX_KERNEL);
  if (!file) {
    grub_printf("trap-vector file open failed\n");
    loop
  }
  if (grub_file_seek (file, 0) == (grub_off_t) -1) {
    grub_printf("trap-vector file seek failed\n");
    loop
  }

  char trap_vector_address[20];
  grub_file_read(file, trap_vector_address, 20);

  trap_vector_address[my_strcspn(trap_vector_address, "\n")] = '\0';

  loongvisor_helper.image_trap_vector_addr = (grub_addr_t) grub_strtoul(trap_vector_address, NULL, 16);
  grub_printf("trap_vector_address: %lx\n", loongvisor_helper.image_trap_vector_addr);

  grub_cmd_load_kernel(argc, argv, &loongvisor_helper);
}

static grub_err_t
grub_cmd_root_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  char *name = "ROOT_LINUX";
  rootlinux_helper.boot_type = BOOT_TYPE_ROOT_LINUX;
  grub_memcpy(rootlinux_helper.name, name, NAME_MAX);
  grub_cmd_load_kernel(argc, argv, &rootlinux_helper);
}

static void free_dir (struct dir *root)
{
  if (!root)
    return;
  free_dir (root->next);
  free_dir (root->child);
  grub_free (root->name);
  grub_free (root);
}

static char
hex (grub_uint8_t val)
{
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

static void
set_field (char *var, grub_uint32_t val)
{
  int i;
  char *ptr = var;
  for (i = 28; i >= 0; i -= 4) {
    *ptr++ = hex((val >> i) & 0xf);
  }
}

static grub_uint8_t *
make_header (grub_uint8_t *ptr,
	     const char *name, grub_size_t len,
	     grub_uint32_t mode,
	     grub_off_t fsize)
{
  struct newc_head *head = (struct newc_head *) ptr;
  grub_uint8_t *optr;
  grub_size_t oh = 0;

  grub_printf ("linux", "newc: Creating path '%s', mode=%s%o, size=%" PRIuGRUB_OFFSET "\n", name, (mode == 0) ? "" : "0", mode, fsize);
  grub_memcpy (head->magic, "070701", 6);
  set_field (head->ino, 0);
  set_field (head->mode, mode);
  set_field (head->uid, 0);
  set_field (head->gid, 0);
  set_field (head->nlink, 1);
  set_field (head->mtime, 0);
  set_field (head->filesize, fsize);
  set_field (head->devmajor, 0);
  set_field (head->devminor, 0);
  set_field (head->rdevmajor, 0);
  set_field (head->rdevminor, 0);
  set_field (head->namesize, len);
  set_field (head->check, 0);
  optr = ptr;
  ptr += sizeof (struct newc_head);
  grub_memcpy (ptr, name, len);
  ptr += len;
  oh = ALIGN_UP_OVERHEAD (ptr - optr, 4);
  grub_memset (ptr, 0, oh);
  ptr += oh;
  return ptr;
}

static grub_err_t
insert_dir (const char *name, struct dir **root,
	    grub_uint8_t *ptr, grub_size_t *size)
{
  struct dir *cur, **head = root;
  const char *cb, *ce = name;
  *size = 0;

  while (1)
  {
    for (cb = ce; *cb == '/'; cb++);
    for (ce = cb; *ce && *ce != '/'; ce++);
    if (!*ce) {
      break;
    }

    for (cur = *root; cur; cur = cur->next)
    if (grub_memcmp (cur->name, cb, ce - cb) == 0
      && cur->name[ce - cb] == 0) {
        break;
    }
    if (!cur)
    {
      struct dir *n;
      n = grub_zalloc (sizeof (*n));
      if (!n) {
        return 0;
      }
      n->next = *head;
      n->name = grub_strndup (cb, ce - cb);
      if (ptr)
      {
        /*
        * Create the substring with the trailing NUL byte
        * to be included in the cpio header.
        */
        char *tmp_name = grub_strndup (name, ce - name);
        if (!tmp_name) {
          grub_free (n->name);
          grub_free (n);
          return grub_errno;
        }
        ptr = make_header (ptr, tmp_name, ce - name + 1, 040777, 0);
        grub_free (tmp_name);
      }
      if (grub_add (*size, ALIGN_UP ((ce - (char *) name + 1) + sizeof (struct newc_head), 4), size))
      {
        grub_error (GRUB_ERR_OUT_OF_RANGE, N_("overflow is detected"));
        grub_free (n->name);
        grub_free (n);
        return grub_errno;
      }
      *head = n;
      cur = n;
    }
    root = &cur->next;
  }
  return GRUB_ERR_NONE;
}

grub_err_t
grub_root_initrd_load (struct grub_root_initrd_context *root_initrd_ctx, void *target)
{
  grub_printf("come here, grub_root_initrd_load\n");
  grub_uint8_t *ptr = target;
  int i;
  int newc = 0;
  struct dir *root = 0;
  grub_ssize_t cursize = 0;

  for (i = 0; i < root_initrd_ctx->nfiles; i++)
  {
    grub_memset (ptr, 0, ALIGN_UP_OVERHEAD (cursize, 4));
    ptr += ALIGN_UP_OVERHEAD (cursize, 4);

    if (root_initrd_ctx->components[i].newc_name)
	  {
      grub_size_t dir_size;

      if (insert_dir (root_initrd_ctx->components[i].newc_name, &root, ptr, &dir_size))
      {
        free_dir (root);
        grub_root_initrd_close (root_initrd_ctx);
        return grub_errno;
      }
      ptr += dir_size;
      ptr = make_header (ptr, root_initrd_ctx->components[i].newc_name,
            grub_strlen (root_initrd_ctx->components[i].newc_name) + 1,
            0100777,
            root_initrd_ctx->components[i].size);
      newc = 1;
    }
    else if (newc)
    {
      ptr = make_header (ptr, "TRAILER!!!", sizeof ("TRAILER!!!"),
            0, 0);
      free_dir (root);
      root = 0;
      newc = 0;
    }

    cursize = root_initrd_ctx->components[i].size;
    if (grub_file_read (root_initrd_ctx->components[i].file, ptr, cursize) != cursize)
    {
      if (!grub_errno) {
        grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
        root_initrd_ctx->components[i].file->name);
      }
      grub_root_initrd_close (root_initrd_ctx);
      return grub_errno;
    }
    ptr += cursize;
  }
  if (newc)
  {
    grub_memset (ptr, 0, ALIGN_UP_OVERHEAD (cursize, 4));
    ptr += ALIGN_UP_OVERHEAD (cursize, 4);
    ptr = make_header (ptr, "TRAILER!!!", sizeof ("TRAILER!!!"), 0, 0);
  }
  free_dir (root);
  root = 0;
  return GRUB_ERR_NONE;
}

void grub_root_initrd_close (struct grub_root_initrd_context *root_initrd_ctx)
{
  int i;
  if (!root_initrd_ctx->components)
    return;
  for (i = 0; i < root_initrd_ctx->nfiles; i++)
    {
      grub_free (root_initrd_ctx->components[i].newc_name);
      grub_file_close (root_initrd_ctx->components[i].file);
    }
  grub_free (root_initrd_ctx->components);
  root_initrd_ctx->components = 0;
}

grub_err_t grub_root_initrd_init (int argc, char *argv[], 
  struct grub_root_initrd_context *root_initrd_ctx)
{
  int i;
  int newc = 0;
  struct dir *root = 0;

  root_initrd_ctx->nfiles = 0;
  root_initrd_ctx->components = 0;

  root_initrd_ctx->components = grub_calloc(argc, sizeof (root_initrd_ctx->components[0]));
  if (!root_initrd_ctx->components)
    return grub_errno;

  root_initrd_ctx->size = 0;

  for (i = 0; i < argc; i++)
  {
    const char *fname = argv[i];

    root_initrd_ctx->size = ALIGN_UP (root_initrd_ctx->size, 4);

    if (grub_memcmp (argv[i], "newc:", 5) == 0)
    {
      const char *ptr, *eptr;
      ptr = argv[i] + 5;
      while (*ptr == '/')
        ptr++;
      eptr = grub_strchr (ptr, ':');
      if (eptr)
      {
        grub_size_t dir_size, name_len;

        root_initrd_ctx->components[i].newc_name = grub_strndup (ptr, eptr - ptr);
        if (!root_initrd_ctx->components[i].newc_name || insert_dir (root_initrd_ctx->components[i].newc_name, &root, 0, &dir_size)) 
        {
          grub_printf("grub_root_initrd_init, error 1\n");
          loop
          // grub_initrd_close (root_initrd_ctx);
          // return grub_errno;
        }
        name_len = grub_strlen (root_initrd_ctx->components[i].newc_name) + 1;
        if (grub_add (root_initrd_ctx->size, ALIGN_UP (sizeof (struct newc_head) + name_len, 4), &root_initrd_ctx->size) || 
            grub_add (root_initrd_ctx->size, dir_size, &root_initrd_ctx->size)) {
            goto overflow;
        }
        newc = 1;
        fname = eptr + 1;
      }
    }
    else if (newc)
	  {
	    if (grub_add (root_initrd_ctx->size, ALIGN_UP(sizeof (struct newc_head) + sizeof ("TRAILER!!!"), 4), 
        &root_initrd_ctx->size)) {
          goto overflow;
        }
        free_dir (root);
        root = 0;
        newc = 0;
    }
    root_initrd_ctx->components[i].file = grub_file_open (fname, GRUB_FILE_TYPE_LINUX_INITRD | GRUB_FILE_TYPE_NO_DECOMPRESS);
    if (!root_initrd_ctx->components[i].file)
    {
      grub_printf("grub_root_initrd_init, error 2\n");
      loop
      // grub_initrd_close (root_initrd_ctx);
      // return grub_errno;
    }
    root_initrd_ctx->nfiles++;
    root_initrd_ctx->components[i].size = grub_file_size (root_initrd_ctx->components[i].file);
    if (grub_add (root_initrd_ctx->size, root_initrd_ctx->components[i].size, &root_initrd_ctx->size))
      goto overflow;
    }

    if (newc)
    {
      root_initrd_ctx->size = ALIGN_UP (root_initrd_ctx->size, 4);
      if (grub_add (root_initrd_ctx->size, 
        ALIGN_UP (sizeof (struct newc_head) + sizeof ("TRAILER!!!"), 4), &root_initrd_ctx->size)) {
        goto overflow; 
      }
      free_dir (root);
      root = 0;
    }
    return GRUB_ERR_NONE;

overflow:
  free_dir (root);
  grub_root_initrd_close (root_initrd_ctx);
  return grub_error (GRUB_ERR_OUT_OF_RANGE, N_("overflow is detected"));
}

static grub_err_t
grub_cmd_root_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  int __attribute__ ((unused)) initrd_size, initrd_pages;
  void *__attribute__ ((unused)) initrd_mem = NULL;
  grub_efi_boot_services_t *b = grub_efi_system_table->boot_services;
  grub_efi_status_t status;

  if (argc == 0)
  {
    grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
    goto fail;
  }

  for (int i = 0; i < argc; i++)
  {
    grub_printf("root initrd argv[%d]: %s\n", i, argv[i]);
  }

  if (!loongvisor_loaded || !rootlinux_loaded)
  {
    grub_error (GRUB_ERR_BAD_ARGUMENT,
    N_("you need to load the kernel first"));
    goto fail;
  }
  if (grub_root_initrd_init (argc, argv, &root_initrd_ctx))
    goto fail;

  // grub_printf("root_initrd_ctx contents:\n");
  // grub_printf("  nfiles: %d\n", root_initrd_ctx.nfiles);
  // grub_printf("  size: %" PRIuGRUB_SIZE "\n", root_initrd_ctx.size);

  if (initrd_use_loadfile2)
  {
    if (initrd_lf2_handle == NULL)
    {
      status = b->install_multiple_protocol_interfaces (&initrd_lf2_handle,
                                                        &load_file2_guid,
                                                        &initrd_lf2,
                                                        &device_path_guid,
                                                        &initrd_lf2_device_path,
                                                        NULL);
      if (status == GRUB_EFI_OUT_OF_RESOURCES)
      {
        grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
        goto fail;
      }
      else if (status != GRUB_EFI_SUCCESS)
      {
        grub_error (GRUB_ERR_BAD_ARGUMENT, N_("failed to install protocols"));
        goto fail;
      }
    }
    grub_printf("Using LoadFile2 initrd loading protocol\n");

    return GRUB_ERR_NONE;
  }

fail:
  return grub_errno;
}

static void print_logo() {
  grub_printf("%s\n","                                       ");
  grub_printf("%s\n"," __                        _       _   ");
  grub_printf("%s\n","|  |   ___ ___ ___ ___ ___| |_ _ _| |_ ");
  grub_printf("%s\n","|  |__| . | . |   | . |_ -|  _| | | . |");
  grub_printf("%s\n","|_____|___|___|_|_|_  |___|_| |___|___|");
  grub_printf("%s\n","                  |___|                ");
}

GRUB_MOD_INIT (loongstub)
{
  print_logo();
  cmd_loongvisor = grub_register_command("loongvisor", grub_cmd_loongvisor, 0,
                                         N_("Load Loongvisor"));
  cmd_root_linux = grub_register_command ("root_linux", grub_cmd_root_linux, 0,
				     N_("Load Root Linux."));
  cmd_root_initrd = grub_register_command ("root_initrd", grub_cmd_root_initrd, 0,
				      N_("Load Root initrd."));
  my_mod = mod;
}

GRUB_MOD_FINI (loongvisor)
{
  grub_unregister_command (cmd_loongvisor);
  grub_unregister_command (cmd_root_linux);
  grub_unregister_command (cmd_root_initrd);
}