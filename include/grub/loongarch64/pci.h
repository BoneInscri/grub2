/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2025  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_CPU_PCI_H
#define GRUB_CPU_PCI_H 1

#include <grub/types.h>

/* LoongArch64 accesses PCI config space via MMIO, not I/O ports.
   The actual read/write helpers are implemented in loongstub.c using
   the IOMMU MMIO window, so we only need the stub declarations here. */

#define GRUB_PCI_NUM_BUS     256
#define GRUB_PCI_NUM_DEVICES  32

static inline grub_uint32_t
grub_pci_read (grub_pci_address_t addr __attribute__ ((unused)))
{
  return 0xffffffff;
}

static inline grub_uint16_t
grub_pci_read_word (grub_pci_address_t addr __attribute__ ((unused)))
{
  return 0xffff;
}

static inline grub_uint8_t
grub_pci_read_byte (grub_pci_address_t addr __attribute__ ((unused)))
{
  return 0xff;
}

static inline void
grub_pci_write (grub_pci_address_t addr __attribute__ ((unused)),
                grub_uint32_t data __attribute__ ((unused)))
{
}

static inline void
grub_pci_write_word (grub_pci_address_t addr __attribute__ ((unused)),
                     grub_uint16_t data __attribute__ ((unused)))
{
}

static inline void
grub_pci_write_byte (grub_pci_address_t addr __attribute__ ((unused)),
                     grub_uint8_t data __attribute__ ((unused)))
{
}

static inline volatile void *
grub_pci_device_map_range (grub_pci_device_t dev __attribute__ ((unused)),
                           grub_addr_t base,
                           grub_size_t size __attribute__ ((unused)))
{
  return (volatile void *) base;
}

static inline void
grub_pci_device_unmap_range (grub_pci_device_t dev __attribute__ ((unused)),
                             volatile void *mem __attribute__ ((unused)),
                             grub_size_t size __attribute__ ((unused)))
{
}

#endif /* GRUB_CPU_PCI_H */
