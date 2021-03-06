/* This file is a part of NXVM project. */

#include "stdio.h"
#include "memory.h"

#include "vapi.h"

#include "vport.h"
#include "vram.h"
#include "vcpu.h"
#include "vpic.h"
#include "bios/qdbios.h"
#include "vcpuins.h"

#include "debug/aasm.h"
#include "debug/dasm.h"

#define MOD ((modrm&0xc0)>>6)
#define REG ((modrm&0x38)>>3)
#define RM  ((modrm&0x07)>>0)

#define ADD_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF)
#define	 OR_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define ADC_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF)
#define SBB_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF)
#define AND_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define SUB_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF)
#define XOR_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define CMP_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF)
#define INC_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_PF)
#define DEC_FLAG  (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_PF)
#define TEST_FLAG (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define SHL_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define SHR_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define SAL_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define SAR_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define AAM_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define AAD_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define DAA_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)
#define DAS_FLAG  (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | VCPU_EFLAGS_PF)

t_cpuins vcpuins;
t_cpurec vcpurec;

static t_vaddrcc rimm;

#define bugfix(n) if(1)

#define CHECKED void
#define DIFF void
#define NOTIMP void
#define WRAPPER void

#define ASMCMP void
#define EXCEPT void

static t_cpu acpu;
static t_nubit8 ub1,ub2,ub3;
static t_nsbit8 sb1,sb2,sb3;
static t_nubit16 uw1,uw2,uw3;
static t_nsbit16 sw1,sw2,sw3;
#define aipcheck if (acpu.ip > 0xfffa && vcpu.ip < 0x0005) {\
	vapiPrint("ip overflow\n");} else
#ifdef VCPUASM

#define AFLAGS0   (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF | \
                   VCPU_EFLAGS_DF | VCPU_EFLAGS_TF | VCPU_EFLAGS_IF)
#define AFLAGS1   (VCPU_EFLAGS_OF | VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF | \
                   VCPU_EFLAGS_DF | VCPU_EFLAGS_TF)
#define AFLAGS2   (VCPU_EFLAGS_SF | VCPU_EFLAGS_ZF | \
                   VCPU_EFLAGS_AF | VCPU_EFLAGS_CF | VCPU_EFLAGS_PF | \
                   VCPU_EFLAGS_DF | VCPU_EFLAGS_TF)

static void acpuPrintRegs(t_cpu *xcpu) {
    if (xcpu == &vcpu) vapiPrint("VCPU REGISTERS\n");
    else               vapiPrint("ACPU REGISTERS\n");
    vapiPrint(  "AX=%04X", xcpu->ax);
    vapiPrint("  BX=%04X", xcpu->bx);
    vapiPrint("  CX=%04X", xcpu->cx);
    vapiPrint("  DX=%04X", xcpu->dx);
    vapiPrint("  SP=%04X", xcpu->sp);
    vapiPrint("  BP=%04X", xcpu->bp);
    vapiPrint("  SI=%04X", xcpu->si);
    vapiPrint("  DI=%04X", xcpu->di);
    vapiPrint("\nDS=%04X", xcpu->ds);
    vapiPrint("  ES=%04X", xcpu->es);
    vapiPrint("  SS=%04X", xcpu->ss);
    vapiPrint("  CS=%04X", xcpu->cs);
    vapiPrint("  IP=%04X", xcpu->ip);
    vapiPrint("   ");
    if (xcpu->flags & VCPU_EFLAGS_OF) vapiPrint("OV ");
    else                           vapiPrint("NV ");
    if (xcpu->flags & VCPU_EFLAGS_DF) vapiPrint("DN ");
    else                           vapiPrint("UP ");
    if (xcpu->flags & VCPU_EFLAGS_IF) vapiPrint("E_ ");
    else                           vapiPrint("D_ ");
    if (xcpu->flags & VCPU_EFLAGS_SF) vapiPrint("NG ");
    else                           vapiPrint("PL ");
    if (xcpu->flags & VCPU_EFLAGS_ZF) vapiPrint("ZR ");
    else                           vapiPrint("NZ ");
    if (xcpu->flags & VCPU_EFLAGS_AF) vapiPrint("AC ");
    else                           vapiPrint("NA ");
    if (xcpu->flags & VCPU_EFLAGS_PF) vapiPrint("PE ");
    else                           vapiPrint("PO ");
    if (xcpu->flags & VCPU_EFLAGS_CF) vapiPrint("CY ");
    else                           vapiPrint("NC ");
    vapiPrint("\n");
}
static t_bool acpuCheck(t_nubit16 flags) {
    t_bool flagdiff = 0x00;
    if (acpu.ax != vcpu.ax)       {
        vapiPrint("diff ax\n");
        flagdiff = 0x01;
    }
    if (acpu.bx != vcpu.bx)       {
        vapiPrint("diff bx\n");
        flagdiff = 0x01;
    }
    if (acpu.cx != vcpu.cx)       {
        vapiPrint("diff cx\n");
        flagdiff = 0x01;
    }
    if (acpu.dx != vcpu.dx)       {
        vapiPrint("diff dx\n");
        flagdiff = 0x01;
    }
    if (acpu.sp != vcpu.sp)       {
        vapiPrint("diff sp\n");
        flagdiff = 0x01;
    }
    if (acpu.bp != vcpu.bp)       {
        vapiPrint("diff bp\n");
        flagdiff = 0x01;
    }
    if (acpu.si != vcpu.si)       {
        vapiPrint("diff si\n");
        flagdiff = 0x01;
    }
    if (acpu.di != vcpu.di)       {
        vapiPrint("diff di\n");
        flagdiff = 0x01;
    }
    if (acpu.ip != vcpu.ip)       {
        vapiPrint("diff ip\n");
        flagdiff = 0x01;
    }
    if (acpu.cs != vcpu.cs.selector)       {
        vapiPrint("diff cs\n");
        flagdiff = 0x01;
    }
    if (acpu.ds != vcpu.ds.selector)       {
        vapiPrint("diff ds\n");
        flagdiff = 0x01;
    }
    if (acpu.es != vcpu.es.selector)       {
        vapiPrint("diff es\n");
        flagdiff = 0x01;
    }
    if (acpu.ss != vcpu.ss.selector)       {
        vapiPrint("diff ss\n");
        flagdiff = 0x01;
    }
    if ((acpu.flags & flags) != (vcpu.flags & flags)) {
        vapiPrint("diff fg\n");
        flagdiff = 0x01;
    }
    if (flagdiff) {
        acpuPrintRegs(&acpu);
        acpuPrintRegs(&vcpu);
        vapiCallBackMachineStop();
    }
    return flagdiff;
}
#define async    if(acpu = vcpu,1)
#define aexec    __asm
#define acheck(f)   if(acpuCheck(f))
#define async8   if(1) {\
	acpu = vcpu;ub1 = d_nubit8(dest);ub2 = d_nubit8(src);} else
#define async12  if(1) {\
	acpu = vcpu;uw1 = d_nubit16(dest);uw2 = d_nsbit8(src);} else
#define async16  if(1) {\
	acpu = vcpu;uw1 = d_nubit16(dest);uw2 = d_nubit16(src);} else
#define asreg8   if(!vramIsAddrInMem(dest)) {\
	d_nubit8((t_vaddrcc)dest - (t_vaddrcc)&vcpu + (t_vaddrcc)&acpu) = ub3;} else
#define asreg16  if(!vramIsAddrInMem(dest)) {\
	d_nubit16((t_vaddrcc)dest - (t_vaddrcc)&vcpu + (t_vaddrcc)&acpu) = uw3;} else
#define asreg12  asreg16
#define acheck8(f)  if(acpuCheck(f) || d_nubit8(dest) != ub3) {\
	vapiPrint("ub1=%02X,ub2=%02X,ub3=%02X,dest=%02X\n",\
		ub1,ub2,ub3,d_nubit8(dest));} else
#define acheck16(f) if(acpuCheck(f) || d_nubit16(dest) != uw3) {\
	vapiPrint("uw1=%04X,uw2=%04X,uw3=%04X,dest=%04X\n",\
		uw1,uw2,uw3,d_nubit16(dest));} else
#define acheck12(f) acheck16(f)
#define aexec8  if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push acpu.flags\
	__asm popf\
	__asm mov al,ub1\
	__asm op  al,ub2\
	__asm mov ub3,al\
	__asm pushf\
	__asm pop acpu.flags\
	__asm pop eax\
	__asm popfd} else
#define aexec16  if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push acpu.flags\
	__asm popf\
	__asm mov ax,uw1\
	__asm op  ax,uw2\
	__asm mov uw3,ax\
	__asm pushf\
	__asm pop acpu.flags\
	__asm pop eax\
	__asm popfd} else
#define aexec12 aexec16
#define asyncall switch(len) {\
	case 8:  async8; break;\
	case 12: async12;break;\
	case 16: async16;break;}
#define aexecall switch(len) {\
	case 8: aexec8; break;\
	case 12:aexec12;break;\
	case 16:aexec16;break;}
#define asregall switch(len) {\
	case 8: asreg8; break;\
	case 12:asreg12;break;\
	case 16:asreg16;break;}
#define acheckall(f) switch(len) {\
	case 8: acheck8(f); break;\
	case 12:acheck12(f);break;\
	case 16:acheck16(f);break;}

#define aexec_csf if(1) {\
	__asm pushfd\
	__asm push acpu.flags\
	__asm popf\
	__asm op\
	__asm pushf\
	__asm pop acpu.flags\
	__asm popfd} else
#define aexec_idc8 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push acpu.flags\
	__asm popf\
	__asm mov al,ub1\
	__asm op al\
	__asm mov ub3,al\
	__asm pushf\
	__asm pop acpu.flags\
	__asm pop eax\
	__asm popfd} else
#define aexec_idc16 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push acpu.flags\
	__asm popf\
	__asm mov ax,uw1\
	__asm op ax\
	__asm mov uw3,ax\
	__asm pushf\
	__asm pop acpu.flags\
	__asm pop eax\
	__asm popfd} else
#define aexec_srd8 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push ecx\
	__asm push acpu.flags\
	__asm popf\
	__asm mov al,ub1\
	__asm mov cl,ub2\
	__asm op al,cl\
	__asm mov ub3,al\
	__asm pushf\
	__asm pop acpu.flags\
	__asm pop ecx\
	__asm pop eax\
	__asm popfd} else
#define aexec_srd16 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push ecx\
	__asm push acpu.flags\
	__asm popf\
	__asm mov ax,uw1\
	__asm mov cl,ub2\
	__asm op ax,cl\
	__asm mov uw3,ax\
	__asm pushf\
	__asm pop acpu.flags\
	__asm pop ecx\
	__asm pop eax\
	__asm popfd} else
#define aexec_cax8 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm mov al, acpu.al\
	__asm push acpu.flags\
	__asm popf\
	__asm op\
	__asm pushf\
	__asm pop acpu.flags\
	__asm mov acpu.al, al\
	__asm pop eax\
	__asm popfd} else
#define aexec_cax16 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm push edx\
	__asm mov ax, acpu.ax\
	__asm mov dx, acpu.dx\
	__asm push acpu.flags\
	__asm popf\
	__asm op\
	__asm pushf\
	__asm pop acpu.flags\
	__asm mov acpu.dx, dx\
	__asm mov acpu.ax, ax\
	__asm pop edx\
	__asm pop eax\
	__asm popfd} else
#define aexec_nxx8 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm mov al, ub1\
	__asm push acpu.flags\
	__asm popf\
	__asm op al\
	__asm pushf\
	__asm pop acpu.flags\
	__asm mov ub3, al\
	__asm pop eax\
	__asm popfd} else
#define aexec_nxx16 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm mov ax, uw1\
	__asm push acpu.flags\
	__asm popf\
	__asm op ax\
	__asm pushf\
	__asm pop acpu.flags\
	__asm mov uw3, ax\
	__asm pop eax\
	__asm popfd} else
#define aexec_mdx8 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm mov ax, acpu.ax\
	__asm push acpu.flags\
	__asm popf\
	__asm op ub2\
	__asm pushf\
	__asm pop acpu.flags\
	__asm mov acpu.ax, ax\
	__asm pop eax\
	__asm popfd} else
#define aexec_mdx16 if(1) {\
	__asm pushfd\
	__asm push eax\
	__asm mov ax, acpu.ax\
	__asm mov dx, acpu.dx\
	__asm push acpu.flags\
	__asm popf\
	__asm op uw2\
	__asm pushf\
	__asm pop acpu.flags\
	__asm mov acpu.dx, dx\
	__asm mov acpu.ax, ax\
	__asm pop eax\
	__asm popfd} else
#else
#define async     if(0)
#define aexec     __asm
#define acheck(f) if(0)
#define async8    if(0)
#define async12   if(0)
#define async16   if(0)
#define asreg8    if(0)
#define asreg12   if(0)
#define asreg16   if(0)
#define acheck8(f)  if(0)
#define acheck12(f) if(0)
#define acheck16(f) if(0)
#define aexec8    if(0)
#define aexec12   if(0)
#define aexec16   if(0)
#define asyncall
#define aexecall
#define asregall
#define acheckall(f)
#define aexec_csf   if(0)
#define aexec_idc8  if(0)
#define aexec_idc16 if(0)
#define aexec_srd8  if(0)
#define aexec_srd16 if(0)
#define aexec_cax8  if(0)
#define aexec_cax16 if(0)
#define aexec_nxx8  if(0)
#define aexec_nxx16 if(0)
#define aexec_mdx8  if(0)
#define aexec_mdx16 if(0)
#endif

