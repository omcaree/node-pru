//System headers
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <pthread.h>

//PRU Driver headers
#include <prussdrv.h>
#include <pruss_intc_mapping.h>	 
#define OFFSET_SHAREDRAM 2048
#define PRU_NUM 	 0


//Node.js addon headers
#include <node.h>
#include <v8.h>

using namespace v8;

//shared memory pointer
static unsigned int* sharedMem_int;

/* Initialise the PRU
 *	Initialise the PRU driver and static memory
 *	Takes no arguments and returns nothing
 */
Handle<Value> InitPRU(const Arguments& args) {
	HandleScope scope;
	
	//Initialise driver
	prussdrv_init ();
	
	//Open interrupt
	unsigned int ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret) {
		printf("prussdrv_open open failed\n");
		return scope.Close(Undefined());
	}
	
	//Initialise interrupt
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;	
	prussdrv_pruintc_init(&pruss_intc_initdata);
	
	// Allocate shared PRU memory
	static void *sharedMem;
    prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &sharedMem);
    sharedMem_int = (unsigned int*) sharedMem;
	
	//Return nothing
	return scope.Close(Undefined());
}

/* Execute PRU program
 *	Takes a single string argument, the filename of the .bin
 */
Handle<Value> executeProgram(const Arguments& args) {
	HandleScope scope;
	
	//Check we have a single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	//Check that it's a string
	if (!args[0]->IsString()) {
		ThrowException(Exception::TypeError(String::New("Argument must be a string")));
		return scope.Close(Undefined());
	}
	
	//Get a C++ string
	String::Utf8Value program(args[0]->ToString());
	std::string programS = std::string(*program);
	
	//Execute the program
	prussdrv_exec_program (PRU_NUM, (char*)programS.c_str());
	
	//Return nothing
	return scope.Close(Undefined());
};


/* Set the shared PRU RAM to an input array
 *	Takes an integer array as input, writes it to PRU shared memory
 *	Not much error checking here, don't pass in large arrays or seg faults will happen
 *	TODO: error checking and allow user to select range to set
 */
Handle<Value> setSharedRAM(const Arguments& args) {
	HandleScope scope;
	
	//Check we have a single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check that it's an array
	if (!args[0]->IsArray()) {
		ThrowException(Exception::TypeError(String::New("Argument must be array")));
		return scope.Close(Undefined());
	}
	
	//Get array
	Local<Array> a = Array::Cast(*args[0]);
	
	//Iterate over array
	for (unsigned int i = 0; i<a->Length(); i++) {
		//Get element and check it's numeric
		Local<Value> element = a->Get(i);
		if (!element->IsNumber()) {
			ThrowException(Exception::TypeError(String::New("Array must be integer")));
			return scope.Close(Undefined());
		}
		
		//Set corresponding memory bytes
		sharedMem_int[OFFSET_SHAREDRAM + i] = (unsigned int) element->NumberValue();
	}
	
	//Return nothing
	return scope.Close(Undefined());
};

/* Set a single integer value in shared RAM
 *	Takes two integer arguments, index and value
 */
Handle<Value> setSharedRAMInt(const Arguments& args) {	//array
	HandleScope scope;
	
	//Check that we have 2 arguments
	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check they are both numbers
	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Arguments must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get the numbers
	unsigned short index = (unsigned short)Array::Cast(*args[0])->NumberValue();
	unsigned int val = (unsigned int)Array::Cast(*args[1])->NumberValue();
	
	//Set the memory date
	sharedMem_int[OFFSET_SHAREDRAM + index] = val;
	
	//Return nothing
	return scope.Close(Undefined());
};

/* Get array from shared memory
 *	Returns first 16 integers from shared memory
 *	TODO: should take start and size integers as input to let user select array size
 */
Handle<Value> getSharedRAM(const Arguments& args) {	//array
	HandleScope scope;
	
	//Create output array
	Local<Array> a = Array::New(16);
	
	//Iterate over output array and fill it with shared memory data
	for (unsigned int i = 0; i<a->Length(); i++) {
		a->Set(i,Number::New(sharedMem_int[OFFSET_SHAREDRAM + i]));
	}
	
	//Return array
	return scope.Close(a);
};

