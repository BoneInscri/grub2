
#include <grub/loongarch64/loongarch.h>
#include <grub/loongarch64/regdef.h>
#include <grub/symbol.h>

	.file	"loongstub_kernel.S"
	.text

# a0: the boot_context pointer, a1: start_image, a2: image_handle, a3: efi_system_table, a4: cmd_line_ptr
FUNCTION(save_context)
    st.d ra, a0, 0
    st.d sp, a0, 8
    st.d tp, a0, 16
    st.d s0, a0, (0+3)*8
    st.d s1, a0, (1+3)*8
    st.d s2, a0, (2+3)*8
    st.d s3, a0, (3+3)*8
    st.d s4, a0, (4+3)*8
    st.d s5, a0, (5+3)*8
    st.d s6, a0, (6+3)*8
    st.d s7, a0, (7+3)*8
    st.d s8, a0, (8+3)*8
    st.d fp, a0, (9+3)*8
    st.d a1, a0, (10+3)*8// start_image
    st.d a2, a0, (11+3)*8// image_handle
    st.d a3, a0, (12+3)*8// efi_system_table
    st.d a4, a0, (13+3)*8// cmd_line_ptr
    ; st.d t1, a0, (13+3)*8
    st.d t2, a0, (14+3)*8
    st.d t3, a0, (15+3)*8
    st.d t4, a0, (16+3)*8
    st.d t5, a0, (17+3)*8
    st.d t6, a0, (18+3)*8
    st.d t7, a0, (19+3)*8
    st.d t8, a0, (20+3)*8
    csrrd a4, LOONGARCH_CSR_CRMD
    st.d a4, a0, (21+3)*8
    csrrd a4, LOONGARCH_CSR_PRMD
    st.d a4, a0, (22+3)*8
    csrrd a4, LOONGARCH_CSR_EUEN
    st.d a4, a0, (23+3)*8
    csrrd a4, LOONGARCH_CSR_MISC
    st.d a4, a0, (24+3)*8
    csrrd a4, LOONGARCH_CSR_ECFG
    st.d a4, a0, (25+3)*8
    csrrd a4, LOONGARCH_CSR_ESTAT
    st.d a4, a0, (26+3)*8
    csrrd a4, LOONGARCH_CSR_ERA
    st.d a4, a0, (27+3)*8
    csrrd a4, LOONGARCH_CSR_BADV
    st.d a4, a0, (28+3)*8
    csrrd a4, LOONGARCH_CSR_BADI
    st.d a4, a0, (29+3)*8
    csrrd a4, LOONGARCH_CSR_EENTRY
    st.d a4, a0, (30+3)*8
    csrrd a4, LOONGARCH_CSR_TLBIDX
    st.d a4, a0, (31+3)*8
    csrrd a4, LOONGARCH_CSR_TLBEHI
    st.d a4, a0, (32+3)*8
    csrrd a4, LOONGARCH_CSR_TLBELO0
    st.d a4, a0, (33+3)*8
    csrrd a4, LOONGARCH_CSR_TLBELO1
    st.d a4, a0, (34+3)*8
    csrrd a4, LOONGARCH_CSR_ASID
    st.d a4, a0, (35+3)*8
    csrrd a4, LOONGARCH_CSR_PGDL
    st.d a4, a0, (36+3)*8
    csrrd a4, LOONGARCH_CSR_PGDH
    st.d a4, a0, (37+3)*8
    csrrd a4, LOONGARCH_CSR_PWCTL0
    st.d a4, a0, (38+3)*8
    csrrd a4, LOONGARCH_CSR_PWCTL1
    st.d a4, a0, (39+3)*8
    csrrd a4, LOONGARCH_CSR_STLBPGSIZE
    st.d a4, a0, (40+3)*8
    csrrd a4, LOONGARCH_CSR_RVACFG
    st.d a4, a0, (41+3)*8
    csrrd a4, LOONGARCH_CSR_CPUID
    st.d a4, a0, (42+3)*8
    csrrd a4, LOONGARCH_CSR_PRCFG1
    st.d a4, a0, (43+3)*8
    csrrd a4, LOONGARCH_CSR_PRCFG2
    st.d a4, a0, (44+3)*8
    csrrd a4, LOONGARCH_CSR_PRCFG3
    st.d a4, a0, (45+3)*8
    csrrd a4, LOONGARCH_CSR_KS0
    st.d a4, a0, (46+3)*8
    csrrd a4, LOONGARCH_CSR_KS1
    st.d a4, a0, (47+3)*8
    csrrd a4, LOONGARCH_CSR_KS2
    st.d a4, a0, (48+3)*8
    csrrd a4, LOONGARCH_CSR_KS3
    st.d a4, a0, (49+3)*8
    csrrd a4, LOONGARCH_CSR_KS4
    st.d a4, a0, (50+3)*8
    csrrd a4, LOONGARCH_CSR_KS5
    st.d a4, a0, (51+3)*8
    csrrd a4, LOONGARCH_CSR_KS6
    st.d a4, a0, (52+3)*8
    csrrd a4, LOONGARCH_CSR_KS7
    st.d a4, a0, (53+3)*8
    csrrd a4, LOONGARCH_CSR_TMID
    st.d a4, a0, (54+3)*8
    csrrd a4, LOONGARCH_CSR_TCFG
    st.d a4, a0, (55+3)*8
    csrrd a4, LOONGARCH_CSR_TVAL
    st.d a4, a0, (56+3)*8
    csrrd a4, LOONGARCH_CSR_CNTC
    st.d a4, a0, (57+3)*8
    csrrd a4, LOONGARCH_CSR_TINTCLR
    st.d a4, a0, (58+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRENTRY
    st.d a4, a0, (59+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRBADV
    st.d a4, a0, (60+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRERA
    st.d a4, a0, (61+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRSAVE
    st.d a4, a0, (62+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRELO0
    st.d a4, a0, (63+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRELO1
    st.d a4, a0, (64+3)*8
    csrrd a4, LOONGARCH_CSR_TLBREHI
    st.d a4, a0, (65+3)*8
    csrrd a4, LOONGARCH_CSR_TLBRPRMD
    st.d a4, a0, (66+3)*8
    csrrd a4, LOONGARCH_CSR_DMWIN0
    st.d a4, a0, (67+3)*8
    csrrd a4, LOONGARCH_CSR_DMWIN1
    st.d a4, a0, (68+3)*8
    csrrd a4, LOONGARCH_CSR_DMWIN2
    st.d a4, a0, (69+3)*8
    csrrd a4, LOONGARCH_CSR_DMWIN3
    st.d a4, a0, (70+3)*8
    // space for secondary csrs (74 ~ 123 offset)
    ; st.d a3, a0, (71+3+50)*8// efi_system_table
    ret
    
.macro ZERO_ECFG_VS
//  save t0 & t1
    csrwr t0, LOONGARCH_CSR_TLBRSAVE
    csrwr t1, LOONGARCH_CSR_MERRERA
    
//  t0 = ECFG
    csrrd t0, LOONGARCH_CSR_ECFG

//  t1 = !(0x7 << 16)
    lu12i.w	t1, -113 
    ori	t1, t1, 4095 

//  t0 = ECFG & !(0x7 << 16)
    and	t0, t0, t1
//  ECFG = t0
    csrwr t0, LOONGARCH_CSR_ECFG

//  restore t0 & t1
    csrrd t0, LOONGARCH_CSR_TLBRSAVE
    csrrd t1, LOONGARCH_CSR_MERRERA
.endm

// PIL: load 操作页无效例外
FUNCTION(trap_table_entry1)
    ZERO_ECFG_VS
    ertn

// PIS: store 操作页无效例外
FUNCTION(trap_table_entry2)
    ZERO_ECFG_VS
    ertn

// PIF: 取指操作页无效例外
FUNCTION(trap_table_entry3)
    ZERO_ECFG_VS
    ertn

// PME: 页修改例外
FUNCTION(trap_table_entry4)
    ZERO_ECFG_VS
    ertn

// PNR: 页不可读例外
FUNCTION(trap_table_entry5)
    ZERO_ECFG_VS
    ertn

// PNX: 页不可执行例外
FUNCTION(trap_table_entry6)
    ZERO_ECFG_VS
    ertn

// PPI: 页特权等级不合规例外
FUNCTION(trap_table_entry7)
    ZERO_ECFG_VS
    ertn

// ADEF: 取指地址错例外    subcode :  0
// ADEM: 访存指令地址错例外 subcode :  1
FUNCTION(trap_table_entry8)
    ZERO_ECFG_VS
    ertn

// ALE: 地址非对齐例外
FUNCTION(trap_table_entry9)
    ZERO_ECFG_VS
    ertn

// BCE: 边界检查错例外
FUNCTION(trap_table_entry10)
    ZERO_ECFG_VS
    ertn

// SYS: 系统调用例外
FUNCTION(trap_table_entry11)
    ZERO_ECFG_VS
    ertn

// BRK: 断点例外
FUNCTION(trap_table_entry12)
    ZERO_ECFG_VS
    ertn

// INE: 指令不存在例外
FUNCTION(trap_table_entry13)
    ZERO_ECFG_VS
    ertn

// IPE: 指令特权等级错例外
FUNCTION(trap_table_entry14)
    ZERO_ECFG_VS
    ertn

// FPD: 浮点指令未使能例外
FUNCTION(trap_table_entry15)
    ZERO_ECFG_VS
    ertn

// SXD: 128 位向量扩展指令未使能例外
FUNCTION(trap_table_entry16)
    ZERO_ECFG_VS
    ertn

// ASXD: 256 位向量扩展指令未使能例外
FUNCTION(trap_table_entry17)
    ZERO_ECFG_VS
    ertn

// FPE:  基础浮点指令例外 esubcode: 0
// VFPE: 向量浮点指令例外 esubcode: 1
FUNCTION(trap_table_entry18)
    ZERO_ECFG_VS
    ertn

// WPEF:  取指监测点例外 esubcode: 0
// WPEFM: load/store 操作监测点例外 esubcode: 1
FUNCTION(trap_table_entry19)
    ZERO_ECFG_VS
    ertn

// BTD: 二进制翻译扩展指令未使能例外
FUNCTION(trap_table_entry20)
    ZERO_ECFG_VS
    ertn

// BTE: 二进制翻译相关例外
FUNCTION(trap_table_entry21)
    ZERO_ECFG_VS
    ertn

// GSPR: 客户机敏感特权资源例外
FUNCTION(trap_table_entry22)
//  save t0 & t1
    
    csrwr t0, LOONGARCH_CSR_TLBRSAVE
    csrrd t0, LOONGARCH_CSR_KS0
    
//  srli.d t0, t0, 1 // check CSR.SAVE0[1]
    beqz  t0, 32
    
    ori	t0, zero, 0
    csrwr t0, LOONGARCH_CSR_KS0 // special for IPI_CLEAR (WRITE)
     
    csrrd t0, LOONGARCH_CSR_ERA
    addi.d t0, t0, 4
    csrwr t0, LOONGARCH_CSR_ERA // ERA += 4
    
    csrrd t0, LOONGARCH_CSR_TLBRSAVE
    ertn

    csrwr t1, LOONGARCH_CSR_MERRERA

    // ========for count and overhead=======
//    rdtime.d t0, t1
//    csrwr t0, LOONGARCH_CSR_MERRSAVE // record start_time
    // ========for count and overhead=======

    csrrd t0, LOONGARCH_CSR_BADI
    lu12i.w	t1, 25736 // 105414656 : idle  0b0000_0110_0100_1000_1000_0000_0000_0000
    sub.d t0, t0, t1  // t0 - t1 -> t0
//    bnez  t0, 64   // 64 / 4  -> 16 条指令 (t0 != 0 jump) count + overhead
//    bnez  t0, 40 // 40 / 4    -> 10 条指令 (t0 != 0 jump) count 
    bnez  t0, 28 // 28 / 4    -> 7  条指令 (t0 != 0 jump)

//  ======record idle count======
//    csrrd  t0, LOONGARCH_CSR_KS7
//    addi.d t0, t0, 1
//    csrwr  t0, LOONGARCH_CSR_KS7

//  ======record idle overhead===

//    rdtime.d t0, t1  // t0 : time_end
//    csrrd t1, LOONGARCH_CSR_MERRSAVE  // t1 : time_start
//    sub.d t0, t0, t1 // time_end - time_start

//    csrrd t1, LOONGARCH_CSR_KS2
//    add.d t1, t1, t0 
//    csrwr t1, LOONGARCH_CSR_KS2 // overhead += (time_end - time_start)

    csrrd t0, LOONGARCH_CSR_ERA
    addi.d t0, t0, 4
    csrwr t0, LOONGARCH_CSR_ERA // ERA += 4

//  restore t0 & t1
    csrrd t0, LOONGARCH_CSR_TLBRSAVE
    csrrd t1, LOONGARCH_CSR_MERRERA

    ertn // IDLE, return directly

//  t0 = ECFG
    csrrd t0, LOONGARCH_CSR_ECFG

//  t1 = !(0x7 << 16)
    lu12i.w	t1, -113 
    ori	t1, t1, 4095 

//  t0 = ECFG & !(0x7 << 16)
    and	t0, t0, t1
//  ECFG = t0
    csrwr t0, LOONGARCH_CSR_ECFG

//  restore t0 & t1
    csrrd t0, LOONGARCH_CSR_TLBRSAVE
    csrrd t1, LOONGARCH_CSR_MERRERA
    
    ertn

// HVC: 虚拟机监控调用例外
FUNCTION(trap_table_entry23)
    ZERO_ECFG_VS
    ertn

// GCSC: 客户机 CSR 软件修改例外 esubcode: 0
// GCHC: 客户机 CSR 硬件修改例外 esubcode: 1
FUNCTION(trap_table_entry24)
    ZERO_ECFG_VS    
    ertn

// invalid
FUNCTION(trap_table_entry25)
    ZERO_ECFG_VS
    ertn

// SWI0
FUNCTION(trap_table_entry64)
    ZERO_ECFG_VS
    ertn

// SWI1
FUNCTION(trap_table_entry65)
    ZERO_ECFG_VS
    ertn

// HWI0
FUNCTION(trap_table_entry66)
    ZERO_ECFG_VS
    ertn

// HWI1
FUNCTION(trap_table_entry67)
    ZERO_ECFG_VS
    ertn

// HWI2
FUNCTION(trap_table_entry68)
    ZERO_ECFG_VS
    ertn

// HWI3
FUNCTION(trap_table_entry69)
    ZERO_ECFG_VS
    ertn

// HWI4
FUNCTION(trap_table_entry70)
    ZERO_ECFG_VS
    ertn

// HWI5
FUNCTION(trap_table_entry71)
    ZERO_ECFG_VS
    ertn

// HWI6
FUNCTION(trap_table_entry72)
    ZERO_ECFG_VS
    ertn

// HWI7
FUNCTION(trap_table_entry73)
    ZERO_ECFG_VS
    ertn

// PMI
FUNCTION(trap_table_entry74)
    ZERO_ECFG_VS
    ertn
 
// TI
FUNCTION(trap_table_entry75)
// csrrd  t0, LOONGARCH_CSR_TLBRSAVE
// addi.d t0, t0, 1
// csrwr  t0, LOONGARCH_CSR_TLBRSAVE

    csrwr t0, LOONGARCH_CSR_TLBRSAVE
    addi.d t0, zero, 1
    csrwr t0, LOONGARCH_CSR_TINTCLR
    csrrd t0, LOONGARCH_CSR_TLBRSAVE
    ertn

    // disable TI interrupt handler
    ZERO_ECFG_VS
    ertn

// IPI
FUNCTION(trap_table_entry76)
    ZERO_ECFG_VS
    ertn

// END: not used
FUNCTION(trap_table_entry77)
    ZERO_ECFG_VS
    ertn