static void CaseError(const char *str) {
    vapiPrint("The NXVM CPU has encountered an internal case error: %s.\n",str);
    vapiCallBackMachineStop();
}
static void CalcCF() {
    switch (vcpuins.type) {
    case ADD8:
    case ADD16:
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,(vcpuins.result < vcpuins.opr1) || (vcpuins.result < vcpuins.opr2));
        break;
    case ADC8:
        bugfix(21)
        MakeBit(vcpu.flags, VCPU_EFLAGS_CF, (
                    (GetBit(vcpu.flags, VCPU_EFLAGS_CF) && vcpuins.opr2 == 0xff) ?
                    1 : ((vcpuins.result < vcpuins.opr1) || (vcpuins.result < vcpuins.opr2))));
        else
            bugfix(1) MakeBit(vcpu.flags,VCPU_EFLAGS_CF,vcpuins.result < vcpuins.opr1);
        else MakeBit(vcpu.flags,VCPU_EFLAGS_CF,vcpuins.result <= vcpuins.opr1);
        break;
    case ADC16:
        bugfix(21)
        MakeBit(vcpu.flags, VCPU_EFLAGS_CF, (
                    (GetBit(vcpu.flags, VCPU_EFLAGS_CF) && vcpuins.opr2 == 0xffff) ?
                    1 : ((vcpuins.result < vcpuins.opr1) || (vcpuins.result < vcpuins.opr2))));
        else
            bugfix(1) MakeBit(vcpu.flags,VCPU_EFLAGS_CF,vcpuins.result < vcpuins.opr1);
        else MakeBit(vcpu.flags,VCPU_EFLAGS_CF,vcpuins.result <= vcpuins.opr1);
        break;
    case SBB8:
        bugfix(20)
        MakeBit(vcpu.flags, VCPU_EFLAGS_CF, (vcpuins.opr1 < vcpuins.result) ||
                (GetBit(vcpu.flags, VCPU_EFLAGS_CF) && (vcpuins.opr2 == 0xff)));
        else
            MakeBit(vcpu.flags, VCPU_EFLAGS_CF, (vcpuins.opr1 < vcpuins.result) ||
                    (vcpuins.opr2 == 0xff));
        break;
    case SBB16:
        bugfix(20)
        MakeBit(vcpu.flags, VCPU_EFLAGS_CF, (vcpuins.opr1 < vcpuins.result) ||
                (GetBit(vcpu.flags, VCPU_EFLAGS_CF) && (vcpuins.opr2 == 0xffff)));
        else
            MakeBit(vcpu.flags, VCPU_EFLAGS_CF, (vcpuins.opr1 < vcpuins.result) ||
                    (vcpuins.opr2 == 0xffff));
        break;
    case SUB8:
    case SUB16:
    case CMP8:
    case CMP16:
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,vcpuins.opr1 < vcpuins.opr2);
        break;
    default:
        CaseError("CalcCF::vcpuins.type");
        break;
    }
}
static void CalcOF() {
    switch (vcpuins.type) {
    case ADD8:
    case ADC8:
        MakeBit(vcpu.flags,VCPU_EFLAGS_OF,((vcpuins.opr1&0x0080) == (vcpuins.opr2&0x0080)) && ((vcpuins.opr1&0x0080) != (vcpuins.result&0x0080)));
        break;
    case ADD16:
    case ADC16:
        MakeBit(vcpu.flags,VCPU_EFLAGS_OF,((vcpuins.opr1&0x8000) == (vcpuins.opr2&0x8000)) && ((vcpuins.opr1&0x8000) != (vcpuins.result&0x8000)));
        break;
    case SBB8:
    case SUB8:
    case CMP8:
        MakeBit(vcpu.flags,VCPU_EFLAGS_OF,((vcpuins.opr1&0x0080) != (vcpuins.opr2&0x0080)) && ((vcpuins.opr2&0x0080) == (vcpuins.result&0x0080)));
        break;
    case SBB16:
    case SUB16:
    case CMP16:
        MakeBit(vcpu.flags,VCPU_EFLAGS_OF,((vcpuins.opr1&0x8000) != (vcpuins.opr2&0x8000)) && ((vcpuins.opr2&0x8000) == (vcpuins.result&0x8000)));
        break;
    default:
        CaseError("CalcOF::vcpuins.type");
        break;
    }
}
static void CalcAF() {
    MakeBit(vcpu.flags,VCPU_EFLAGS_AF,((vcpuins.opr1^vcpuins.opr2)^vcpuins.result)&0x10);
}
static void CalcPF() {
    t_nubit8 res8 = vcpuins.result & 0xff;
    t_nubitcc count = 0;
    while (res8) {
        res8 &= res8-1;
        count++;
    }
    MakeBit(vcpu.flags,VCPU_EFLAGS_PF,!(count&0x01));
}
static void CalcZF() {
    MakeBit(vcpu.flags,VCPU_EFLAGS_ZF,!vcpuins.result);
}
static void CalcSF() {
    switch (vcpuins.bit) {
    case 8:
        MakeBit(vcpu.flags,VCPU_EFLAGS_SF,!!(vcpuins.result&0x80));
        break;
    case 16:
        MakeBit(vcpu.flags,VCPU_EFLAGS_SF,!!(vcpuins.result&0x8000));
        break;
    default:
        CaseError("CalcSF::vcpuins.bit");
        break;
    }
}
static void CalcTF() {}
static void CalcIF() {}
static void CalcDF() {}

static void SetFlags(t_nubit16 flags) {
    if (flags & VCPU_EFLAGS_CF) CalcCF();
    if (flags & VCPU_EFLAGS_PF) CalcPF();
    if (flags & VCPU_EFLAGS_AF) CalcAF();
    if (flags & VCPU_EFLAGS_ZF) CalcZF();
    if (flags & VCPU_EFLAGS_SF) CalcSF();
    if (flags & VCPU_EFLAGS_TF) CalcTF();
    if (flags & VCPU_EFLAGS_IF) CalcIF();
    if (flags & VCPU_EFLAGS_DF) CalcDF();
    if (flags & VCPU_EFLAGS_OF) CalcOF();
}
static void GetMem() {
    /* returns vcpuins.rrm */
    vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vramRealWord(vcpu.cs.selector,vcpu.ip));
    vcpu.ip += 2;
}
static void GetImm(t_nubitcc immbit) {
    // returns rimm
    rimm = vramGetRealAddr(vcpu.cs.selector,vcpu.ip);
    switch (immbit) {
    case 8:
        vcpu.ip += 1;
        break;
    case 16:
        vcpu.ip += 2;
        break;
    case 32:
        vcpu.ip += 4;
        break;
    default:
        CaseError("GetImm::immbit");
        break;
    }
}
static void GetModRegRM(t_nubitcc regbit,t_nubitcc rmbit) {
    // returns vcpuins.rrm and vcpuins.rr
    t_nsbit8 disp8 = 0x00;
    t_nubit16 disp16 = 0x0000;
    t_nubit8 modrm = vramRealByte(vcpu.cs.selector,vcpu.ip++);
    vcpuins.rrm = vcpuins.rr = (t_vaddrcc)NULL;
    switch (MOD) {
    case 0:
        switch (RM) {
        case 0:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+vcpu.si);
            break;
        case 1:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+vcpu.di);
            break;
        case 2:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+vcpu.si);
            break;
        case 3:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+vcpu.di);
            break;
        case 4:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.si);
            break;
        case 5:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.di);
            break;
        case 6:
            disp16 = vramRealWord(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 2;
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,disp16);
            break;
        case 7:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx);
            break;
        default:
            CaseError("GetModRegRM::MOD0::RM");
            break;
        }
        break;
    case 1:
        bugfix(23) {
            disp8 = (t_nsbit8)vramRealByte(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 1;
        }
        switch (RM) {
        case 0:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+vcpu.si+disp8);
            break;
        case 1:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+vcpu.di+disp8);
            break;
        case 2:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+vcpu.si+disp8);
            break;
        case 3:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+vcpu.di+disp8);
            break;
        case 4:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.si+disp8);
            break;
        case 5:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.di+disp8);
            break;
        case 6:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+disp8);
            break;
        case 7:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+disp8);
            break;
        default:
            CaseError("GetModRegRM::MOD1::RM");
            break;
        }
        bugfix(23) ;
        else {
            bugfix(3) vcpuins.rrm += (t_nsbit8)vramRealByte(vcpu.cs.selector,vcpu.ip);
            else vcpuins.rrm += vramRealByte(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 1;
        }
        break;
    case 2:
        bugfix(23) {
            disp16 = vramRealWord(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 2;
        }
        switch (RM) {
        case 0:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+vcpu.si+disp16);
            break;
        case 1:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+vcpu.di+disp16);
            break;
        case 2:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+vcpu.si+disp16);
            break;
        case 3:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+vcpu.di+disp16);
            break;
        case 4:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.si+disp16);
            break;
        case 5:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.di+disp16);
            break;
        case 6:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp+disp16);
            break;
        case 7:
            vcpuins.rrm = vramGetRealAddr(vcpuins.roverds->selector,vcpu.bx+disp16);
            break;
        default:
            CaseError("GetModRegRM::MOD2::RM");
            break;
        }
        bugfix(23) ;
        else {
            vcpuins.rrm += vramRealWord(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 2;
        }
        break;
    case 3:
        switch (RM) {
        case 0:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.al);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.ax);
            break;
        case 1:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.cl);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.cx);
            break;
        case 2:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.dl);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.dx);
            break;
        case 3:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.bl);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.bx);
            break;
        case 4:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.ah);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.sp);
            break;
        case 5:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.ch);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.bp);
            break;
        case 6:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.dh);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.si);
            break;
        case 7:
            if (rmbit == 8) vcpuins.rrm = (t_vaddrcc)(&vcpu.bh);
            else vcpuins.rrm = (t_vaddrcc)(&vcpu.di);
            break;
        default:
            CaseError("GetModRegRM::MOD3::RM");
            break;
        }
        break;
    default:
        CaseError("GetModRegRM::MOD");
        break;
    }
    switch (regbit) {
    case 0:
        vcpuins.rr = REG;
        break;
    case 4:
        switch (REG) {
        case 0:
            vcpuins.rr = (t_vaddrcc)(&vcpu.es.selector);
            break;
        case 1:
            vcpuins.rr = (t_vaddrcc)(&vcpu.cs.selector);
            break;
        case 2:
            vcpuins.rr = (t_vaddrcc)(&vcpu.ss.selector);
            break;
        case 3:
            vcpuins.rr = (t_vaddrcc)(&vcpu.ds.selector);
            break;
        default:
            CaseError("GetModRegRM::regbit4::REG");
            break;
        }
        break;
    case 8:
        switch (REG) {
        case 0:
            vcpuins.rr = (t_vaddrcc)(&vcpu.al);
            break;
        case 1:
            vcpuins.rr = (t_vaddrcc)(&vcpu.cl);
            break;
        case 2:
            vcpuins.rr = (t_vaddrcc)(&vcpu.dl);
            break;
        case 3:
            vcpuins.rr = (t_vaddrcc)(&vcpu.bl);
            break;
        case 4:
            vcpuins.rr = (t_vaddrcc)(&vcpu.ah);
            break;
        case 5:
            vcpuins.rr = (t_vaddrcc)(&vcpu.ch);
            break;
        case 6:
            vcpuins.rr = (t_vaddrcc)(&vcpu.dh);
            break;
        case 7:
            vcpuins.rr = (t_vaddrcc)(&vcpu.bh);
            break;
        default:
            CaseError("GetModRegRM::regbit8::REG");
            break;
        }
        break;
    case 16:
        switch (REG) {
        case 0:
            vcpuins.rr = (t_vaddrcc)(&vcpu.ax);
            break;
        case 1:
            vcpuins.rr = (t_vaddrcc)(&vcpu.cx);
            break;
        case 2:
            vcpuins.rr = (t_vaddrcc)(&vcpu.dx);
            break;
        case 3:
            vcpuins.rr = (t_vaddrcc)(&vcpu.bx);
            break;
        case 4:
            vcpuins.rr = (t_vaddrcc)(&vcpu.sp);
            break;
        case 5:
            vcpuins.rr = (t_vaddrcc)(&vcpu.bp);
            break;
        case 6:
            vcpuins.rr = (t_vaddrcc)(&vcpu.si);
            break;
        case 7:
            vcpuins.rr = (t_vaddrcc)(&vcpu.di);
            break;
        default:
            CaseError("GetModRegRM::regbit16::REG");
            break;
        }
        break;
    default:
        CaseError("GetModRegRM::regbit");
        break;
    }
}
static void GetModRegRMEA() {
    t_nsbit8 disp8 = 0x00;
    t_nubit16 disp16 = 0x0000;
    t_nubit8 modrm = vramRealByte(vcpu.cs.selector,vcpu.ip++);
    vcpuins.rrm = vcpuins.rr = (t_vaddrcc)NULL;
    switch (MOD) {
    case 0:
        switch (RM) {
        case 0:
            vcpuins.rrm = vcpu.bx+vcpu.si;
            break;
        case 1:
            vcpuins.rrm = vcpu.bx+vcpu.di;
            break;
        case 2:
            vcpuins.rrm = vcpu.bp+vcpu.si;
            break;
        case 3:
            vcpuins.rrm = vcpu.bp+vcpu.di;
            break;
        case 4:
            vcpuins.rrm = vcpu.si;
            break;
        case 5:
            vcpuins.rrm = vcpu.di;
            break;
        case 6:
            vcpuins.rrm = vramRealWord(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 2;
            break;
        case 7:
            vcpuins.rrm = vcpu.bx;
            break;
        default:
            CaseError("GetModRegRMEA::MOD0::RM");
            break;
        }
        break;
    case 1:
        bugfix(23) {
            disp8 = (t_nsbit8)vramRealByte(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 1;
        }
        switch (RM) {
        case 0:
            vcpuins.rrm = vcpu.bx+vcpu.si+disp8;
            break;
        case 1:
            vcpuins.rrm = vcpu.bx+vcpu.di+disp8;
            break;
        case 2:
            vcpuins.rrm = vcpu.bp+vcpu.si+disp8;
            break;
        case 3:
            vcpuins.rrm = vcpu.bp+vcpu.di+disp8;
            break;
        case 4:
            vcpuins.rrm = vcpu.si+disp8;
            break;
        case 5:
            vcpuins.rrm = vcpu.di+disp8;
            break;
        case 6:
            vcpuins.rrm = vcpu.bp+disp8;
            break;
        case 7:
            vcpuins.rrm = vcpu.bx+disp8;
            break;
        default:
            CaseError("GetModRegRMEA::MOD1::RM");
            break;
        }
        bugfix(23) {
            vcpuins.rrm %= 0x10000;
        }
        else {
            bugfix(3) vcpuins.rrm += (t_nsbit8)vramRealByte(vcpu.cs.selector,vcpu.ip);
            else vcpuins.rrm += vramRealByte(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 1;
        }
        break;
    case 2:
        bugfix(23) {
            disp16 = vramRealWord(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 2;
        }
        switch (RM) {
        case 0:
            vcpuins.rrm = vcpu.bx+vcpu.si+disp16;
            break;
        case 1:
            vcpuins.rrm = vcpu.bx+vcpu.di+disp16;
            break;
        case 2:
            vcpuins.rrm = vcpu.bp+vcpu.si+disp16;
            break;
        case 3:
            vcpuins.rrm = vcpu.bp+vcpu.di+disp16;
            break;
        case 4:
            vcpuins.rrm = vcpu.si+disp16;
            break;
        case 5:
            vcpuins.rrm = vcpu.di+disp16;
            break;
        case 6:
            bugfix(14) vcpuins.rrm = vcpu.bp+disp16;
            else vcpuins.rrm = vramGetRealAddr(vcpuins.roverss->selector,vcpu.bp);
            break;
        case 7:
            vcpuins.rrm = vcpu.bx+disp16;
            break;
        default:
            CaseError("GetModRegRMEA::MOD2::RM");
            break;
        }
        bugfix(23) {
            vcpuins.rrm %= 0x10000;
        }
        else {
            vcpuins.rrm += vramRealWord(vcpu.cs.selector,vcpu.ip);
            vcpu.ip += 2;
        }
        break;
    default:
        CaseError("GetModRegRMEA::MOD");
        break;
    }
    switch (REG) {
    case 0:
        vcpuins.rr = (t_vaddrcc)(&vcpu.ax);
        break;
    case 1:
        vcpuins.rr = (t_vaddrcc)(&vcpu.cx);
        break;
    case 2:
        vcpuins.rr = (t_vaddrcc)(&vcpu.dx);
        break;
    case 3:
        vcpuins.rr = (t_vaddrcc)(&vcpu.bx);
        break;
    case 4:
        vcpuins.rr = (t_vaddrcc)(&vcpu.sp);
        break;
    case 5:
        vcpuins.rr = (t_vaddrcc)(&vcpu.bp);
        break;
    case 6:
        vcpuins.rr = (t_vaddrcc)(&vcpu.si);
        break;
    case 7:
        vcpuins.rr = (t_vaddrcc)(&vcpu.di);
        break;
    default:
        CaseError("GetModRegRMEA::REG");
        break;
    }
}

static void INT(t_nubit8 intid);
static ASMCMP ADD(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = ADD8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        bugfix(6) break;
        else ;
    case 12:
        vcpuins.bit = 16;
        vcpuins.type = ADD16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        bugfix(22) vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        else vcpuins.opr2 = d_nsbit8(src); /* in this case opr2 could be 0xffffffff */
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = ADD16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("ADD::len");
        break;
    }
    SetFlags(ADD_FLAG);
#define op add
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP OR(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        //vcpuins.type = OR8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1|vcpuins.opr2) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 12:
        vcpuins.bit = 16;
        //vcpuins.type = OR16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1|vcpuins.opr2) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        //vcpuins.type = OR16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1|vcpuins.opr2) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("OR::len");
        break;
    }
    ClrBit(vcpu.flags, VCPU_EFLAGS_OF);
    ClrBit(vcpu.flags, VCPU_EFLAGS_CF);
    ClrBit(vcpu.flags, VCPU_EFLAGS_AF);
    SetFlags(OR_FLAG);
#define op or
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP ADC(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = ADC8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2+GetBit(vcpu.flags, VCPU_EFLAGS_CF)) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 12:
        vcpuins.bit = 16;
        vcpuins.type = ADC16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2+GetBit(vcpu.flags, VCPU_EFLAGS_CF)) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = ADC16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2+GetBit(vcpu.flags, VCPU_EFLAGS_CF)) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("ADC::len");
        break;
    }
    SetFlags(ADC_FLAG);
#define op adc
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP SBB(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = SBB8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1-(vcpuins.opr2+GetBit(vcpu.flags, VCPU_EFLAGS_CF))) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 12:
        vcpuins.bit = 16;
        vcpuins.type = SBB16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1-(vcpuins.opr2+GetBit(vcpu.flags, VCPU_EFLAGS_CF))) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = SBB16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1-(vcpuins.opr2+GetBit(vcpu.flags, VCPU_EFLAGS_CF))) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("SBB::len");
        break;
    }
    SetFlags(SBB_FLAG);
