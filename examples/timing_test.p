.origin 0
.entrypoint START

#include "prucode.hp"

START:
// Preamble to set up OCP and shared RAM
	LBCO	r0, CONST_PRUCFG, 4, 4		// Enable OCP master port
	CLR 	r0, r0, 4					// Clear SYSCFG[STANDBY_INIT] to enable OCP master port
	SBCO	r0, CONST_PRUCFG, 4, 4
	MOV		r0, 0x00000120				// Configure the programmable pointer register for PRU0 by setting c28_pointer[15:0]
	MOV		r1, CTPPR_0					// field to 0x0120.  This will make C28 point to 0x00012000 (PRU shared RAM).
	ST32	r0, r1

// Useful things start here...
	// Load in the number of loops to perform from shared ram
	// This should be set by the calling code before executing this PRU code
	LBCO	r0, CONST_PRUSHAREDRAM, 0, 4

// while (r0 != 0) {
LOOP1:
	SUB		r0, r0, 1						// r0--
	SBCO	r0, CONST_PRUSHAREDRAM,0,4		// Update the value in shared RAM
	QBNE	LOOP1, r0, 0					// Loop while r0!=0
// }

	MOV		r31.b0, PRU0_ARM_INTERRUPT+16	// Fire interrupt on completion
	HALT									// Halt the processor

