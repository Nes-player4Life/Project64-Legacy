/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include <windows.h>
#include "main.h"
#include "cpu.h"
#include "plugin.h"
#include "debugger.h"

void __cdecl AiCheckInterrupts ( void ) {	
	CPU_Action.CheckInterrupts = TRUE;
	CPU_Action.DoSomething = TRUE;
}

void __cdecl CheckInterrupts ( void ) {	

	MI_INTR_REG &= ~MI_INTR_AI;
	if (CPU_Type != CPU_SyncCores) {
		MI_INTR_REG |= (AudioIntrReg & MI_INTR_AI);
	}
	if ((MI_INTR_MASK_REG & MI_INTR_REG) != 0) {
		FAKE_CAUSE_REGISTER |= CAUSE_IP2;
	} else  {
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP2;
	}

	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }

	if (( STATUS_REGISTER & FAKE_CAUSE_REGISTER & 0xFF00) != 0) {
		if (!CPU_Action.DoInterrupt) {
			CPU_Action.DoSomething = TRUE;
			CPU_Action.DoInterrupt = TRUE;
		}
	}
}

void DoIntegerOverflow(BOOL DelaySlot) {
	if (ShowDebugMessages) {
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in AddressError Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in AddressError Exception");
		}
	}
	CAUSE_REGISTER = EXC_OV;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void DoAddressError ( BOOL DelaySlot, QWORD BadVaddr, BOOL FromRead) {
	if (ShowDebugMessages) {
		DisplayError("AddressError while accessing %016llX (%s). PC: %08X",
			BadVaddr,
			FromRead ? "read" : "write",
			DelaySlot ? PROGRAM_COUNTER -4 : PROGRAM_COUNTER);
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in AddressError Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in AddressError Exception");
		}
	}
	if (FromRead) {
		CAUSE_REGISTER = EXC_RADE;
	} else {
		CAUSE_REGISTER = EXC_WADE;
	}
	BAD_VADDR_REGISTER = BadVaddr;
	CONTEXT_REGISTER &= 0xFF80000F;
	CONTEXT_REGISTER |= (BadVaddr >> 9) & 0x007FFFF0;
	XCONTEXT_REGISTER &= 0xFFFFFFFE00000000LL;
	XCONTEXT_REGISTER |= ((QWORD)BadVaddr >> 9) & 0x7FFFFFF0LL;
	XCONTEXT_REGISTER |= ((QWORD)BadVaddr >> 31) & 0x180000000LL;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void _fastcall DoBreakException ( BOOL DelaySlot) {
	if (ShowDebugMessages) {
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in Break Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in Break Exception");
		}
	}

	CAUSE_REGISTER = EXC_BREAK;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void _fastcall DoCopUnusableException ( BOOL DelaySlot, int Coprocessor ) {
	if (ShowDebugMessages) {
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in Break Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in Break Exception");
		}
	}

	CAUSE_REGISTER = EXC_CPU;
	if (Coprocessor == 1) { CAUSE_REGISTER |= 0x10000000; }
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void DoIntrException ( BOOL DelaySlot ) {
	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }
	CAUSE_REGISTER = FAKE_CAUSE_REGISTER;
	CAUSE_REGISTER |= EXC_INT;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void _fastcall DoTLBMiss ( BOOL DelaySlot, DWORD BadVaddr, BOOL FromRead ) {
	CAUSE_REGISTER = FromRead ? EXC_RMISS : EXC_WMISS;
	BAD_VADDR_REGISTER = BadVaddr;
	CONTEXT_REGISTER &= 0xFF80000F;
	CONTEXT_REGISTER |= (BadVaddr >> 9) & 0x007FFFF0;
	ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
	if ((STATUS_REGISTER & STATUS_EXL) == 0) {
		if (DelaySlot) {
			CAUSE_REGISTER |= CAUSE_BD;
			EPC_REGISTER = PROGRAM_COUNTER - 4;
		} else {
			EPC_REGISTER = PROGRAM_COUNTER;
		}
		if (AddressDefined(BadVaddr)) {
			PROGRAM_COUNTER = 0x80000180;
		} else {
			PROGRAM_COUNTER = 0x80000000;
		}
		STATUS_REGISTER |= STATUS_EXL;
	} else {
		if (ShowDebugMessages)
			DisplayError("EXL Set\nAddress Defined: %s",AddressDefined(BadVaddr)?"TRUE":"FALSE");
		PROGRAM_COUNTER = 0x80000180;
	}
}

void _fastcall DoSysCallException(BOOL DelaySlot) {
	if (ShowDebugMessages) {
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in SysCall Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in SysCall Exception");
		}
	}

	CAUSE_REGISTER = EXC_SYSCALL;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	}
	else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void _fastcall DoIllegalInstructionException(BOOL DelaySlot) {
	if (ShowDebugMessages) {
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in SysCall Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in SysCall Exception");
		}
	}

	CAUSE_REGISTER = EXC_II;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	}
	else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void _fastcall DoTrapException(BOOL DelaySlot) {
	if (ShowDebugMessages) {
		if ((STATUS_REGISTER & STATUS_EXL) != 0) {
			DisplayError("EXL set in Break Exception");
		}
		if ((STATUS_REGISTER & STATUS_ERL) != 0) {
			DisplayError("ERL set in Break Exception");
		}
	}

	CAUSE_REGISTER = EXC_TRAP;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	}
	else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}