#define op sbb
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP AND(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        //vcpuins.type = AND8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1&vcpuins.opr2) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 12:
        vcpuins.bit = 16;
        //vcpuins.type = AND16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1&vcpuins.opr2) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        //vcpuins.type = AND16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1&vcpuins.opr2) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("AND::len");
        break;
    }
    ClrBit(vcpu.flags,VCPU_EFLAGS_OF);
    ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
    ClrBit(vcpu.flags,VCPU_EFLAGS_AF);
    SetFlags(AND_FLAG);
#define op and
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static EXCEPT SUB(void *dest, void *src, t_nubit8 len) {
//	asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = SUB8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 12:
        vcpuins.bit = 16;
        vcpuins.type = SUB16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = SUB16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("SUB::len");
        break;
    }
    SetFlags(SUB_FLAG);
//#define op sub
//	aexecall
//#undef op
//	asregall
//	acheckall(AFLAGS1)
}
static ASMCMP XOR(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        //vcpuins.type = XOR8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1^vcpuins.opr2)&0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 12:
        vcpuins.bit = 16;
        //vcpuins.type = XOR16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1^vcpuins.opr2)&0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    case 16:
        vcpuins.bit = 16;
        //vcpuins.type = XOR16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1^vcpuins.opr2)&0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("XOR::len");
        break;
    }
    ClrBit(vcpu.flags,VCPU_EFLAGS_OF);
    ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
    ClrBit(vcpu.flags,VCPU_EFLAGS_AF);
    SetFlags(XOR_FLAG);
#define op xor
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP CMP(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = CMP8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nsbit8(src) & 0xff;
        bugfix(7) vcpuins.result = ((t_nubit8)vcpuins.opr1-(t_nsbit8)vcpuins.opr2)&0xff;
        else vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xff;
        break;
    case 12:
        vcpuins.bit = 16;
        vcpuins.type = CMP16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit8(src) & 0xffff;
        vcpuins.result = ((t_nubit16)vcpuins.opr1-(t_nsbit8)vcpuins.opr2)&0xffff;
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = CMP16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nsbit16(src) & 0xffff;
        bugfix(7) vcpuins.result = ((t_nubit16)vcpuins.opr1-(t_nsbit16)vcpuins.opr2)&0xffff;
        else vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xffff;
        break;
    default:
        CaseError("CMP::len");
        break;
    }
    SetFlags(CMP_FLAG);
#define op cmp
    aexecall
#undef op
    asregall
    acheckall(AFLAGS1)
}
static void PUSH(void *src, t_nubit8 len) {
    t_nubit16 data = d_nubit16(src);
    switch (len) {
    case 16:
        vcpuins.bit = 16;
        vcpu.sp -= 2;
        bugfix(13) vramRealWord(vcpu.ss.selector,vcpu.sp) = data;
        else vramRealWord(vcpu.ss.selector,vcpu.sp) = d_nubit16(src);
        break;
    default:
        CaseError("PUSH::len");
        break;
    }
}
static void POP(void *dest, t_nubit8 len) {
    switch (len) {
    case 16:
        vcpuins.bit = 16;
        d_nubit16(dest) = vramRealWord(vcpu.ss.selector,vcpu.sp);
        vcpu.sp += 2;
        break;
    default:
        CaseError("POP::len");
        break;
    }
}
static ASMCMP INC(void *dest, t_nubit8 len) {
    switch (len) {
    case 8:
        async ub1 = d_nubit8(dest);
#define op inc
        aexec_idc8;
#undef op
        vcpuins.bit = 8;
        vcpuins.type = ADD8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = 0x01;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 16:
        async uw1 = d_nubit16(dest);
#define op inc
        aexec_idc16;
#undef op
        vcpuins.bit = 16;
        vcpuins.type = ADD16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = 0x0001;
        vcpuins.result = (vcpuins.opr1+vcpuins.opr2)&0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("INC::len");
        break;
    }
    SetFlags(INC_FLAG);
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP DEC(void *dest, t_nubit8 len) {
    switch (len) {
    case 8:
        async ub1 = d_nubit8(dest);
#define op dec
        aexec_idc8;
#undef op
        vcpuins.bit = 8;
        vcpuins.type = SUB8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = 0x01;
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.result;
        break;
    case 16:
        async uw1 = d_nubit16(dest);
#define op dec
        aexec_idc16;
#undef op
        vcpuins.bit = 16;
        vcpuins.type = SUB16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = 0x0001;
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.result;
        break;
    default:
        CaseError("DEC::len");
        break;
    }
    SetFlags(DEC_FLAG);
    asregall
    acheckall(AFLAGS1)
}
static void JCC(void *src, t_bool flagj,t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        if (flagj)
            bugfix(5) vcpu.ip += d_nsbit8(src);
        else vcpu.ip += d_nubit8(src);
        break;
    default:
        CaseError("JCC::len");
        break;
    }
}
static ASMCMP TEST(void *dest, void *src, t_nubit8 len) {
    asyncall
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        //vcpuins.type = TEST8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        vcpuins.result = (vcpuins.opr1&vcpuins.opr2)&0xff;
        break;
    case 16:
        vcpuins.bit = 16;
        //vcpuins.type = TEST16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        vcpuins.result = (vcpuins.opr1&vcpuins.opr2)&0xffff;
        break;
    default:
        CaseError("TEST::len");
        break;
    }
    ClrBit(vcpu.flags,VCPU_EFLAGS_OF);
    ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
    ClrBit(vcpu.flags,VCPU_EFLAGS_AF);
    SetFlags(TEST_FLAG);
#define op test
    aexecall
#undef op
    acheckall(AFLAGS1)
}
static void XCHG(void *dest, void *src, t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.opr1 = d_nubit8(dest) & 0xff;
        vcpuins.opr2 = d_nubit8(src) & 0xff;
        d_nubit8(dest) = (t_nubit8)vcpuins.opr2;
        d_nubit8(src) = (t_nubit8)vcpuins.opr1;
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.opr1 = d_nubit16(dest) & 0xffff;
        vcpuins.opr2 = d_nubit16(src) & 0xffff;
        d_nubit16(dest) = (t_nubit16)vcpuins.opr2;
        d_nubit16(src) = (t_nubit16)vcpuins.opr1;
        break;
    default:
        CaseError("XCHG::len");
        break;
    }
}
static void MOV(void *dest, void *src, t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        d_nubit8(dest) = d_nubit8(src);
        break;
    case 16:
        vcpuins.bit = 16;
        d_nubit16(dest) = d_nubit16(src);
        break;
    default:
        CaseError("MOV::len");
        break;
    }
}
static ASMCMP ROL(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount;
    t_bool tempCF;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 8;
        vcpuins.bit = 8;
        while (tempcount) {
            tempCF = GetMSB(d_nubit8(dest), 8);
            d_nubit8(dest) = (d_nubit8(dest)<<1)+(t_nubit8)tempCF;
            tempcount--;
        }
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetLSB(d_nubit8(dest), 8));
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit8(dest), 8)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
#define op rol
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 16;
        vcpuins.bit = 16;
        while (tempcount) {
            tempCF = GetMSB(d_nubit16(dest), 16);
            d_nubit16(dest) = (d_nubit16(dest)<<1)+(t_nubit16)tempCF;
            tempcount--;
        }
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetLSB(d_nubit16(dest), 16));
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit16(dest), 16)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
#define op rol
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("ROL::len");
        break;
    }
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP ROR(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount;
    t_bool tempCF;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 8;
        vcpuins.bit = 8;
        while (tempcount) {
            tempCF = GetLSB(d_nubit8(dest), 8);
            d_nubit8(dest) >>= 1;
            if (tempCF) d_nubit8(dest) |= 0x80;
            tempcount--;
        }
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetMSB(d_nubit8(dest), 8));
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit8(dest), 8)^(!!(d_nubit8(dest)&0x40)));
#define op ror
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 16;
        vcpuins.bit = 16;
        while (tempcount) {
            tempCF = GetLSB(d_nubit16(dest), 16);
            d_nubit16(dest) >>= 1;
            if (tempCF) d_nubit16(dest) |= 0x8000;
            tempcount--;
        }
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetMSB(d_nubit16(dest), 16));
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit16(dest), 16)^(!!(d_nubit16(dest)&0x4000)));
#define op ror
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("ROR::len");
        break;
    }
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP RCL(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount;
    t_bool tempCF;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 9;
        vcpuins.bit = 8;
        while (tempcount) {
            tempCF = GetMSB(d_nubit8(dest), 8);
            d_nubit8(dest) = (d_nubit8(dest)<<1)+GetBit(vcpu.flags, VCPU_EFLAGS_CF);
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,tempCF);
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit8(dest), 8)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
#define op rcl
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 17;
        vcpuins.bit = 16;
        while (tempcount) {
            tempCF = GetMSB(d_nubit16(dest), 16);
            d_nubit16(dest) = (d_nubit16(dest)<<1)+GetBit(vcpu.flags, VCPU_EFLAGS_CF);
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,tempCF);
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit16(dest), 16)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
#define op rcl
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("RCL::len");
        break;
    }
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP RCR(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount;
    t_bool tempCF;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 9;
        vcpuins.bit = 8;
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit8(dest), 8)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
        while (tempcount) {
            tempCF = GetLSB(d_nubit8(dest), 8);
            d_nubit8(dest) >>= 1;
            if (GetBit(vcpu.flags, VCPU_EFLAGS_CF)) d_nubit8(dest) |= 0x80;
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,tempCF);
            tempcount--;
        }
#define op rcr
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        tempcount = (count & 0x1f) % 17;
        vcpuins.bit = 16;
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit16(dest), 16)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
        while (tempcount) {
            tempCF = GetLSB(d_nubit16(dest), 16);
            d_nubit16(dest) >>= 1;
            if (GetBit(vcpu.flags, VCPU_EFLAGS_CF)) d_nubit16(dest) |= 0x8000;
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,tempCF);
            tempcount--;
        }
#define op rcr
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("RCR::len");
        break;
    }
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP SHL(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        vcpuins.bit = 8;
        tempcount = count&0x1f;
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetMSB(d_nubit8(dest), 8));
            d_nubit8(dest) <<= 1;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit8(dest), 8)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nubit8(dest);
                SetFlags(SHL_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nubit8(dest);
                SetFlags(SHL_FLAG);
            }
        }
#define op shl
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        vcpuins.bit = 16;
        tempcount = count&0x1f;
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetMSB(d_nubit16(dest), 16));
            d_nubit16(dest) <<= 1;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit16(dest), 16)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nubit16(dest);
                SetFlags(SHL_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nubit16(dest);
                SetFlags(SHL_FLAG);
            }
        }
#define op shl
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("SHL::len");
        break;
    }
    if (count) _ClrEFLAGS_AF;
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP SHR(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount,tempdest8;
    t_nubit16 tempdest16;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        vcpuins.bit = 8;
        tempcount = count&0x1f;
        tempdest8 = d_nubit8(dest);
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetLSB(d_nubit8(dest), 8));
            d_nubit8(dest) >>= 1;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,!!(tempdest8&0x80));
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nubit8(dest);
                SetFlags(SHR_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nubit8(dest);
                SetFlags(SHR_FLAG);
            }
        }
#define op shr
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        vcpuins.bit = 16;
        tempcount = count&0x1f;
        tempdest16 = d_nubit16(dest);
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetLSB(d_nubit16(dest), 16));
            d_nubit16(dest) >>= 1;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,!!(tempdest16&0x8000));
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nubit16(dest);
                SetFlags(SHR_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nubit16(dest);
                SetFlags(SHR_FLAG);
            }
        }
#define op shr
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("SHR::len");
        break;
    }
    if (count) _ClrEFLAGS_AF;
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP SAL(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        vcpuins.bit = 8;
        tempcount = count&0x1f;
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetMSB(d_nubit8(dest), 8));
            d_nubit8(dest) <<= 1;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit8(dest), 8)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nubit8(dest);
                SetFlags(SAL_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nubit8(dest);
                SetFlags(SAL_FLAG);
            }
        }
#define op sal
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        vcpuins.bit = 16;
        tempcount = count&0x1f;
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetMSB(d_nubit16(dest), 16));
            d_nubit16(dest) <<= 1;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,GetMSB(d_nubit16(dest), 16)^GetBit(vcpu.flags, VCPU_EFLAGS_CF));
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nubit16(dest);
                SetFlags(SAL_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nubit16(dest);
                SetFlags(SAL_FLAG);
            }
        }
