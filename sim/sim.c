#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_NUM_OF_LINES 4096
#define NUM_OF_SECTORS 128
#define SECTOR_SIZE 512 //Bytes
#define INST_WDT 12 //Bytes
#define MEM_WDT 8 //Bytes
char* asmcode[MAX_NUM_OF_LINES] = { NULL }; //PC will read from this array to get current instruction
char* mem[MAX_NUM_OF_LINES] = { "00000000" }; //write dmemin here, and output this to dmemout
int furthestaddresswritten; //so we know when to stop writing "00000000" in dmemout
char* disk[NUM_OF_SECTORS][SECTOR_SIZE];
int furthestsectorwritten; // so we know when to stop writing "00000000" in diskout
int sectordepth;
char* regs[16]; //hold regs value, same encoding as in PDF
char* IO[23]; //hold IO regs value, same encoding as in PDF
int nextirq2;
bool havenextirq2;
int PC;
int cyclecount;
bool running;
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
    furthestaddresswritten = row;
    return true;
}

bool init_disk(){//todo
    char buffer[MEM_WDT];
	int sector = 0;
    int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), diskin) != NULL) {
		//translate instruction to hex code:
		strcpy(disk[sector][row], buffer);
		row++;
        if (row == SECTOR_SIZE){
            row = 0;
            sector++;
        }
	}//where can we check for errors?
    furthestsectorwritten = sector;
    sectordepth = row;
    return true;
}

void get_next_irq2(){
    char buffer[MAX_LINE_LEN] = NULL;
    char* err = fgets(buffer, sizeof(buffer), irq2in);
    if (err == NULL){
        havenextirq2 = false;
        return;
    }
    havenextirq2 = true;
    nextirq2 = strtol(buffer, NULL, 0);
}

bool init_misc(){
    int i = 0;

    for (i; i < 16; i++){
        regs[i] = "00000000";
    }
    
    for (i = 0; i < 6 ; i++){
        IO[i] = "0"; //1 bit
    }
    for (i = 6; i < 8 ; i++){
        IO[i] = "000"; //12 bit
    }
    for (i = 8; i < 11 ; i++){
        IO[i] = "00000000"; //32 bit
    }
    for (i = 12; i < 14 ; i++){
        IO[i] = "00000000"; //32 bit
    }
    
    IO[11] = "0"; //1 bit
    IO[14] = "0"; //2 bit
    IO[15] = "00"; //7 bit
    IO[16] = "000"; //12 bit
    IO[17] = "0"; //1 bit
    IO[18] = "0"; //rsvd
    IO[19] = "0"; //rsvd
    IO[20] = "0000"; //16 bit
    IO[21] = "00"; //8 bit
    IO[22] = "0"; //1 bit
    

    get_next_irq2();
    PC = 0;
    cyclecount = 0;
    running = true;
    return true;
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