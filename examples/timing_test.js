var pru = require('../build/Release/pru');

// Initialise the PRU
pru.init();

// Set the first integer element of shared RAM to 3E9
// This is the number of loops the PRU should perform (see timing_test.p)
// This should correspond to exactly 60s runtime (4 instructions at 5ns/instruction)
pru.setSharedRAMInt(0, 3E9);

// Wait for the PRU to fire an interrupt to tell us it's done
pru.waitForInterrupt(function() {
	// Print the time taken
	var t_end = new Date().getTime();
	console.log("Done in " + (t_end - t_start) + "ms");
	
	// Stop the counter
	clearInterval(counter);
});

// Start the PRU code
pru.execute("timing_test.bin");

// Get the start time
var t_start = new Date().getTime();

// Print the current value of the counter every second until completion
var counter = setInterval(function() {
	var c = pru.getSharedRAMInt(0)
	console.log("\tCounter is " + c + " (" + (c*4*5E-9).toFixed(3) + "s remaining)");
}, 1000);