#define op sal
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("SAL::len");
        break;
    }
    if (count) _ClrEFLAGS_AF;
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static ASMCMP SAR(void *dest, void *src, t_nubit8 len) {
    t_nubit8 count,tempcount,tempdest8;
    t_nubit16 tempdest16;
    if (src) count = d_nubit8(src);
    else count = 1;
    switch (len) {
    case 8:
        async {
            ub1 = d_nubit8(dest);
            ub2 = count;
        }
        vcpuins.bit = 8;
        tempcount = count&0x1f;
        tempdest8 = d_nubit8(dest);
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetLSB(d_nubit8(dest), 8));
            d_nsbit8(dest) >>= 1;
            //d_nubit8(dest) |= tempdest8&0x80;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,0);
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nsbit8(dest);
                SetFlags(SAR_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nsbit8(dest);
                SetFlags(SAR_FLAG);
            }
        }
#define op sar
        aexec_srd8;
#undef op
        break;
    case 16:
        async {
            uw1 = d_nubit16(dest);
            ub2 = count;
        }
        vcpuins.bit = 16;
        tempcount = count&0x1f;
        tempdest16 = d_nubit16(dest);
        while (tempcount) {
            MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetLSB(d_nubit16(dest), 16));
            d_nsbit16(dest) >>= 1;
            //d_nubit16(dest) |= tempdest16&0x8000;
            tempcount--;
        }
        if (count == 1) MakeBit(vcpu.flags,VCPU_EFLAGS_OF,0);
        bugfix(8) {
            if (count != 0) {
                vcpuins.result = d_nsbit16(dest);
                SetFlags(SAR_FLAG);
            }
        }
        else {
            if (count != 1 && count != 0) {
                vcpuins.result = d_nsbit16(dest);
                SetFlags(SAR_FLAG);
            }
        }
#define op sar
        aexec_srd16;
