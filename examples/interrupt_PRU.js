var pru = require('../build/Release/pru');

// Initialise the PRU
pru.init();

// Wait for the PRU to fire an interrupt to tell us it's been inerrupted
pru.waitForInterrupt(function() {
	console.log(pru.getSharedRAMInt(0));
	pru.clearInterrupt();
});

// Start the PRU code
pru.execute("interrupt_PRU.bin");

// Wait 10ms, then interrupt the PRU
setTimeout(function() {
	pru.interrupt();
	console.log("Interrupted PRU");
}, 10);