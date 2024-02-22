#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_NUM_OF_LINES 4096
#define NUM_OF_SECTORS 128
#define INST_WDT 12 //in Bytes
#define MEM_WDT 8 //in Bytes
char* asmcode[MAX_NUM_OF_LINES] = { NULL }; //PC will read from this array to get current instruction
char* mem[MAX_NUM_OF_LINES] = { "00000000" }; //write dmemin here, and output this to dmemout
int furthestaddresswritten; //so we know when to stop writing "00000000" in dmemout
char* disk[NUM_OF_SECTORS];
int furthestsectorwritten; // so we know when to stop writing "00000000" in diskout
char* regs[16]; //hold regs value, same encoding as in PDF
char* IO[23]; //hold IO regs value, same encoding as in PDF
int nextirq2;
bool havenextirq2;
int PC;
int cyclecount;
bool running = true;
FILE *imemin, *dmemin, *diskin, *irq2in; //input files
FILE *dmemout, *regout, *trace, *hwregtrace, *cycles, *leds, *display7seg, *diskout, *monitortxt, *monitoryuv; //output files

bool init_files(char* argv[]){ //open all FILE structs. return false if failed
    
    //input files
    imemin = fopen(argv[1], "r");
	if (imemin == NULL) {
		return false;
	}
    dmemin = fopen(argv[2], "r");
	if (dmemin == NULL) {
		return false;
	}
    diskin = fopen(argv[3], "r");
	if (diskin == NULL) {
		return false;
	}
    irq2in = fopen(argv[4], "r");
	if (irq2in == NULL) {
		return false;
	}

    //output files
    dmemout = fopen(argv[5], "w"); 
	if (dmemout == NULL) {
		return false;
	}
    regout = fopen(argv[6], "w"); 
	if (regout == NULL) {
		return false;
	}
    trace = fopen(argv[7], "w"); 
	if (trace == NULL) {
		return false;
	}
    hwregtrace = fopen(argv[8], "w"); 
	if (hwregtrace == NULL) {
		return false;
	}
    cycles = fopen(argv[9], "w"); 
	if (cycles == NULL) {
		return false;
	}
    leds = fopen(argv[10], "w"); 
	if (leds == NULL) {
		return false;
	}
    display7seg = fopen(argv[11], "w"); 
	if (display7seg == NULL) {
		return false;
	}
    diskout = fopen(argv[12], "w"); 
	if (diskout == NULL) {
		return false;
	}
    monitortxt = fopen(argv[13], "w"); 
	if (monitortxt == NULL) {
		return false;
	}
    monitoryuv = fopen(argv[14], "w"); 
	if (monitoryuv == NULL) {
		return false;
	}

    return true;
}

bool init_code(){
    char buffer[INST_WDT];
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), imemin) != NULL) {
		//translate instruction to hex code:
		strcpy(asmcode[row], buffer);
		row++;
	}//where can we check for errors?
    return true;
}

bool init_mem(){
    char buffer[MEM_WDT];
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), dmemin) != NULL) {
		//translate instruction to hex code:
		strcpy(mem[row], buffer);
		row++;
	}//where can we check for errors?
    return true;
}

bool init_disk(){
    char buffer[MEM_WDT];
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), diskin) != NULL) {
		//translate instruction to hex code:
		strcpy(disk[row], buffer);
		row++;
	}//where can we check for errors?
    return true;
}

bool init_misc(){

}

bool init_values(){ //init values of regs, IO, and copy imemin and dmemin into asmcode and mem. return false if failed
    if (!init_code()){
        return false;
    }
    if (!init_mem()){
        return false;
    }
    if (!init_disk()){
        return false;
    }
    if (!init_misc()){
        return false;
    }
    return true;
}

bool run_program(){ //main func that runs while we didn't get HALT instruction
    while(running){
        //split into 4 parts:
        //get all args needed from asmcode[PC]: translate from opcode to args
        //run instruction: switch case depending on opcode !!!PC IS UPDATED HERE!!!
        //run all parallel auxilary systems: timer, disk, INTERRUPTS!!! - PC maybe updated here if interrupted
        //write to output files all that is relevant
        int opcode, rd, rs, rt, rm, imm1, imm2; //the 7 arguments per instruction
        

    }
}

bool finalize(){ //release everything for program end and print last things for output files if needed

}














int main(int argc, char* argv[]) {
    if (argc != 15) {
		return EXIT_FAILURE;
	}

    if (!init_files(argv)){
        //error
        return EXIT_FAILURE;
    }

    if (!init_values()){
        //error
        return EXIT_FAILURE;
    }

    if (!run_program()){
        //error
        return EXIT_FAILURE
    }

    if (!finalize()){
        //error
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
};