#undef op
        break;
    default:
        CaseError("SAR::len");
        break;
    }
    if (count) _ClrEFLAGS_AF;
    asregall
    acheckall((count == 1) ? AFLAGS1 : AFLAGS2)
}
static void STRDIR(t_nubit8 len, t_bool flagsi, t_bool flagdi) {
    bugfix(10) {
        /* add parameters flagsi, flagdi */
    } else ;
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        if (GetBit(vcpu.flags, VCPU_EFLAGS_DF)) {
            if (flagdi) vcpu.di--;
            if (flagsi) vcpu.si--;
        } else {
            if (flagdi) vcpu.di++;
            if (flagsi) vcpu.si++;
        }
        break;
    case 16:
        vcpuins.bit = 16;
        if (GetBit(vcpu.flags, VCPU_EFLAGS_DF)) {
            if (flagdi) vcpu.di -= 2;
            if (flagsi) vcpu.si -= 2;
        } else {
            if (flagdi) vcpu.di += 2;
            if (flagsi) vcpu.si += 2;
        }
        break;
    default:
        CaseError("STRDIR::len");
        break;
    }
}
static void MOVS(t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vramRealByte(vcpu.es.selector,vcpu.di) = vramRealByte(vcpuins.roverds->selector,vcpu.si);
        STRDIR(8,1,1);
        break;
    case 16:
        vcpuins.bit = 16;
        vramRealWord(vcpu.es.selector,vcpu.di) = vramRealWord(vcpuins.roverds->selector,vcpu.si);
        STRDIR(16,1,1);
        break;
    default:
        CaseError("MOVS::len");
        break;
    }
}
static void CMPS(t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = CMP8;
        vcpuins.opr1 = vramRealByte(vcpuins.roverds->selector,vcpu.si);
        vcpuins.opr2 = vramRealByte(vcpu.es.selector,vcpu.di);
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xff;
        STRDIR(8,1,1);
        SetFlags(CMP_FLAG);
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = CMP16;
        vcpuins.opr1 = vramRealWord(vcpuins.roverds->selector,vcpu.si);
        vcpuins.opr2 = vramRealWord(vcpu.es.selector,vcpu.di);
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xffff;
        STRDIR(16,1,1);
        SetFlags(CMP_FLAG);
        break;
    default:
        CaseError("CMPS::len");
        break;
    }
}
static void STOS(t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vramRealByte(vcpu.es.selector,vcpu.di) = vcpu.al;
        STRDIR(8,0,1);
        break;
    case 16:
        vcpuins.bit = 16;
        vramRealWord(vcpu.es.selector,vcpu.di) = vcpu.ax;
        STRDIR(16,0,1);
        break;
    default:
        CaseError("STOS::len");
        break;
    }
}
static void LODS(t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpu.al = vramRealByte(vcpuins.roverds->selector,vcpu.si);
        STRDIR(8,1,0);
        break;
    case 16:
        vcpuins.bit = 16;
        vcpu.ax = vramRealWord(vcpuins.roverds->selector,vcpu.si);
        STRDIR(16,1,0);
        break;
    default:
        CaseError("LODS::len");
        break;
    }
}
static void SCAS(t_nubit8 len) {
    switch (len) {
    case 8:
        vcpuins.bit = 8;
        vcpuins.type = CMP8;
        vcpuins.opr1 = vcpu.al;
        vcpuins.opr2 = vramRealByte(vcpu.es.selector,vcpu.di);
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xff;
        STRDIR(8,0,1);
        SetFlags(CMP_FLAG);
        break;
    case 16:
        vcpuins.bit = 16;
        vcpuins.type = CMP16;
        vcpuins.opr1 = vcpu.ax;
        vcpuins.opr2 = vramRealWord(vcpu.es.selector,vcpu.di);
        vcpuins.result = (vcpuins.opr1-vcpuins.opr2)&0xffff;
        STRDIR(16,0,1);
        SetFlags(CMP_FLAG);
        break;
    default:
        CaseError("SCAS::len");
        break;
    }
}
static ASMCMP NOT(void *dest, t_nubit8 len) {
    switch (len) {
    case 8:
        async ub1 = d_nubit8(dest);
        vcpuins.bit = 8;
        d_nubit8(dest) = ~d_nubit8(dest);
#define op not
        aexec_nxx8;
#undef op
        break;
    case 16:
        async uw1 = d_nubit16(dest);
        vcpuins.bit = 16;
        d_nubit16(dest) = ~d_nubit16(dest);
#define op not
        aexec_nxx16;
#undef op
        break;
    default:
        CaseError("NOT::len");
        break;
    }
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP NEG(void *dest, t_nubit8 len) {
    t_nubitcc zero = 0;
    switch (len) {
    case 8:
        async ub1 = d_nubit8(dest);
        vcpuins.bit = 8;
        SUB((void *)&zero,(void *)dest,8);
        d_nubit8(dest) = (t_nubit8)zero;
#define op neg
        aexec_nxx8;
#undef op
        break;
    case 16:
        async uw1 = d_nubit16(dest);
        vcpuins.bit = 16;
        SUB((void *)&zero,(void *)dest,16);
        d_nubit16(dest) = (t_nubit16)zero;
#define op neg
        aexec_nxx16;
#undef op
        break;
    default:
        CaseError("NEG::len");
        break;
    }
    asregall
    acheckall(AFLAGS1)
}
static ASMCMP MUL(void *src, t_nubit8 len) {
    t_nubit32 tempresult;
    switch (len) {
    case 8:
        async ub2 = d_nubit8(src);
        vcpuins.bit = 8;
        vcpu.ax = vcpu.al * d_nubit8(src);
        MakeBit(vcpu.flags,VCPU_EFLAGS_OF,!!vcpu.ah);
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,!!vcpu.ah);
#define op mul
        aexec_mdx8;
#undef op
        break;
    case 16:
        async uw2 = d_nubit16(src);
        vcpuins.bit = 16;
        tempresult = vcpu.ax * d_nubit16(src);
        vcpu.dx = (tempresult>>16)&0xffff;
        vcpu.ax = tempresult&0xffff;
        MakeBit(vcpu.flags,VCPU_EFLAGS_OF,!!vcpu.dx);
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,!!vcpu.dx);
#define op mul
        aexec_mdx16;
#undef op
        break;
    default:
        CaseError("MUL::len");
        break;
    }
    acheck(VCPU_EFLAGS_OF | VCPU_EFLAGS_CF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+MUL");
}
static ASMCMP IMUL(void *src, t_nubit8 len) {
    t_nsbit32 tempresult;
    switch (len) {
    case 8:
        async ub2 = d_nubit8(src);
        vcpuins.bit = 8;
        vcpu.ax = (t_nsbit8)vcpu.al * d_nsbit8(src);
        if (vcpu.ax == vcpu.al) {
            ClrBit(vcpu.flags,VCPU_EFLAGS_OF);
            ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
        } else {
            SetBit(vcpu.flags,VCPU_EFLAGS_OF);
            SetBit(vcpu.flags,VCPU_EFLAGS_CF);
        }
#define op imul
        aexec_mdx8;
#undef op
        acheck(VCPU_EFLAGS_OF | VCPU_EFLAGS_CF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+IMUL");
        break;
    case 16:
        async uw2 = d_nubit16(src);
        vcpuins.bit = 16;
        tempresult = (t_nsbit16)vcpu.ax * d_nsbit16(src);
        vcpu.dx = (t_nubit16)((tempresult>>16)&0xffff);
        vcpu.ax = (t_nubit16)(tempresult&0xffff);
        if (tempresult == (t_nsbit32)vcpu.ax) {
            ClrBit(vcpu.flags,VCPU_EFLAGS_OF);
            ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
        } else {
            SetBit(vcpu.flags,VCPU_EFLAGS_OF);
            SetBit(vcpu.flags,VCPU_EFLAGS_CF);
        }
#define op imul
        aexec_mdx16;
#undef op
        acheck(VCPU_EFLAGS_OF | VCPU_EFLAGS_CF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+IMUL");
        break;
    default:
        CaseError("IMUL::len");
        break;
    }
}
static ASMCMP DIV(void *src, t_nubit8 len) {
    t_nubit16 tempAX = vcpu.ax;
    t_nubit32 tempDXAX = (((t_nubit32)vcpu.dx)<<16)+vcpu.ax;
    switch (len) {
    case 8:
        async ub2 = d_nubit8(src);
        vcpuins.bit = 8;
        if (d_nubit8(src) == 0) {
            INT(0x00);
        } else {
            vcpu.al = (t_nubit8)(tempAX / d_nubit8(src));
            vcpu.ah = (t_nubit8)(tempAX % d_nubit8(src));
        }
#define op div
        aexec_mdx8;
#undef op
        acheck(VCPU_EFLAGS_DF | VCPU_EFLAGS_TF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+DIV");
        break;
    case 16:
        async uw2 = d_nubit16(src);
        vcpuins.bit = 16;
        if (d_nubit16(src) == 0) {
            INT(0x00);
        } else {
            vcpu.ax = (t_nubit16)(tempDXAX / d_nubit16(src));
            vcpu.dx = (t_nubit16)(tempDXAX % d_nubit16(src));
        }
#define op div
        aexec_mdx16;
#undef op
        acheck(VCPU_EFLAGS_DF | VCPU_EFLAGS_TF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+DIV");
        break;
    default:
        CaseError("DIV::len");
        break;
    }
}
static void IDIV(void *src, t_nubit8 len) {
    t_nsbit16 tempAX = vcpu.ax;
    t_nsbit32 tempDXAX = (((t_nubit32)vcpu.dx)<<16)+vcpu.ax;
    switch (len) {
    case 8:
        async ub2 = d_nubit8(src);
        vcpuins.bit = 8;
        if (d_nubit8(src) == 0) {
            INT(0x00);
        } else {
            vcpu.al = (t_nubit8)(tempAX / d_nsbit8(src));
            vcpu.ah = (t_nubit8)(tempAX % d_nsbit8(src));
        }
#define op idiv
        aexec_mdx8;
#undef op
        acheck(VCPU_EFLAGS_DF | VCPU_EFLAGS_TF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+IDIV");
        break;
    case 16:
        async uw2 = d_nubit16(src);
        vcpuins.bit = 16;
        if (d_nubit16(src) == 0) {
            INT(0x00);
        } else {
            vcpu.ax = (t_nubit16)(tempDXAX / d_nsbit16(src));
            vcpu.dx = (t_nubit16)(tempDXAX % d_nsbit16(src));
        }
#define op idiv
        aexec_mdx16;
#undef op
        acheck(VCPU_EFLAGS_DF | VCPU_EFLAGS_TF) vapiPrintIns(vcpu.cs.selector,vcpu.ip,"+IDIV");
        break;
    default:
        CaseError("IDIV::len");
        break;
    }
}
static void INT(t_nubit8 intid) {
    PUSH((void *)&vcpu.flags,16);
    ClrBit(vcpu.flags, (VCPU_EFLAGS_IF | VCPU_EFLAGS_TF));
    PUSH((void *)&vcpu.cs.selector,16);
    PUSH((void *)&vcpu.ip,16);
    vcpu.ip = vramRealWord(0x0000,intid*4+0);
    vcpu.cs.selector = vramRealWord(0x0000,intid*4+2);
}

CHECKED UndefinedOpcode() {
    vapiPrint("The NXVM CPU has encountered an illegal instruction.\n");
    vapiPrint("CS:%04X IP:%04X OP:%02X %02X %02X %02X\n",
              vcpu.cs.selector, vcpu.ip, vramRealByte(vcpu.cs.selector,vcpu.ip+0),
              vramRealByte(vcpu.cs.selector,vcpu.ip+1), vramRealByte(vcpu.cs.selector,vcpu.ip+2),
              vramRealByte(vcpu.cs.selector,vcpu.ip+3), vramRealByte(vcpu.cs.selector,vcpu.ip+4));
    vapiCallBackMachineStop();
}
CHECKED ADD_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    ADD((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED ADD_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    ADD((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED ADD_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    ADD((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED ADD_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    ADD((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED ADD_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    ADD((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED ADD_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    ADD((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED PUSH_ES() {
    vcpu.ip++;
    PUSH((void *)&vcpu.es.selector,16);
}
CHECKED POP_ES() {
    vcpu.ip++;
    POP((void *)&vcpu.es.selector,16);
}
CHECKED OR_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    OR((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED OR_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    OR((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED OR_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    OR((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED OR_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    OR((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED OR_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    OR((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED OR_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    OR((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED PUSH_CS() {
    vcpu.ip++;
    PUSH((void *)&vcpu.cs.selector,16);
}
CHECKED POP_CS() {
    vcpu.ip++;
    POP((void *)&vcpu.cs.selector,16);
}
DIFF INS_0F() {
    UndefinedOpcode();
}
CHECKED ADC_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    ADC((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED ADC_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    ADC((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED ADC_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    ADC((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED ADC_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    ADC((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED ADC_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    ADC((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED ADC_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    ADC((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED PUSH_SS() {
    vcpu.ip++;
    PUSH((void *)&vcpu.ss.selector,16);
}
CHECKED POP_SS() {
    vcpu.ip++;
    POP((void *)&vcpu.ss.selector,16);
}
CHECKED SBB_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    SBB((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED SBB_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    SBB((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED SBB_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    SBB((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED SBB_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    SBB((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED SBB_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    SBB((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED SBB_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    SBB((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED PUSH_DS() {
    vcpu.ip++;
    PUSH((void *)&vcpu.ds.selector,16);
}
CHECKED POP_DS() {
    vcpu.ip++;
    POP((void *)&vcpu.ds.selector,16);
}
CHECKED AND_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    AND((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED AND_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    AND((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED AND_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    AND((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED AND_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    AND((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED AND_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    AND((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED AND_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    AND((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED ES() {
    vcpu.ip++;
    vcpuins.roverds = &vcpu.es;
    vcpuins.roverss = &vcpu.es;
}
ASMCMP DAA() {
    t_nubit8 oldAL = vcpu.al;
    t_nubit8 newAL = vcpu.al + 0x06;
    vcpu.ip++;
    async;
    if (((vcpu.al & 0x0f) > 0x09) || GetBit(vcpu.flags, VCPU_EFLAGS_AF)) {
        vcpu.al = newAL;
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetBit(vcpu.flags, VCPU_EFLAGS_CF) || ((newAL < oldAL) || (newAL < 0x06)));
        bugfix(19) SetBit(vcpu.flags, VCPU_EFLAGS_AF);
        else ;
    } else ClrBit(vcpu.flags, VCPU_EFLAGS_AF);
    if (((vcpu.al & 0xf0) > 0x90) || GetBit(vcpu.flags, VCPU_EFLAGS_CF)) {
        vcpu.al += 0x60;
        SetBit(vcpu.flags,VCPU_EFLAGS_CF);
    } else ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
    bugfix(18) {
        vcpuins.bit = 8;
        vcpuins.result = (t_nubitcc)vcpu.al;
        SetFlags(DAA_FLAG);
    }
    else ;
#define op daa
    aexec_cax8;
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"DAA");
}
CHECKED SUB_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    SUB((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED SUB_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    SUB((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED SUB_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    SUB((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED SUB_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    SUB((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED SUB_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    SUB((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED SUB_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    SUB((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED CS() {
    vcpu.ip++;
    vcpuins.roverds = &vcpu.cs;
    vcpuins.roverss = &vcpu.cs;
}
ASMCMP DAS() {
    t_nubit8 oldAL = vcpu.al;
    vcpu.ip++;
    async;
    if (((vcpu.al & 0x0f) > 0x09) || GetBit(vcpu.flags, VCPU_EFLAGS_AF)) {
        vcpu.al -= 0x06;
        MakeBit(vcpu.flags,VCPU_EFLAGS_CF,GetBit(vcpu.flags, VCPU_EFLAGS_CF) || (oldAL < 0x06));
        SetBit(vcpu.flags,VCPU_EFLAGS_AF);
    } else ClrBit(vcpu.flags,VCPU_EFLAGS_AF);
    if ((vcpu.al > 0x9f) || GetBit(vcpu.flags, VCPU_EFLAGS_CF)) {
        vcpu.al -= 0x60;
        SetBit(vcpu.flags,VCPU_EFLAGS_CF);
    } else ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
    bugfix(18) {
        vcpuins.bit = 8;
        vcpuins.result = (t_nubitcc)vcpu.al;
        SetFlags(DAS_FLAG);
    }
    else ;
#define op das
    aexec_cax8;
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"DAS");
}
CHECKED XOR_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    XOR((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
CHECKED XOR_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    XOR((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
CHECKED XOR_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    XOR((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
CHECKED XOR_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    XOR((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
CHECKED XOR_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    XOR((void *)&vcpu.al,(void *)rimm,8);
}
CHECKED XOR_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    XOR((void *)&vcpu.ax,(void *)rimm,16);
}
CHECKED SS() {
    vcpu.ip++;
    vcpuins.roverds = &vcpu.ss;
    vcpuins.roverss = &vcpu.ss;
}
ASMCMP AAA() {
    vcpu.ip++;
    async;
    if (((vcpu.al&0x0f) > 0x09) || GetBit(vcpu.flags, VCPU_EFLAGS_AF)) {
        vcpu.al += 0x06;
        vcpu.ah += 0x01;
        SetBit(vcpu.flags,VCPU_EFLAGS_AF);
        SetBit(vcpu.flags,VCPU_EFLAGS_CF);
    } else {
        ClrBit(vcpu.flags,VCPU_EFLAGS_AF);
        ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
    }
    vcpu.al &= 0x0f;
#define op aaa
    aexec_cax16;
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"AAA");
}
WRAPPER CMP_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    CMP((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
WRAPPER CMP_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    CMP((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
WRAPPER CMP_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    CMP((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
WRAPPER CMP_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    CMP((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
WRAPPER CMP_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    CMP((void *)&vcpu.al,(void *)rimm,8);
}
WRAPPER CMP_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    CMP((void *)&vcpu.ax,(void *)rimm,16);
}
DIFF DS() {
    vcpu.ip++;
    vcpuins.roverds = &vcpu.ds;
    vcpuins.roverss = &vcpu.ds;
}
ASMCMP AAS() {
    vcpu.ip++;
    async;
    if (((vcpu.al&0x0f) > 0x09) || GetBit(vcpu.flags, VCPU_EFLAGS_AF)) {
        vcpu.al -= 0x06;
        vcpu.ah += 0x01;
        SetBit(vcpu.flags,VCPU_EFLAGS_AF);
        SetBit(vcpu.flags,VCPU_EFLAGS_CF);
    } else {
        ClrBit(vcpu.flags,VCPU_EFLAGS_CF);
        ClrBit(vcpu.flags,VCPU_EFLAGS_AF);
    }
    vcpu.al &= 0x0f;
#define op aas
    aexec_cax16;
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"AAS");
}
WRAPPER INC_AX() {
    vcpu.ip++;
    INC((void *)&vcpu.ax,16);
}
WRAPPER INC_CX() {
    vcpu.ip++;
    INC((void *)&vcpu.cx,16);
}
WRAPPER INC_DX() {
    vcpu.ip++;
    INC((void *)&vcpu.dx,16);
}
WRAPPER INC_BX() {
    vcpu.ip++;
    INC((void *)&vcpu.bx,16);
}
WRAPPER INC_SP() {
    vcpu.ip++;
    INC((void *)&vcpu.sp,16);
}
WRAPPER INC_BP() {
    vcpu.ip++;
    INC((void *)&vcpu.bp,16);
}
WRAPPER INC_SI() {
    vcpu.ip++;
    INC((void *)&vcpu.si,16);
}
WRAPPER INC_DI() {
    vcpu.ip++;
    INC((void *)&vcpu.di,16);
}
WRAPPER DEC_AX() {
    vcpu.ip++;
    DEC((void *)&vcpu.ax,16);
}
WRAPPER DEC_CX() {
    vcpu.ip++;
    DEC((void *)&vcpu.cx,16);
}
WRAPPER DEC_DX() {
    vcpu.ip++;
    DEC((void *)&vcpu.dx,16);
}
WRAPPER DEC_BX() {
    vcpu.ip++;
    DEC((void *)&vcpu.bx,16);
}
WRAPPER DEC_SP() {
    vcpu.ip++;
    DEC((void *)&vcpu.sp,16);
}
WRAPPER DEC_BP() {
    vcpu.ip++;
    DEC((void *)&vcpu.bp,16);
}
WRAPPER DEC_SI() {
    vcpu.ip++;
    DEC((void *)&vcpu.si,16);
}
WRAPPER DEC_DI() {
    vcpu.ip++;
    DEC((void *)&vcpu.di,16);
}
CHECKED PUSH_AX() {
    vcpu.ip++;
    PUSH((void *)&vcpu.ax,16);
}
CHECKED PUSH_CX() {
    vcpu.ip++;
    PUSH((void *)&vcpu.cx,16);
}
CHECKED PUSH_DX() {
    vcpu.ip++;
    PUSH((void *)&vcpu.dx,16);
}
CHECKED PUSH_BX() {
    vcpu.ip++;
    PUSH((void *)&vcpu.bx,16);
}
CHECKED PUSH_SP() {
    vcpu.ip++;
    PUSH((void *)&vcpu.sp,16);
}
CHECKED PUSH_BP() {
    vcpu.ip++;
    PUSH((void *)&vcpu.bp,16);
}
CHECKED PUSH_SI() {
    vcpu.ip++;
    PUSH((void *)&vcpu.si,16);
}
CHECKED PUSH_DI() {
    vcpu.ip++;
    PUSH((void *)&vcpu.di,16);
}
CHECKED POP_AX() {
    vcpu.ip++;
    POP((void *)&vcpu.ax,16);
}
CHECKED POP_CX() {
    vcpu.ip++;
    POP((void *)&vcpu.cx,16);
}
CHECKED POP_DX() {
    vcpu.ip++;
    POP((void *)&vcpu.dx,16);
}
CHECKED POP_BX() {
    vcpu.ip++;
    POP((void *)&vcpu.bx,16);
}
CHECKED POP_SP() {
    vcpu.ip++;
    POP((void *)&vcpu.sp,16);
}
CHECKED POP_BP() {
    vcpu.ip++;
    POP((void *)&vcpu.bp,16);
}
CHECKED POP_SI() {
    vcpu.ip++;
    POP((void *)&vcpu.si,16);
}
CHECKED POP_DI() {
    vcpu.ip++;
    POP((void *)&vcpu.di,16);
}
CHECKED JO() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, GetBit(vcpu.flags, VCPU_EFLAGS_OF), 8);
}
CHECKED JNO() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, !GetBit(vcpu.flags, VCPU_EFLAGS_OF), 8);
}
CHECKED JC() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, GetBit(vcpu.flags, VCPU_EFLAGS_CF), 8);
}
CHECKED JNC() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, !GetBit(vcpu.flags, VCPU_EFLAGS_CF), 8);
}
CHECKED JZ() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, GetBit(vcpu.flags, VCPU_EFLAGS_ZF), 8);
}
CHECKED JNZ() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, !GetBit(vcpu.flags, VCPU_EFLAGS_ZF), 8);
}
CHECKED JBE() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, (GetBit(vcpu.flags, VCPU_EFLAGS_CF) ||
                       GetBit(vcpu.flags, VCPU_EFLAGS_ZF)), 8);
}
CHECKED JA() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, (!GetBit(vcpu.flags, VCPU_EFLAGS_CF) &&
                       !GetBit(vcpu.flags, VCPU_EFLAGS_ZF)), 8);
}
CHECKED JS() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, GetBit(vcpu.flags, VCPU_EFLAGS_SF), 8);
}
CHECKED JNS() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, !GetBit(vcpu.flags, VCPU_EFLAGS_SF), 8);
}
CHECKED JP() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, GetBit(vcpu.flags, VCPU_EFLAGS_PF), 8);
}
CHECKED JNP() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, !GetBit(vcpu.flags, VCPU_EFLAGS_PF), 8);
}
CHECKED JL() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, (GetBit(vcpu.flags, VCPU_EFLAGS_SF) !=
                       GetBit(vcpu.flags, VCPU_EFLAGS_OF)), 8);
}
CHECKED JNL() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, (GetBit(vcpu.flags, VCPU_EFLAGS_SF) ==
                       GetBit(vcpu.flags, VCPU_EFLAGS_OF)), 8);
}
CHECKED JLE() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, (GetBit(vcpu.flags, VCPU_EFLAGS_ZF) ||
                       (GetBit(vcpu.flags, VCPU_EFLAGS_SF) !=
                        GetBit(vcpu.flags, VCPU_EFLAGS_OF))), 8);
}
CHECKED JG() {
    vcpu.ip++;
    GetImm(8);
    JCC((void *)rimm, (!GetBit(vcpu.flags, VCPU_EFLAGS_ZF) &&
                       (GetBit(vcpu.flags, VCPU_EFLAGS_SF) ==
                        GetBit(vcpu.flags, VCPU_EFLAGS_OF))), 8);
}
void INS_80() {
    vcpu.ip++;
    GetModRegRM(0,8);
    GetImm(8);
    switch (vcpuins.rr) {
    case 0:
        ADD((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 1:
        OR ((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 2:
        ADC((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 3:
        SBB((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 4:
        AND((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 5:
        SUB((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 6:
        XOR((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 7:
        CMP((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    default:
        CaseError("INS_80::vcpuins.rr");
        break;
    }
}
void INS_81() {
    vcpu.ip++;
    GetModRegRM(0,16);
    GetImm(16);
    switch (vcpuins.rr) {
    case 0:
        ADD((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 1:
        OR ((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 2:
        ADC((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 3:
        SBB((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 4:
        AND((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 5:
        SUB((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 6:
        XOR((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 7:
        CMP((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    default:
        CaseError("INS_81::vcpuins.rr");
        break;
    }
}
void INS_82() {
    INS_80();
}
void INS_83() {
    vcpu.ip++;
    GetModRegRM(0,16);
    GetImm(8);
    switch (vcpuins.rr) {
    case 0:
        ADD((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 1:
        OR ((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 2:
        ADC((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 3:
        SBB((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 4:
        AND((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 5:
        SUB((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 6:
        XOR((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    case 7:
        CMP((void *)vcpuins.rrm,(void *)rimm,12);
        break;
    default:
        CaseError("INS_83::vcpuins.rr");
        break;
    }
}
WRAPPER TEST_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    TEST((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
WRAPPER TEST_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    TEST((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
WRAPPER XCHG_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    XCHG((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
WRAPPER XCHG_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    XCHG((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
WRAPPER MOV_RM8_R8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    MOV((void *)vcpuins.rrm,(void *)vcpuins.rr,8);
}
WRAPPER MOV_RM16_R16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    MOV((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
WRAPPER MOV_R8_RM8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    MOV((void *)vcpuins.rr,(void *)vcpuins.rrm,8);
}
WRAPPER MOV_R16_RM16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    MOV((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
WRAPPER MOV_RM16_SEG() {
    vcpu.ip++;
    GetModRegRM(4,16);
    MOV((void *)vcpuins.rrm,(void *)vcpuins.rr,16);
}
void LEA_R16_M16() {
    vcpu.ip++;
    GetModRegRMEA();
    d_nubit16(vcpuins.rr) = vcpuins.rrm & 0xffff;
}
WRAPPER MOV_SEG_RM16() {
    vcpu.ip++;
    GetModRegRM(4,16);
    MOV((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
}
void POP_RM16() {
    vcpu.ip++;
    GetModRegRM(0,16);
    switch (vcpuins.rr) {
    case 0:
        bugfix(14) POP((void *)vcpuins.rrm,16);
        else POP((void *)vcpuins.rrm,16);
        break;
    default:
        CaseError("POP_RM16::vcpuins.rr");
        break;
    }
}
CHECKED NOP() {
    vcpu.ip++;
}
WRAPPER XCHG_CX_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.cx,(void *)&vcpu.ax,16);
}
WRAPPER XCHG_DX_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.dx,(void *)&vcpu.ax,16);
}
WRAPPER XCHG_BX_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.bx,(void *)&vcpu.ax,16);
}
WRAPPER XCHG_SP_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.sp,(void *)&vcpu.ax,16);
}
WRAPPER XCHG_BP_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.bp,(void *)&vcpu.ax,16);
}
WRAPPER XCHG_SI_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.si,(void *)&vcpu.ax,16);
}
WRAPPER XCHG_DI_AX() {
    vcpu.ip++;
    XCHG((void *)&vcpu.di,(void *)&vcpu.ax,16);
}
ASMCMP CBW() {
    vcpu.ip++;
    async;
    vcpu.ax = (t_nsbit8)vcpu.al;
#define op cbw
    aexec_cax16;
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"CBW");
}
ASMCMP CWD() {
    vcpu.ip++;
    async;
    if (vcpu.ax & 0x8000) vcpu.dx = 0xffff;
    else vcpu.dx = 0x0000;
#define op cwd
    aexec_cax16;
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"CWD");
}
void CALL_PTR16_16() {
    t_nubit16 newcs,newip;
    async;
    vcpu.ip++;
    GetImm(16);
    newip = d_nubit16(rimm);
    GetImm(16);
    newcs = d_nubit16(rimm);
    aipcheck;
    PUSH((void *)&vcpu.cs.selector,16);
    PUSH((void *)&vcpu.ip,16);
    vcpu.ip = newip;
    vcpu.cs.selector = newcs;
}
NOTIMP WAIT() {
    vcpu.ip++;
    /* not implemented */
}
CHECKED PUSHF() {
    vcpu.ip++;
    PUSH((void *)&vcpu.flags,16);
}
CHECKED POPF() {
    vcpu.ip++;
    POP((void *)&vcpu.flags,16);
}
void SAHF() {
    vcpu.ip++;
    d_nubit8(&vcpu.flags) = vcpu.ah;
}
void LAHF() {
    vcpu.ip++;
    vcpu.ah = d_nubit8(&vcpu.flags);
}
WRAPPER MOV_AL_M8() {
    vcpu.ip++;
    GetMem();
    MOV((void *)&vcpu.al,(void *)vcpuins.rrm,8);
}
WRAPPER MOV_AX_M16() {
    vcpu.ip++;
    GetMem();
    MOV((void *)&vcpu.ax,(void *)vcpuins.rrm,16);
}
WRAPPER MOV_M8_AL() {
    vcpu.ip++;
    GetMem();
    MOV((void *)vcpuins.rrm,(void *)&vcpu.al,8);
}
WRAPPER MOV_M16_AX() {
    vcpu.ip++;
    GetMem();
    MOV((void *)vcpuins.rrm,(void *)&vcpu.ax,16);
}
void MOVSB() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) MOVS(8);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            MOVS(8);
            vcpu.cx--;
        }
    }
}
void MOVSW() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) MOVS(16);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            MOVS(16);
            vcpu.cx--;
        }
    }
}
void CMPSB() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) CMPS(8);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            CMPS(8);
            vcpu.cx--;
            if ((vcpuins.prefix_rep == PREFIX_REP_REPZ && !GetBit(vcpu.flags, VCPU_EFLAGS_ZF)) || (vcpuins.prefix_rep == PREFIX_REP_REPZNZ && GetBit(vcpu.flags, VCPU_EFLAGS_ZF))) break;
        }
    }
}
void CMPSW() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) CMPS(16);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            CMPS(16);
            vcpu.cx--;
            if ((vcpuins.prefix_rep == PREFIX_REP_REPZ && !GetBit(vcpu.flags, VCPU_EFLAGS_ZF)) || (vcpuins.prefix_rep == PREFIX_REP_REPZNZ && GetBit(vcpu.flags, VCPU_EFLAGS_ZF))) break;
        }
    }
}
WRAPPER TEST_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    TEST((void *)&vcpu.al,(void *)rimm,8);
}
WRAPPER TEST_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    TEST((void *)&vcpu.ax,(void *)rimm,16);
}
void STOSB() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) STOS(8);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            STOS(8);
            vcpu.cx--;
        }
    }
}
void STOSW() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) STOS(16);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            STOS(16);
            vcpu.cx--;
        }
    }
}
void LODSB() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) LODS(8);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            LODS(8);
            vcpu.cx--;
        }
    }
}
void LODSW() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) LODS(16);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            LODS(16);
            vcpu.cx--;
        }
    }
}
void SCASB() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) SCAS(8);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            SCAS(8);
            vcpu.cx--;
            if ((vcpuins.prefix_rep == PREFIX_REP_REPZ && !GetBit(vcpu.flags, VCPU_EFLAGS_ZF)) || (vcpuins.prefix_rep == PREFIX_REP_REPZNZ && GetBit(vcpu.flags, VCPU_EFLAGS_ZF))) break;
        }
    }
}
void SCASW() {
    vcpu.ip++;
    if (vcpuins.prefix_rep == PREFIX_REP_NONE) SCAS(16);
    else {
        while (vcpu.cx) {
            //vcpuinsExecInt();
            SCAS(16);
            vcpu.cx--;
            if ((vcpuins.prefix_rep == PREFIX_REP_REPZ && !GetBit(vcpu.flags, VCPU_EFLAGS_ZF)) || (vcpuins.prefix_rep == PREFIX_REP_REPZNZ && GetBit(vcpu.flags, VCPU_EFLAGS_ZF))) break;
        }
    }
}
WRAPPER MOV_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.al,(void *)rimm,8);
}
WRAPPER MOV_CL_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.cl,(void *)rimm,8);
}
WRAPPER MOV_DL_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.dl,(void *)rimm,8);
}
WRAPPER MOV_BL_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.bl,(void *)rimm,8);
}
WRAPPER MOV_AH_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.ah,(void *)rimm,8);
}
WRAPPER MOV_CH_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.ch,(void *)rimm,8);
}
WRAPPER MOV_DH_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.dh,(void *)rimm,8);
}
WRAPPER MOV_BH_I8() {
    vcpu.ip++;
    GetImm(8);
    MOV((void *)&vcpu.bh,(void *)rimm,8);
}
WRAPPER MOV_AX_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.ax,(void *)rimm,16);
}
WRAPPER MOV_CX_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.cx,(void *)rimm,16);
}
WRAPPER MOV_DX_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.dx,(void *)rimm,16);
}
WRAPPER MOV_BX_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.bx,(void *)rimm,16);
}
WRAPPER MOV_SP_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.sp,(void *)rimm,16);
}
WRAPPER MOV_BP_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.bp,(void *)rimm,16);
}
WRAPPER MOV_SI_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.si,(void *)rimm,16);
}
WRAPPER MOV_DI_I16() {
    vcpu.ip++;
    GetImm(16);
    MOV((void *)&vcpu.di,(void *)rimm,16);
}
void INS_C0() {
    vcpu.ip++;
    GetModRegRM(0,8);
    GetImm(8);
    switch (vcpuins.rr) {
    case 0:
        ROL((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 1:
        ROR((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 2:
        RCL((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 3:
        RCR((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 4:
        SHL((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 5:
        SHR((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 6:
        SAL((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 7:
        SAR((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    default:
        CaseError("INS_C0::vcpuins.rr");
        break;
    }
}
void INS_C1() {
    vcpu.ip++;
    GetModRegRM(0,16);
    GetImm(8);
    switch (vcpuins.rr) {
    case 0:
        ROL((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 1:
        ROR((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 2:
        RCL((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 3:
        RCR((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 4:
        SHL((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 5:
        SHR((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 6:
        SAL((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 7:
        SAR((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    default:
        CaseError("INS_C1::vcpuins.rr");
        break;
    }
}
void RET_I16() {
    t_nubit16 addsp;
    vcpu.ip++;
    bugfix(15) {
        GetImm(16);
        addsp = d_nubit16(rimm);
    }
    else {
        GetImm(8);
        addsp = d_nubit8(rimm);
    }
    POP((void *)&vcpu.ip,16);
    vcpu.sp += addsp;
}
void RET() {
    vcpu.ip++;
    POP((void *)&vcpu.ip,16);
}
void LES_R16_M16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    MOV((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
    MOV((void *)&vcpu.es.selector,(void *)(vcpuins.rrm+2),16);
}
void LDS_R16_M16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    MOV((void *)vcpuins.rr,(void *)vcpuins.rrm,16);
    MOV((void *)&vcpu.ds.selector,(void *)(vcpuins.rrm+2),16);
}
WRAPPER MOV_M8_I8() {
    vcpu.ip++;
    GetModRegRM(8,8);
    GetImm(8);
    MOV((void *)vcpuins.rrm,(void *)rimm,8);
}
WRAPPER MOV_M16_I16() {
    vcpu.ip++;
    GetModRegRM(16,16);
    GetImm(16);
    MOV((void *)vcpuins.rrm,(void *)rimm,16);
}
void RETF_I16() {
    t_nubit16 addsp;
    vcpu.ip++;
    GetImm(16);
    addsp = d_nubit16(rimm);
    POP((void *)&vcpu.ip,16);
    POP((void *)&vcpu.cs.selector,16);
    vcpu.sp += addsp;
}
void RETF() {
    POP((void *)&vcpu.ip,16);
    POP((void *)&vcpu.cs.selector,16);
}
void INT3() {
    async;
    vcpu.ip++;
    aipcheck;
    INT(0x03);
}
void INT_I8() {
    async;
    vcpu.ip++;
    GetImm(8);
    aipcheck;
    INT(d_nubit8(rimm));
}
void INTO() {
    async;
    vcpu.ip++;
    aipcheck;
    if (GetBit(vcpu.flags, VCPU_EFLAGS_OF)) INT(0x04);
}
void IRET() {
    vcpu.ip++;
    POP((void *)&vcpu.ip,16);
    POP((void *)&vcpu.cs.selector,16);
    POP((void *)&vcpu.flags,16);
}
void INS_D0() {
    vcpu.ip++;
    GetModRegRM(0,8);
    switch (vcpuins.rr) {
    case 0:
        ROL((void *)vcpuins.rrm,NULL,8);
        break;
    case 1:
        ROR((void *)vcpuins.rrm,NULL,8);
        break;
    case 2:
        RCL((void *)vcpuins.rrm,NULL,8);
        break;
    case 3:
        RCR((void *)vcpuins.rrm,NULL,8);
        break;
    case 4:
        SHL((void *)vcpuins.rrm,NULL,8);
        break;
    case 5:
        SHR((void *)vcpuins.rrm,NULL,8);
        break;
    case 6:
        SAL((void *)vcpuins.rrm,NULL,8);
        break;
    case 7:
        SAR((void *)vcpuins.rrm,NULL,8);
        break;
    default:
        CaseError("INS_D0::vcpuins.rr");
        break;
    }
}
void INS_D1() {
    vcpu.ip++;
    GetModRegRM(0,16);
    switch (vcpuins.rr) {
    case 0:
        ROL((void *)vcpuins.rrm,NULL,16);
        break;
    case 1:
        ROR((void *)vcpuins.rrm,NULL,16);
        break;
    case 2:
        RCL((void *)vcpuins.rrm,NULL,16);
        break;
    case 3:
        RCR((void *)vcpuins.rrm,NULL,16);
        break;
    case 4:
        SHL((void *)vcpuins.rrm,NULL,16);
        break;
    case 5:
        SHR((void *)vcpuins.rrm,NULL,16);
        break;
    case 6:
        SAL((void *)vcpuins.rrm,NULL,16);
        break;
    case 7:
        SAR((void *)vcpuins.rrm,NULL,16);
        break;
    default:
        CaseError("INS_D1::vcpuins.rr");
        break;
    }
}
void INS_D2() {
    vcpu.ip++;
    GetModRegRM(0,8);
    switch (vcpuins.rr) {
    case 0:
        ROL((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 1:
        ROR((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 2:
        RCL((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 3:
        RCR((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 4:
        SHL((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 5:
        SHR((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 6:
        SAL((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    case 7:
        SAR((void *)vcpuins.rrm,(void *)&vcpu.cl,8);
        break;
    default:
        CaseError("INS_D2::vcpuins.rr");
        break;
    }
}
void INS_D3() {
    vcpu.ip++;
    GetModRegRM(0,16);
    switch (vcpuins.rr) {
    case 0:
        ROL((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 1:
        ROR((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 2:
        RCL((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 3:
        RCR((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 4:
        SHL((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 5:
        SHR((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 6:
        SAL((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    case 7:
        SAR((void *)vcpuins.rrm,(void *)&vcpu.cl,16);
        break;
    default:
        CaseError("INS_D3::vcpuins.rr");
        break;
    }
}
ASMCMP AAM() {
    t_nubit8 base,tempAL;
    vcpu.ip++;
    GetImm(8);
    async;
    base = d_nubit8(rimm);
    if (base == 0) INT(0x00);
    else {
        tempAL = vcpu.al;
        vcpu.ah = tempAL / base;
        vcpu.al = tempAL % base;
        vcpuins.bit = 0x08;
        vcpuins.result = vcpu.al & 0xff;
        SetFlags(AAM_FLAG);
    }
#define op aam
    aexec_cax16
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-2,"AAM");
}
ASMCMP AAD() {
    t_nubit8 base,tempAL,tempAH;
    vcpu.ip++;
    GetImm(8);
    async;
    base = d_nubit8(rimm);
    if (base == 0) INT(0x00);
    else {
        tempAL = vcpu.al;
        tempAH = vcpu.ah;
        vcpu.al = (tempAL+(tempAH*base)) & 0xff;
        vcpu.ah = 0x00;
        vcpuins.bit = 0x08;
        vcpuins.result = vcpu.al & 0xff;
        SetFlags(AAD_FLAG);
    }
#define op aad
    aexec_cax16
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-2,"AAD");
}
void XLAT() {
    vcpu.ip++;
    vcpu.al = vramRealByte(vcpuins.roverds->selector,vcpu.bx+vcpu.al);
}
/*
void INS_D9();
void INS_DB();
*/
void LOOPNZ() {
    t_nsbit8 rel8;
    vcpu.ip++;
    GetImm(8);
    bugfix(12) rel8 = d_nsbit8(rimm);
    else rel8 = d_nubit8(rimm);
    vcpu.cx--;
    if (vcpu.cx && !GetBit(vcpu.flags, VCPU_EFLAGS_ZF)) vcpu.ip += rel8;
}
void LOOPZ() {
    t_nsbit8 rel8;
    vcpu.ip++;
    GetImm(8);
    bugfix(12) rel8 = d_nsbit8(rimm);
    else rel8 = d_nubit8(rimm);
    vcpu.cx--;
    if (vcpu.cx && GetBit(vcpu.flags, VCPU_EFLAGS_ZF)) vcpu.ip += rel8;
}
void LOOP() {
    t_nsbit8 rel8;
    vcpu.ip++;
    GetImm(8);
    bugfix(12) rel8 = d_nsbit8(rimm);
    else rel8 = d_nubit8(rimm);
    vcpu.cx--;
    if (vcpu.cx) vcpu.ip += rel8;
}
void JCXZ_REL8() {
    vcpu.ip++;
    GetImm(8);
    JCC((void*)rimm,!vcpu.cx,8);
}
void IN_AL_I8() {
    vcpu.ip++;
    GetImm(8);
    ExecFun(vport.in[d_nubit8(rimm)]);
    vcpu.al = vport.iobyte;
}
void IN_AX_I8() {
    vcpu.ip++;
    GetImm(8);
    ExecFun(vport.in[d_nubit8(rimm)]);
    vcpu.ax = vport.ioword;
}
void OUT_I8_AL() {
    vcpu.ip++;
    GetImm(8);
    vport.iobyte = vcpu.al;
    ExecFun(vport.out[d_nubit8(rimm)]);
}
void OUT_I8_AX() {
    vcpu.ip++;
    GetImm(8);
    vport.ioword = vcpu.ax;
    ExecFun(vport.out[d_nubit8(rimm)]);
}
void CALL_REL16() {
    t_nsbit16 rel16;
    async;
    vcpu.ip++;
    GetImm(16);
    rel16 = d_nsbit16(rimm);
    aipcheck;
    PUSH((void *)&vcpu.ip,16);
    bugfix(12) vcpu.ip += rel16;
    else vcpu.ip += d_nubit16(rimm);
}
void JMP_REL16() {
    t_nsbit16 rel16;
    vcpu.ip++;
    GetImm(16);
    rel16 = d_nsbit16(rimm);
    bugfix(2) vcpu.ip += rel16;
    else vcpu.ip += d_nubit16(rimm);
}
void JMP_PTR16_16() {
    t_nubit16 newip,newcs;
    vcpu.ip++;
    GetImm(16);
    newip = d_nubit16(rimm);
    GetImm(16);
    newcs = d_nubit16(rimm);
    vcpu.ip = newip;
    vcpu.cs.selector = newcs;
}
void JMP_REL8() {
    t_nsbit8 rel8;
    vcpu.ip++;
    GetImm(8);
    rel8 = d_nsbit8(rimm);
    bugfix(9) vcpu.ip += rel8;
    else vcpu.ip += d_nubit8(rimm);
}
void IN_AL_DX() {
    vcpu.ip++;
    ExecFun(vport.in[vcpu.dx]);
    vcpu.al = vport.iobyte;
}
void IN_AX_DX() {
    vcpu.ip++;
    ExecFun(vport.in[vcpu.dx]);
    vcpu.ax = vport.ioword;
}
void OUT_DX_AL() {
    vcpu.ip++;
    vport.iobyte = vcpu.al;
    ExecFun(vport.out[vcpu.dx]);
}
void OUT_DX_AX() {
    vcpu.ip++;
    vport.ioword = vcpu.ax;
    ExecFun(vport.out[vcpu.dx]);
}
NOTIMP LOCK() {
    vcpu.ip++;
    /* Not Implemented */
}
void REPNZ() {
    // CMPS,SCAS
    vcpu.ip++;
    vcpuins.prefix_rep = PREFIX_REP_REPZNZ;
}
void REP() {
    // MOVS,LODS,STOS,CMPS,SCAS
    vcpu.ip++;
    vcpuins.prefix_rep = PREFIX_REP_REPZ;
}
NOTIMP HLT() {
    vcpu.ip++;
    /* Not Implemented */
}
ASMCMP CMC() {
    vcpu.ip++;
    async;
    vcpu.flags ^= VCPU_EFLAGS_CF;
#define op cmc
    aexec_csf
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"CMC");
}
void INS_F6() {
    vcpu.ip++;
    GetModRegRM(0,8);
    switch (vcpuins.rr) {
    case 0:
        GetImm(8);
        TEST((void *)vcpuins.rrm,(void *)rimm,8);
        break;
    case 2:
        NOT ((void *)vcpuins.rrm,8);
        break;
    case 3:
        NEG ((void *)vcpuins.rrm,8);
        break;
    case 4:
        MUL ((void *)vcpuins.rrm,8);
        break;
    case 5:
        IMUL((void *)vcpuins.rrm,8);
        break;
    case 6:
        DIV ((void *)vcpuins.rrm,8);
        break;
    case 7:
        IDIV((void *)vcpuins.rrm,8);
        break;
    default:
        CaseError("INS_F6::vcpuins.rr");
        break;
    }
}
void INS_F7() {
    vcpu.ip++;
    GetModRegRM(0,16);
    switch (vcpuins.rr) {
    case 0:
        GetImm(16);
        TEST((void *)vcpuins.rrm,(void *)rimm,16);
        break;
    case 2:
        NOT ((void *)vcpuins.rrm,16);
        break;
    case 3:
        NEG ((void *)vcpuins.rrm,16);
        break;
    case 4:
        MUL ((void *)vcpuins.rrm,16);
        break;
    case 5:
        IMUL((void *)vcpuins.rrm,16);
        break;
    case 6:
        DIV ((void *)vcpuins.rrm,16);
        break;
    case 7:
        IDIV((void *)vcpuins.rrm,16);
        break;
    default:
        CaseError("INS_F7::vcpuins.rr");
        break;
    }
}
ASMCMP CLC() {
    vcpu.ip++;
    async;
    ClrBit(vcpu.flags, VCPU_EFLAGS_CF);
#define op clc
    aexec_csf
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"CLC");
}
ASMCMP STC() {
    vcpu.ip++;
    async;
    SetBit(vcpu.flags, VCPU_EFLAGS_CF);
#define op stc
    aexec_csf
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"STC");
}
void CLI() {
    vcpu.ip++;
    ClrBit(vcpu.flags, VCPU_EFLAGS_IF);
}
void STI() {
    vcpu.ip++;
    SetBit(vcpu.flags, VCPU_EFLAGS_IF);
}
ASMCMP CLD() {
    vcpu.ip++;
    async;
    ClrBit(vcpu.flags, VCPU_EFLAGS_DF);
#define op cld
    aexec_csf
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"CLD");
}
ASMCMP STD() {
    vcpu.ip++;
    async;
    SetBit(vcpu.flags,VCPU_EFLAGS_DF);
#define op STD
    aexec_csf
#undef op
    acheck(AFLAGS1) vapiPrintIns(vcpu.cs.selector,vcpu.ip-1,"STD");
}
void INS_FE() {
    vcpu.ip++;
    GetModRegRM(0,8);
    switch (vcpuins.rr) {
    case 0:
        INC((void *)vcpuins.rrm,8);
        break;
    case 1:
        DEC((void *)vcpuins.rrm,8);
        break;
    default:
        CaseError("INS_FE::vcpuins.rr");
        break;
    }
}
void INS_FF() {
    async;
    vcpu.ip++;
    GetModRegRM(0,16);
    aipcheck;
    switch (vcpuins.rr) {
    case 0:
        INC((void *)vcpuins.rrm,16);
        break;
    case 1:
        DEC((void *)vcpuins.rrm,16);
        break;
    case 2:	/* CALL_RM16 */
        PUSH((void *)&vcpu.ip,16);
        vcpu.ip = d_nubit16(vcpuins.rrm);
        break;
    case 3:	/* CALL_M16_16 */
        PUSH((void *)&vcpu.cs.selector,16);
        PUSH((void *)&vcpu.ip,16);
        vcpu.ip = d_nubit16(vcpuins.rrm);
        bugfix(11) vcpu.cs.selector = d_nubit16(vcpuins.rrm+2);
        else vcpu.cs.selector = d_nubit16(vcpuins.rrm+1);
        break;
    case 4:	/* JMP_RM16 */
        vcpu.ip = d_nubit16(vcpuins.rrm);
        break;
    case 5:	/* JMP_M16_16 */
        vcpu.ip = d_nubit16(vcpuins.rrm);
        bugfix(11) vcpu.cs.selector = d_nubit16(vcpuins.rrm+2);
        else vcpu.cs.selector = d_nubit16(vcpuins.rrm+1);
        break;
    case 6:	/* PUSH_RM16 */
        PUSH((void *)vcpuins.rrm,16);
        break;
    default:
        CaseError("INS_FF::vcpuins.rr");
        break;
    }
}

static t_bool IsPrefix(t_nubit8 opcode) {
    switch (opcode) {
    case 0xf0:
    case 0xf2:
    case 0xf3:
    case 0x2e:
    case 0x36:
    case 0x3e:
    case 0x26:
        //case 0x64: case 0x65: case 0x66: case 0x67:
        return 0x01;
        break;
    default:
        return 0x00;
        break;
    }
}
static void ClrPrefix() {
    vcpuins.roverds = &vcpu.ds;
    vcpuins.roverss = &vcpu.ss;
    vcpuins.prefix_rep = PREFIX_REP_NONE;
}
static void ExecIns() {
    t_nubit8 opcode;
    vcpurec.linear = (vcpu.cs.selector << 4) + vcpu.ip;
    vcpurec.msize = 0;
    vcpurec.opcode = vramRealQWord(vcpu.cs.selector, vcpu.ip);
    vcpurec.rcpu = vcpu;
    vcpurec.stack = vramRealQWord(vcpu.ss.selector, vcpu.sp);
    vcpurec.svcextl = 0;
    bugfix(17) {
        /* Note: in this case, if we use two prefixes, the second prefix
           will be discarded incorrectly */
        ClrPrefix();
        do {
            opcode = vramRealByte(vcpu.cs.selector, vcpu.ip);
            ExecFun(vcpuins.table[opcode]);
        } while (IsPrefix(opcode));
    }
    else {
        bugfix(16) {
            /* Note: in this bug, if an interrupt generated between
               prefix and operation, the prefix may not be deployed */
            if (IsPrefix(opcode)) ExecFun(vcpuins.table[opcode]);
            opcode = vramRealByte(vcpu.cs.selector, vcpu.ip);
            ExecFun(vcpuins.table[opcode]);
            ClrPrefix();
        }
        else {
            ExecFun(vcpuins.table[opcode]);
            if (!IsPrefix(opcode)) ClrPrefix();
        }
    }
    vcpurec.a1 = vcpuins.opr1;
    vcpurec.a2 = vcpuins.opr2;
    vcpurec.a3 = vcpuins.result;
    vcpurec.abit = vcpuins.bit;
}
static void ExecInt() {
    /* hardware interrupt handeler */
    if (vcpu.flagnmi) INT(0x02);
    vcpu.flagnmi = 0x00;

    if (GetBit(vcpu.flags, VCPU_EFLAGS_IF) && vpicScanINTR())
        INT(vpicGetINTR());

    if (GetBit(vcpu.flags, VCPU_EFLAGS_TF)) INT(0x01);
}

void QDX() {
    vcpu.ip++;
    GetImm(8);
    switch (d_nubit8(rimm)) {
    case 0x00:
    case 0xff:
        vapiPrint("\nNXVM STOP at CS:%04X IP:%04X INS:QDX IMM:%02X\n",
                  vcpu.cs.selector,vcpu.ip,d_nubit8(rimm));
        vapiPrint("This happens because of the special instruction.\n");
        vapiCallBackMachineStop();
        break;
    case 0x01:
    case 0xfe:
        vapiPrint("\nNXVM RESET at CS:%04X IP:%04X INS:QDX IMM:%02X\n",
                  vcpu.cs.selector,vcpu.ip,d_nubit8(rimm));
        vapiPrint("This happens because of the special instruction.\n");
        vapiCallBackMachineReset();
        break;
    default:
        qdbiosExecInt(d_nubit8(rimm));
        MakeBit(vramRealWord(_ss,_sp + 4), VCPU_EFLAGS_ZF, GetBit(_flags, VCPU_EFLAGS_ZF));
        MakeBit(vramRealWord(_ss,_sp + 4), VCPU_EFLAGS_CF, GetBit(_flags, VCPU_EFLAGS_CF));
        break;
    }
}

void vcpuinsInit() {
    memset(&vcpuins, 0x00, sizeof(t_cpuins));
    vcpuins.table[0x00] = (t_faddrcc)ADD_RM8_R8;
    vcpuins.table[0x01] = (t_faddrcc)ADD_RM16_R16;
    vcpuins.table[0x02] = (t_faddrcc)ADD_R8_RM8;
    vcpuins.table[0x03] = (t_faddrcc)ADD_R16_RM16;
    vcpuins.table[0x04] = (t_faddrcc)ADD_AL_I8;
    vcpuins.table[0x05] = (t_faddrcc)ADD_AX_I16;
    vcpuins.table[0x06] = (t_faddrcc)PUSH_ES;
    vcpuins.table[0x07] = (t_faddrcc)POP_ES;
    vcpuins.table[0x08] = (t_faddrcc)OR_RM8_R8;
    vcpuins.table[0x09] = (t_faddrcc)OR_RM16_R16;
    vcpuins.table[0x0a] = (t_faddrcc)OR_R8_RM8;
    vcpuins.table[0x0b] = (t_faddrcc)OR_R16_RM16;
    vcpuins.table[0x0c] = (t_faddrcc)OR_AL_I8;
    vcpuins.table[0x0d] = (t_faddrcc)OR_AX_I16;
    vcpuins.table[0x0e] = (t_faddrcc)PUSH_CS;
    vcpuins.table[0x0f] = (t_faddrcc)POP_CS;
    //vcpuins.table[0x0f] = (t_faddrcc)INS_0F;
    vcpuins.table[0x10] = (t_faddrcc)ADC_RM8_R8;
    vcpuins.table[0x11] = (t_faddrcc)ADC_RM16_R16;
    vcpuins.table[0x12] = (t_faddrcc)ADC_R8_RM8;
    vcpuins.table[0x13] = (t_faddrcc)ADC_R16_RM16;
    vcpuins.table[0x14] = (t_faddrcc)ADC_AL_I8;
    vcpuins.table[0x15] = (t_faddrcc)ADC_AX_I16;
    vcpuins.table[0x16] = (t_faddrcc)PUSH_SS;
    vcpuins.table[0x17] = (t_faddrcc)POP_SS;
    vcpuins.table[0x18] = (t_faddrcc)SBB_RM8_R8;
    vcpuins.table[0x19] = (t_faddrcc)SBB_RM16_R16;
    vcpuins.table[0x1a] = (t_faddrcc)SBB_R8_RM8;
    vcpuins.table[0x1b] = (t_faddrcc)SBB_R16_RM16;
    vcpuins.table[0x1c] = (t_faddrcc)SBB_AL_I8;
    vcpuins.table[0x1d] = (t_faddrcc)SBB_AX_I16;
    vcpuins.table[0x1e] = (t_faddrcc)PUSH_DS;
    vcpuins.table[0x1f] = (t_faddrcc)POP_DS;
    vcpuins.table[0x20] = (t_faddrcc)AND_RM8_R8;
    vcpuins.table[0x21] = (t_faddrcc)AND_RM16_R16;
    vcpuins.table[0x22] = (t_faddrcc)AND_R8_RM8;
    vcpuins.table[0x23] = (t_faddrcc)AND_R16_RM16;
    vcpuins.table[0x24] = (t_faddrcc)AND_AL_I8;
    vcpuins.table[0x25] = (t_faddrcc)AND_AX_I16;
    vcpuins.table[0x26] = (t_faddrcc)ES;
    vcpuins.table[0x27] = (t_faddrcc)DAA;
    vcpuins.table[0x28] = (t_faddrcc)SUB_RM8_R8;
    vcpuins.table[0x29] = (t_faddrcc)SUB_RM16_R16;
    vcpuins.table[0x2a] = (t_faddrcc)SUB_R8_RM8;
    vcpuins.table[0x2b] = (t_faddrcc)SUB_R16_RM16;
    vcpuins.table[0x2c] = (t_faddrcc)SUB_AL_I8;
    vcpuins.table[0x2d] = (t_faddrcc)SUB_AX_I16;
    vcpuins.table[0x2e] = (t_faddrcc)CS;
    vcpuins.table[0x2f] = (t_faddrcc)DAS;
    vcpuins.table[0x30] = (t_faddrcc)XOR_RM8_R8;
    vcpuins.table[0x31] = (t_faddrcc)XOR_RM16_R16;
    vcpuins.table[0x32] = (t_faddrcc)XOR_R8_RM8;
    vcpuins.table[0x33] = (t_faddrcc)XOR_R16_RM16;
    vcpuins.table[0x34] = (t_faddrcc)XOR_AL_I8;
    vcpuins.table[0x35] = (t_faddrcc)XOR_AX_I16;
    vcpuins.table[0x36] = (t_faddrcc)SS;
    vcpuins.table[0x37] = (t_faddrcc)AAA;
    vcpuins.table[0x38] = (t_faddrcc)CMP_RM8_R8;
    vcpuins.table[0x39] = (t_faddrcc)CMP_RM16_R16;
    vcpuins.table[0x3a] = (t_faddrcc)CMP_R8_RM8;
    vcpuins.table[0x3b] = (t_faddrcc)CMP_R16_RM16;
    vcpuins.table[0x3c] = (t_faddrcc)CMP_AL_I8;
    vcpuins.table[0x3d] = (t_faddrcc)CMP_AX_I16;
    vcpuins.table[0x3e] = (t_faddrcc)DS;
    vcpuins.table[0x3f] = (t_faddrcc)AAS;
    vcpuins.table[0x40] = (t_faddrcc)INC_AX;
    vcpuins.table[0x41] = (t_faddrcc)INC_CX;
    vcpuins.table[0x42] = (t_faddrcc)INC_DX;
    vcpuins.table[0x43] = (t_faddrcc)INC_BX;
    vcpuins.table[0x44] = (t_faddrcc)INC_SP;
    vcpuins.table[0x45] = (t_faddrcc)INC_BP;
    vcpuins.table[0x46] = (t_faddrcc)INC_SI;
    vcpuins.table[0x47] = (t_faddrcc)INC_DI;
    vcpuins.table[0x48] = (t_faddrcc)DEC_AX;
    vcpuins.table[0x49] = (t_faddrcc)DEC_CX;
    vcpuins.table[0x4a] = (t_faddrcc)DEC_DX;
    vcpuins.table[0x4b] = (t_faddrcc)DEC_BX;
    vcpuins.table[0x4c] = (t_faddrcc)DEC_SP;
    vcpuins.table[0x4d] = (t_faddrcc)DEC_BP;
    vcpuins.table[0x4e] = (t_faddrcc)DEC_SI;
    vcpuins.table[0x4f] = (t_faddrcc)DEC_DI;
    vcpuins.table[0x50] = (t_faddrcc)PUSH_AX;
    vcpuins.table[0x51] = (t_faddrcc)PUSH_CX;
    vcpuins.table[0x52] = (t_faddrcc)PUSH_DX;
    vcpuins.table[0x53] = (t_faddrcc)PUSH_BX;
    vcpuins.table[0x54] = (t_faddrcc)PUSH_SP;
    vcpuins.table[0x55] = (t_faddrcc)PUSH_BP;
    vcpuins.table[0x56] = (t_faddrcc)PUSH_SI;
    vcpuins.table[0x57] = (t_faddrcc)PUSH_DI;
    vcpuins.table[0x58] = (t_faddrcc)POP_AX;
    vcpuins.table[0x59] = (t_faddrcc)POP_CX;
    vcpuins.table[0x5a] = (t_faddrcc)POP_DX;
    vcpuins.table[0x5b] = (t_faddrcc)POP_BX;
    vcpuins.table[0x5c] = (t_faddrcc)POP_SP;
    vcpuins.table[0x5d] = (t_faddrcc)POP_BP;
    vcpuins.table[0x5e] = (t_faddrcc)POP_SI;
    vcpuins.table[0x5f] = (t_faddrcc)POP_DI;
    vcpuins.table[0x60] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x61] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x62] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x63] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x64] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x65] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x66] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x67] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x68] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x69] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x6a] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x6b] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x6c] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x6d] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x6e] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x6f] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0x70] = (t_faddrcc)JO;
    vcpuins.table[0x71] = (t_faddrcc)JNO;
    vcpuins.table[0x72] = (t_faddrcc)JC;
    vcpuins.table[0x73] = (t_faddrcc)JNC;
    vcpuins.table[0x74] = (t_faddrcc)JZ;
    vcpuins.table[0x75] = (t_faddrcc)JNZ;
    vcpuins.table[0x76] = (t_faddrcc)JBE;
    vcpuins.table[0x77] = (t_faddrcc)JA;
    vcpuins.table[0x78] = (t_faddrcc)JS;
    vcpuins.table[0x79] = (t_faddrcc)JNS;
    vcpuins.table[0x7a] = (t_faddrcc)JP;
    vcpuins.table[0x7b] = (t_faddrcc)JNP;
    vcpuins.table[0x7c] = (t_faddrcc)JL;
    vcpuins.table[0x7d] = (t_faddrcc)JNL;
    vcpuins.table[0x7e] = (t_faddrcc)JLE;
    vcpuins.table[0x7f] = (t_faddrcc)JG;
    vcpuins.table[0x80] = (t_faddrcc)INS_80;
    vcpuins.table[0x81] = (t_faddrcc)INS_81;
    vcpuins.table[0x82] = (t_faddrcc)INS_82;
    vcpuins.table[0x83] = (t_faddrcc)INS_83;
    vcpuins.table[0x84] = (t_faddrcc)TEST_RM8_R8;
    vcpuins.table[0x85] = (t_faddrcc)TEST_RM16_R16;
    vcpuins.table[0x86] = (t_faddrcc)XCHG_R8_RM8;
    vcpuins.table[0x87] = (t_faddrcc)XCHG_R16_RM16;
    vcpuins.table[0x88] = (t_faddrcc)MOV_RM8_R8;
    vcpuins.table[0x89] = (t_faddrcc)MOV_RM16_R16;
    vcpuins.table[0x8a] = (t_faddrcc)MOV_R8_RM8;
    vcpuins.table[0x8b] = (t_faddrcc)MOV_R16_RM16;
    vcpuins.table[0x8c] = (t_faddrcc)MOV_RM16_SEG;
    vcpuins.table[0x8d] = (t_faddrcc)LEA_R16_M16;
    vcpuins.table[0x8e] = (t_faddrcc)MOV_SEG_RM16;
    vcpuins.table[0x8f] = (t_faddrcc)POP_RM16;
    vcpuins.table[0x90] = (t_faddrcc)NOP;
    vcpuins.table[0x91] = (t_faddrcc)XCHG_CX_AX;
    vcpuins.table[0x92] = (t_faddrcc)XCHG_DX_AX;
    vcpuins.table[0x93] = (t_faddrcc)XCHG_BX_AX;
    vcpuins.table[0x94] = (t_faddrcc)XCHG_SP_AX;
    vcpuins.table[0x95] = (t_faddrcc)XCHG_BP_AX;
    vcpuins.table[0x96] = (t_faddrcc)XCHG_SI_AX;
    vcpuins.table[0x97] = (t_faddrcc)XCHG_DI_AX;
    vcpuins.table[0x98] = (t_faddrcc)CBW;
    vcpuins.table[0x99] = (t_faddrcc)CWD;
    vcpuins.table[0x9a] = (t_faddrcc)CALL_PTR16_16;
    vcpuins.table[0x9b] = (t_faddrcc)WAIT;
    vcpuins.table[0x9c] = (t_faddrcc)PUSHF;
    vcpuins.table[0x9d] = (t_faddrcc)POPF;
    vcpuins.table[0x9e] = (t_faddrcc)SAHF;
    vcpuins.table[0x9f] = (t_faddrcc)LAHF;
    vcpuins.table[0xa0] = (t_faddrcc)MOV_AL_M8;
    vcpuins.table[0xa1] = (t_faddrcc)MOV_AX_M16;
    vcpuins.table[0xa2] = (t_faddrcc)MOV_M8_AL;
    vcpuins.table[0xa3] = (t_faddrcc)MOV_M16_AX;
    vcpuins.table[0xa4] = (t_faddrcc)MOVSB;
    vcpuins.table[0xa5] = (t_faddrcc)MOVSW;
    vcpuins.table[0xa6] = (t_faddrcc)CMPSB;
    vcpuins.table[0xa7] = (t_faddrcc)CMPSW;
    vcpuins.table[0xa8] = (t_faddrcc)TEST_AL_I8;
    vcpuins.table[0xa9] = (t_faddrcc)TEST_AX_I16;
    vcpuins.table[0xaa] = (t_faddrcc)STOSB;
    vcpuins.table[0xab] = (t_faddrcc)STOSW;
    vcpuins.table[0xac] = (t_faddrcc)LODSB;
    vcpuins.table[0xad] = (t_faddrcc)LODSW;
    vcpuins.table[0xae] = (t_faddrcc)SCASB;
    vcpuins.table[0xaf] = (t_faddrcc)SCASW;
    vcpuins.table[0xb0] = (t_faddrcc)MOV_AL_I8;
    vcpuins.table[0xb1] = (t_faddrcc)MOV_CL_I8;
    vcpuins.table[0xb2] = (t_faddrcc)MOV_DL_I8;
    vcpuins.table[0xb3] = (t_faddrcc)MOV_BL_I8;
    vcpuins.table[0xb4] = (t_faddrcc)MOV_AH_I8;
    vcpuins.table[0xb5] = (t_faddrcc)MOV_CH_I8;
    vcpuins.table[0xb6] = (t_faddrcc)MOV_DH_I8;
    vcpuins.table[0xb7] = (t_faddrcc)MOV_BH_I8;
    vcpuins.table[0xb8] = (t_faddrcc)MOV_AX_I16;
    vcpuins.table[0xb9] = (t_faddrcc)MOV_CX_I16;
    vcpuins.table[0xba] = (t_faddrcc)MOV_DX_I16;
    vcpuins.table[0xbb] = (t_faddrcc)MOV_BX_I16;
    vcpuins.table[0xbc] = (t_faddrcc)MOV_SP_I16;
    vcpuins.table[0xbd] = (t_faddrcc)MOV_BP_I16;
    vcpuins.table[0xbe] = (t_faddrcc)MOV_SI_I16;
    vcpuins.table[0xbf] = (t_faddrcc)MOV_DI_I16;
    vcpuins.table[0xc0] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xc1] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xc2] = (t_faddrcc)RET_I16;
    vcpuins.table[0xc3] = (t_faddrcc)RET;
    vcpuins.table[0xc4] = (t_faddrcc)LES_R16_M16;
    vcpuins.table[0xc5] = (t_faddrcc)LDS_R16_M16;
    vcpuins.table[0xc6] = (t_faddrcc)MOV_M8_I8;
    vcpuins.table[0xc7] = (t_faddrcc)MOV_M16_I16;
    vcpuins.table[0xc8] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xc9] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xca] = (t_faddrcc)RETF_I16;
    vcpuins.table[0xcb] = (t_faddrcc)RETF;
    vcpuins.table[0xcc] = (t_faddrcc)INT3;
    vcpuins.table[0xcd] = (t_faddrcc)INT_I8;
    vcpuins.table[0xce] = (t_faddrcc)INTO;
    vcpuins.table[0xcf] = (t_faddrcc)IRET;
    vcpuins.table[0xd0] = (t_faddrcc)INS_D0;
    vcpuins.table[0xd1] = (t_faddrcc)INS_D1;
    vcpuins.table[0xd2] = (t_faddrcc)INS_D2;
    vcpuins.table[0xd3] = (t_faddrcc)INS_D3;
    vcpuins.table[0xd4] = (t_faddrcc)AAM;
    vcpuins.table[0xd5] = (t_faddrcc)AAD;
    vcpuins.table[0xd6] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xd7] = (t_faddrcc)XLAT;
    vcpuins.table[0xd8] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xd9] = (t_faddrcc)UndefinedOpcode;
    //vcpuins.table[0xd9] = (t_faddrcc)INS_D9;
    vcpuins.table[0xda] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xdb] = (t_faddrcc)UndefinedOpcode;
    //vcpuins.table[0xdb] = (t_faddrcc)INS_DB;
    vcpuins.table[0xdc] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xdd] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xde] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xdf] = (t_faddrcc)UndefinedOpcode;
    vcpuins.table[0xe0] = (t_faddrcc)LOOPNZ;
    vcpuins.table[0xe1] = (t_faddrcc)LOOPZ;
    vcpuins.table[0xe2] = (t_faddrcc)LOOP;
    vcpuins.table[0xe3] = (t_faddrcc)JCXZ_REL8;
    vcpuins.table[0xe4] = (t_faddrcc)IN_AL_I8;
    vcpuins.table[0xe5] = (t_faddrcc)IN_AX_I8;
    vcpuins.table[0xe6] = (t_faddrcc)OUT_I8_AL;
    vcpuins.table[0xe7] = (t_faddrcc)OUT_I8_AX;
    vcpuins.table[0xe8] = (t_faddrcc)CALL_REL16;
    vcpuins.table[0xe9] = (t_faddrcc)JMP_REL16;
    vcpuins.table[0xea] = (t_faddrcc)JMP_PTR16_16;
    vcpuins.table[0xeb] = (t_faddrcc)JMP_REL8;
    vcpuins.table[0xec] = (t_faddrcc)IN_AL_DX;
    vcpuins.table[0xed] = (t_faddrcc)IN_AX_DX;
    vcpuins.table[0xee] = (t_faddrcc)OUT_DX_AL;
    vcpuins.table[0xef] = (t_faddrcc)OUT_DX_AX;
    vcpuins.table[0xf0] = (t_faddrcc)LOCK;
    vcpuins.table[0xf1] = (t_faddrcc)QDX;
    vcpuins.table[0xf2] = (t_faddrcc)REPNZ;
    vcpuins.table[0xf3] = (t_faddrcc)REP;
    vcpuins.table[0xf4] = (t_faddrcc)HLT;
    vcpuins.table[0xf5] = (t_faddrcc)CMC;
    vcpuins.table[0xf6] = (t_faddrcc)INS_F6;
    vcpuins.table[0xf7] = (t_faddrcc)INS_F7;
    vcpuins.table[0xf8] = (t_faddrcc)CLC;
    vcpuins.table[0xf9] = (t_faddrcc)STC;
    vcpuins.table[0xfa] = (t_faddrcc)CLI;
    vcpuins.table[0xfb] = (t_faddrcc)STI;
    vcpuins.table[0xfc] = (t_faddrcc)CLD;
    vcpuins.table[0xfd] = (t_faddrcc)STD;
    vcpuins.table[0xfe] = (t_faddrcc)INS_FE;
    vcpuins.table[0xff] = (t_faddrcc)INS_FF;
}
void vcpuinsReset() {
    vcpuins.rrm = vcpuins.rr = rimm = (t_vaddrcc)NULL;
    vcpuins.opr1 = vcpuins.opr2 = vcpuins.result = vcpuins.bit = 0;
    ClrPrefix();
}
void vcpuinsRefresh() {
    ExecIns();
    ExecInt();
}
void vcpuinsFinal() {}
