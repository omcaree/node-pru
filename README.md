Access the Programmable Realtime Units (PRUs) of the BeagleBone from Node.js
============================================================================

This module allows you to interface your Node.js code with programs executing on the BeagleBones Programmable Realtime Units (PRUs). The BeagleBone has 2 PRUs which are separate to the main CPU and run at 200MHz with access to 16 GPIOs each. The benefits of executing code on the PRU are guaranteed realtime execution (outside of the OS) with no load on the primary CPU. The PRUs are coded in (TIs own assembly instruction set)[http://processors.wiki.ti.com/index.php/PRU_Assembly_Instructions] and can communicate with code running within the OS via interrupts and shared memory space. 

This README aims to be a complete guide to setting up the PRUs and using them from Node.js, however this code is mostly built on the [AM335x_PRU Drivers](https://github.com/beagleboard/am335x_pru_package), the [Python PRU bindings](https://bitbucket.org/intelligentagent/pypruss) and the [BBB PRU setup guide](http://www.element14.com/community/community/knode/single-board_computers/next-gen_beaglebone/blog/2013/05/22/bbb--working-with-the-pru-icssprussv2). Refer to these sources for more information about the PRUs.

Prerequisites
-------------
The PRU system is supported on both the BeagleBone and BeagleBone Black. This README is written for the BeagleBone Black (BBB) running Ubuntu. Instructions for the BealgeBone (white) and Angstrom may differ slightly.

### Kernel Module ###
The *uio_pruss* kernel module must be loaded before any work can commence. This module is present in the default Ubuntu build (and probably many more), but is not loaded by default. Load the module with

	modprobe uio_pruss
	
And to avoid the need to do this in the future (the drivers tend to seg fault if you forget!), add the module to the end of */etc/modules*.

### Device Tree ###
The most difficult part of setting up the PRUs on the BBB involves setting up the device tree. The instructions [here](http://www.element14.com/community/community/knode/single-board_computers/next-gen_beaglebone/blog/2013/05/22/bbb--working-with-the-pru-icssprussv2) are pretty easy to follow. What follows is what I did to enable PRU0 and set pins 25, 27, 28, 29, 30 and 31 of the P9 expansion header to outputs. I did not use the device tree overlay approach as I am simply building a custom image, the approach I used is quicker but less portable.

Firstly, get hold of the sourcecode for the BBB device tree (details [here](http://blog.pignology.net/2013/05/getting-uart2-devttyo1-working-on.html))

	wget http://pignology.net/blackdts.tgz
	tar xvzf blackdts.tgz
	cd blackdts
	
Now install the device-tree-compiler. On Ubuntu, do this with

	sudo apt-get install device-tree-compiler
	
Next, open up *am335x-bone-common.dtsi*. In the section named *am33xx_pinmux: pinmux@44e10800* add the following to set the pinmuxing

	pruicss_pins: pinmux_pruicss_pins {
		pinctrl-single,pins = <
			0x190 0x05	/* P9_31 to PRU output */
			0x194 0x05	/* P9_29 to PRU output */
			0x198 0x05	/* P9_30 to PRU output */
			0x19C 0x05	/* P9_28 to PRU output */
			0x1A4 0x05	/* P9_27 to PRU output */
			0x1AC 0x05	/* P9_25 to PRU output */
			>;
	};
	
Then to enable PRU0, add the following to the *ocp: ocp* section

	pruss: pruss@4a300000 {
		status = "okay";
		pinctrl-names = "default";
		pinctrl-0 = <&pruicss_pins>;
	};
	
Save and close the file, compile it with

	dtc -O dtb -o am335x-boneblack.dtb -b 0 am335x-boneblack.dts

Finally, backup your old binary and replace it with the one you just compiled

	sudo mv /boot/uboot/dtbs/am335x-boneblack.dtb /boot/uboot/dtbs/am335x-boneblack.orig.dtb
	sudo mv am335x-boneblack.dtb /boot/uboot/dtbs/

After a reboot, PRU0 will be enabled and the pinmuxing set.

### Driver library and assembler ###
Get the driver and assembler code

	git clone https://github.com/beagleboard/am335x_pru_package.git
	cd am335x_pru_package
	
Apply the following patch to prevent interrupts being fired twice by the driver

	wget http://e2e.ti.com/cfs-file.ashx/__key/telligent-evolution-components-attachments/00-791-00-00-00-23-97-35/attachments.tar.gz
	tar -xzf attachments.tar.gz
	patch -p1 <  0001-Fix-for-duplicated-interrupts-when-interrupts-are-se.patch 

Compile the driver as a shared library (don't use *make*, this builds a static library which node-gyp does not like!)

	cd pru_sw/app_loader/interface/
	gcc -I. -Wall -I../include   -c -fPIC -O3 -mtune=cortex-a8 -march=armv7-a -shared -o prussdrv.o prussdrv.c
	gcc -shared -o libprussdrv.so prussdrv.o

Copy the driver and headers to system folders

	sudo cp libprussdrv.so /usr/lib/
	sudo cp ../include/*.h /usr/include/
	
Now build the assember

	cd ../../utils/pasm_source
	./linuxbuild
	
Copy the assembler to system

	sudo cp ../pasm /usr/bin/

Finally, test the PRU system with one of the examples.

	cd ../../example_apps/PRU_memAccess_DDR_PRUsharedRAM

Assemble the PRU code

	pasm -b PRU_memAccess_DDR_PRUsharedRAM.p

This will generate the PRU binary *PRU_memAccess_DDR_PRUsharedRAM.bin*. Now compile the C code

	gcc PRU_memAccess_DDR_PRUsharedRAM.c -lprussdrv -lpthread -otest

Run the example (must run as root to access the PRU)

	sudo ./test
	
If all goes well you should see the following

	INFO: Starting PRU_memAccess_DDR_PRUsharedRAM example.
	AM33XX
			INFO: Initializing example.
			INFO: Executing example.
	File ./PRU_memAccess_DDR_PRUsharedRAM.bin open passed
			INFO: Waiting for HALT command.
			INFO: PRU completed transfer.
	Example executed succesfully.

Your system is now set up to use the PRU, now we can start using Node.JS

Installation
------------
To install the module simply type
	npm install pru
	
Usage
-------
### Example ###
A simple example is given in the examples folder, *timing_test.js* and *timing_test.p*. The example simply loops the PRU very quickly and prints the status of the countdown every second.

First assemble the PRU code
	pasm -b timing_test.p
	
Then run the example

	cd node_modules/pru/examples
	sudo node timing_test.js
	
Remeber to run as root or you'll get a seg fault!

### Module description ###
To include the module in your own code, simply use 

	var pru = require('pru');
	
Before you can do anything with the PRU you must initialise it with

	pru.init();
	
Execute a binary named "mycode.bin"

	pru.execute("mycode.bin");
	
Set the shared memory space to an array of integers ([0x1 0x2 0x3])

	pru.setSharedRAM([0x1 0x2 0x3]);
	
And to set the 6th integer in the RAM to 0x10

	pru.setSharedRAMInt(5, 0x10);
	
Get an array from the RAM, or the 4rd value

	var ramArray = pru.getSharedRAM();
	var ramElement = pru.getSharedRAMInt(3);
	
Set a callback to fire when the PRU generates an interupt

	pru.waitForInterrupt(function() {
		console.log("Interrupted by PRU");
		});
		
Terminate the PRU execution

	pru.exit();
	
Limitations
-----------
* Currently only PRU0 is supported
* Only the Shared Memory space can be used for communication
* There is no support (yet) for node.js to interrupt the PRU
* The Shared Memory getters/setters are limited to integers