/* Get single integer from shared memory
 *	Takes integer index argument, returns array at that index
 */
Handle<Value> getSharedRAMInt(const Arguments& args) {	//array
	HandleScope scope;
	
	//Check we have single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check it's a number
	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Argument must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get index value
	unsigned short index = (unsigned short)Array::Cast(*args[0])->NumberValue();
	
	//Return memory
	return scope.Close(Number::New(sharedMem_int[OFFSET_SHAREDRAM + index]));
};


/*-------------------This is mostly copy/pasted from here: ---------------------*/
/*----------------http://kkaefer.github.io/node-cpp-modules/--------------------*/
struct Baton {
    uv_work_t request;
    Persistent<Function> callback;
    int error_code;
    std::string error_message;
    int32_t result;
};

void AsyncWork(uv_work_t* req) {
//    Baton* baton = static_cast<Baton*>(req->data);
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
}

void AsyncAfter(uv_work_t* req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);
	baton->callback->Call(Context::GetCurrent()->Global(), 0, 0);
    baton->callback.Dispose();
    delete baton;
}

Handle<Value> waitForInterrupt(const Arguments& args) {
	HandleScope scope;
	Local<Function> callback = Local<Function>::Cast(args[0]);

	Baton* baton = new Baton();
    baton->request.data = baton;
    baton->callback = Persistent<Function>::New(callback);
	
	uv_queue_work(uv_default_loop(), &baton->request, AsyncWork, AsyncAfter);
	return scope.Close(Undefined());
}

/*---------------------------Here ends the copy/pasting----------------------------*/

/* Clear Interrupt */
Handle<Value> clearInterrupt(const Arguments& args) {
	HandleScope scope;
	
	//Check we have single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check it's a number
	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Argument must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get index value
	int event = (int) Array::Cast(*args[0])->NumberValue();
	
	prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT, event);
	return scope.Close(Undefined());
};

Handle<Value> interruptPRU(const Arguments& args) {
	HandleScope scope;
	prussdrv_pru_send_event(ARM_PRU0_INTERRUPT);
	return scope.Close(Undefined());
};


/* Force the PRU code to terminate */
Handle<Value> forceExit(const Arguments& args) {
	HandleScope scope;
	prussdrv_pru_disable(PRU_NUM); 
    prussdrv_exit ();
	return scope.Close(Undefined());
};

/* Initialise the module */
void Init(Handle<Object> exports, Handle<Object> module) {
	//	pru.init();
	exports->Set(String::NewSymbol("init"), FunctionTemplate::New(InitPRU)->GetFunction());
	
	//	pru.execute("mycode.bin");
	exports->Set(String::NewSymbol("execute"), FunctionTemplate::New(executeProgram)->GetFunction());
	
	//	pru.setSharedRAM([0x1, 0x2, 0x3]);
	exports->Set(String::NewSymbol("setSharedRAM"), FunctionTemplate::New(setSharedRAM)->GetFunction());
	
	// var intArray = pru.getSharedRAM();
	exports->Set(String::NewSymbol("getSharedRAM"), FunctionTemplate::New(getSharedRAM)->GetFunction());
	
	//	pru.setSharedRAM(4, 0xff);
	exports->Set(String::NewSymbol("setSharedRAMInt"), FunctionTemplate::New(setSharedRAMInt)->GetFunction());
	
	//	var intVal = pru.getSharedRAM(3);
	exports->Set(String::NewSymbol("getSharedRAMInt"), FunctionTemplate::New(getSharedRAMInt)->GetFunction());
	
	//	pru.waitForInterrupt(function() { console.log("Interrupted by PRU");});
	exports->Set(String::NewSymbol("waitForInterrupt"), FunctionTemplate::New(waitForInterrupt)->GetFunction());

	//	pru.clearInterrupt();
	exports->Set(String::NewSymbol("clearInterrupt"), FunctionTemplate::New(clearInterrupt)->GetFunction());
	
	//	pru.interrupt();
	exports->Set(String::NewSymbol("interrupt"), FunctionTemplate::New(interruptPRU)->GetFunction());	
	
	//	pru.exit();
	exports->Set(String::NewSymbol("exit"), FunctionTemplate::New(forceExit)->GetFunction());
}

NODE_MODULE(pru, Init)
