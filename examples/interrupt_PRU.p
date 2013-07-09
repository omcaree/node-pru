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
	MOV		r0, 0							// Initialise counter to 0
LOOP1:
	QBBS	INTERRUPTED, r31, 30			// If we're interrupted, execute handler code
	ADD		r0, r0, 1						// Otherwise, increment counter
	QBA		LOOP1							// Keep going...
	
INTERRUPTED:
	//Clear system event in SECR2
	MOV32	r4, 0x00000001
	MOV32	r3, SECR2_OFFSET
	SBCO	r4,	CONST_PRUSSINTC, r3, 4 

	//Clear system event enable in ECR2
	MOV32	r4, 0x00000001
	MOV32	r3, ECR2_OFFSET
	SBCO	r4, CONST_PRUSSINTC, r3, 4 

	SBCO	r0, CONST_PRUSHAREDRAM,0,4		// If we're interrupted, output counter value to shared memory

	MOV		r31.b0, PRU0_ARM_INTERRUPT+16	// Fire interrupt on completion
	HALT									// Halt the processor

