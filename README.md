


Access the Programmable Realtime Units (PRUs) of the BeagleBone from Node.js
----------------------------------------------------------------------------

This module allows you to interface your Node.js code with programs executing on the BeagleBones Programmable Realtime Units (PRUs). The BeagleBone has 2 PRUs which are separate to the main CPU and run at 200MHz with access to 16 GPIOs each. The benefits of executing code on the PRU are guaranteed realtime execution (outside of the OS) with no load on the primary CPU. The PRUs are coded in [TIs own assembly instruction set](http://processors.wiki.ti.com/index.php/PRU_Assembly_Instructions) and can communicate with code running within the OS via interrupts and shared memory space. 

This README does not aim to be a complete guide to setting up the PRUs and using them from Node.js. As it is mostly built on the [AM335x_PRU Drivers](https://github.com/beagleboard/am335x_pru_package), the [Python PRU bindings](https://bitbucket.org/intelligentagent/pypruss) and the [BBB PRU setup guide](http://www.element14.com/community/community/knode/single-board_computers/next-gen_beaglebone/blog/2013/05/22/bbb--working-with-the-pru-icssprussv2), you can refer to these sources for more information about the PRUs.

The Name of the Game
------------------
This package is essentially a Node.js bindings for the [AM335x_PRU Drivers](https://github.com/beagleboard/am335x_pru_package).

To use this code you need to load *uio_pruss* and (possibly) configure an appropriate *device tree* fragment. Here I assume you know how to
do this.

Working with device tree, loading kernel modules, installing *PASM* tool are outside the scope of this document. This code does (some of the) functions
of the [AM335x_PRU Drivers](https://github.com/beagleboard/am335x_pru_package), that, essentially allow reading/writing the PRU memory, loading and executing PRU firmware, and working with PRU interrupts. 


Installation
------------
To install the module simply type

	npm install prussdrv
	
Credits
-------
This code is a fork of [node-pru](http://github.com/omcaree/node-pru). Since the latter repository seem to be abandoned, I renamed
it (so that it can be pushed to npm registry), cleaned up, simplified, and added few new functions (and possibly few bugs).
