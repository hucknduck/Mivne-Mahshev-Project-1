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
#define NUM_OF_REGS 16
#define NUM_OF_IO_REGS 23

#define DISPLAY7SEGREG 10
#define LEDSREG 9

#define Add 0
#define Sub 1
#define Mac 2
#define And 3
#define Or 4
#define Xor 5
#define Sll 6
#define Sra 7
#define Srl 8
#define Beq 9
#define Bne 10
#define Blt 11
#define Bgt 12
#define Ble 13
#define Bge 14
#define Jal 15
#define Lw 16
#define Sw 17
#define Reti 18
#define In 19
#define Out 20
#define Halt 21

char* asmcode[MAX_NUM_OF_LINES] = { NULL }; //PC will read from this array to get current instruction
char* mem[MAX_NUM_OF_LINES] = { "00000000" }; //write dmemin here, and output this to dmemout
int furthestaddresswritten; //so we know when to stop writing "00000000" in dmemout
char* disk[NUM_OF_SECTORS][SECTOR_SIZE];
int furthestsectorwritten; // so we know when to stop writing "00000000" in diskout
char* regs[NUM_OF_REGS]; //hold regs value, same encoding as in PDF
char* IO[NUM_OF_IO_REGS]; //hold IO regs value, same encoding as in PDF
bool irq;
int nextirq2;
bool havenextirq2;
int PC;
int cyclecount;
int disk_timer;
int timer;
int max_timer;
bool running;
FILE *imemin, *dmemin, *diskin, *irq2in; //input files
FILE *dmemout, *regout, *trace, *hwregtrace, *cycles, *leds, *display7seg, *diskout, *monitortxt, *monitoryuv; //output files

int opcode, rd, rs, rt, rm, imm1, imm2; //the 7 arguments per instruction

// -------------------------------------------------- HELPERS ------------------------------------------//
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

char * sign_extend_immediate(int immediate){
    char * out;
    int err;
    err = sprintf(out, "%3X", immediate);//immediates are 12 bits = 3 hex digits
    if (err < 0){}//handle error?
    bool ShouldExtend = (out[0] == '9' || out[0] == '8');
    ShouldExtend |= (out[0] >= 'A' && out[0] <= 'F');
    ShouldExtend |= (out[0] >= 'a' && out[0] <= 'f');
    if (ShouldExtend){ //we checked if MSB is 1
        out = strcat("FFFFF", out);
    }
    else {
        out = strcat("00000", out);
    }
    return out;
}

char* get_IO_reg_name(int regnum){
    switch (regnum){
        case 0:
            return "irq0enable";
        case 1:
            return "irq1enable";
        case 2:
            return "irq2enable";
        case 3:
            return "irq0status";
        case 4:
            return "irq1status";
        case 5:
            return "irq2status";
        case 6:
            return "irqhandler";
        case 7:
            return "irqreturn";
        case 8:
            return "clks";
        case 9:
            return "leds";
        case 10:
            return "display7seg";
        case 11:
            return "timerenable";
        case 12:
            return "timercurrent";
        case 13:
            return "timermax";
        case 14:
            return "diskcmd";
        case 15:
            return "disksector";
        case 16:
            return "diskbuffer";
        case 17:
            return "diskstatus";
        case 18:
            return "reserved";
        case 19:
            return "reserved";
        case 20:
            return "monitoraddr";
        case 21:
            return "monitordata";
        case 22:
            return "monitorcmd";
        default:
            return NULL; //error
    }
}
//---------------------------------------------------- INIT -----------------------------------------//

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
    return true;
}

bool init_misc(){
    int i;
    for (i = 0; i < NUM_OF_REGS; i++){
        regs[i] = "00000000";
    }
    
    for (i = 0; i < NUM_OF_IO_REGS ; i++){
        IO[i] = "00000000";
    }

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

//------------------------------------------------OUTPUT-------------------------------------------------//

void write2hwtrace(char* rw, int IOnum) {
    char* IOreg = get_IO_reg_name(IOnum);
    fprintf(trace, "%d %s %s %s\n", cyclecount, rw, IOreg, IO[IOnum]);
}

void write2hw7seg(){
    fprintf(display7seg, "%d %s\n", cyclecount, IO[DISPLAY7SEGREG]);
}

void write2hwleds{
    fprintf(leds, "%d %s\n", cyclecount, IO[LEDSREG]);
}

void output2trace(){
    fprintf(trace, "%03X %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", PC, asmcode[PC], regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
}

void write_to_output(){
    //write at end: *dmemout, *regout,   *cycles,   *diskout, *monitortxt, *monitoryuv;
    write2dmemout();
    write2regout();
    write2cycles();
    write2diskout();
    write2monitor();
}

//-------------------------------------------------------- CPU RUN -------------------------------------------------//

void decode_instruction(char* instruction){
    //opcode: 2 digits, rd: 1 digit, rs: 1 digit, rt: 1 digit, rm: 1 digit, imm1: 3 digits, imm2: 3 digits
    //000000000000
    char instructionbytes[INST_WDT + 1];
    strcpy(instructionbytes, instruction);
    
    char* start = instructionbytes + 9;
    imm2 = strtol(start, NULL, 16);
    instructionbytes[9] = NULL;
    
    start = instructionbytes + 6;
    imm1 = strtol(start, NULL, 16);
    instructionbytes[6] = NULL;
    
    start -= 1;
    rm = strtol(start, NULL, 16);
    instructionbytes[5] = NULL;
    
    start -= 1;
    rt = strtol(start, NULL, 16);
    instructionbytes[4] = NULL;
    
    start -= 1;
    rs = strtol(start, NULL, 16);
    instructionbytes[3] = NULL;
    
    start -= 1;
    rd = strtol(start, NULL, 16);
    instructionbytes[2] = NULL;
    
    opcode = strtol(instructionbytes, NULL, 16);

    //not sure if this works, pretty disgusting code
}

void prepare_instruction(){
    regs[1] = sign_extend_immediate(imm1);
    regs[2] = sign_extend_immediate(imm2);
}

void handle_input(){
    int regnum = strtol(regs[rt], NULL, 16) + strtol(regs[rs], NULL, 16);
    if (regnum > 22 || regnum < 0) {return;}//out of bounds
    regs[rd] = IO[regnum];
    write2hwtrace("READ", regnum);
}

void handle_output(){
    int regnum = (strtol(regs[rt], NULL, 16) + strtol(regs[rs], NULL, 16));
    if (regnum > 22 || regnum < 0){return;}//out of bounds
    bool cantwrite = (regnum == 3 || regnum == 4 || regnum == 5 || regnum == 17 || regnum == 18 || regnum == 19);//irq or rsvd or diskstatus
    if (!cantwrite) { return; }
    IO[regnum] = regs[rd];
    //conditionally write to led.txt or 7seg.txt
    write2hwtrace("WRITE", regnum);
    switch (regnum){
        case DISPLAY7SEGREG:
            write2hw7seg();
            break;
        case LEDSREG:
            write2hwleds();
            break;
        default:
            break;
    }
}

void run_instruction(){
    int val;
    bool branch;
    bool rdiswritable = !(rd == 0 || rd == 1 || rd == 2);//false when trying to write to $zero or $imm1,2
    int err;
    int rsval = strtol(regs[rs], NULL, 16);
    int rtval = strtol(regs[rt], NULL, 16);
    int rmval = strtol(regs[rm], NULL, 16);
    switch (opcode){
        case Add:
            val = rsval + rtval + rmval;
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            };
            PC++;
            break;
        case Sub:
            val = rsval - rtval - rmval;
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Mac:
            val = rsval * rtval + rmval;
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case And:
            val = rsval & rtval & rmval;
            if (rdiswritable) {    
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Or:
            val = rsval | rtval | rmval;
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Xor:
            val = rsval ^ rtval ^ rmval;
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Sll:
            val = rsval << rtval;
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Sra:
            if (rdiswritable) {
                //todo
            }
            PC++;
            break;
        case Srl:
            if (rdiswritable) {
                //todo
            }
            PC++;
            break;
        case Beq:
            branch = (rsval == rtval);
            PC = branch? rmval : PC+1;
            break;
        case Bne:
            branch = (rsval != rtval);
            PC = branch? rmval : PC+1;
            break;
        case Blt:
            branch = (rsval < rtval);
            PC = branch? rmval : PC+1;
            break;
        case Bgt:
            branch = (rsval > rtval);
            PC = branch? rmval : PC+1;
            break;
        case Ble:
            branch = (rsval <= rtval);
            PC = branch? rmval : PC+1;
            break;
        case Bge:
            branch = (rsval >= rtval);
            PC = branch? rmval : PC+1;
            break;
        case Jal:
            if (rdiswritable) {
                err = sprintf(&regs[rd], "%08X", PC+1);
                if (err < 0){}//handle error?   
            }
            PC = rmval;
            break;
        case Lw:
            if (rdiswritable) {
                regs[rd] = mem[(rsval + rtval) % MAX_NUM_OF_LINES];
                val = strtol(regs[rd], NULL, 16) + rmval;
                err = sprintf(&regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Sw:
            val = strtol(regs[rd], NULL, 16) + rmval;
            err = sprintf(&mem[(rsval + rtval) % MAX_NUM_OF_LINES], "%08X", val);
            if (err < 0){}//handle error?
            PC++;
            break;
        case Reti:
            PC = strtol(IO[7], NULL, 16);
            break;
        case In:
            if (rdiswritable) {
                handle_input();
            }
            PC++;
            break;
        case Out:
            handle_output();
            PC++;
            break;
        case Halt:
            running = false;
            break;
        default:
            return;//invalid opcode. error
    }
}

 //---------------------------------------------- PERIPHERALS --------------------------------------//

void write2disk(){
    long sector;
    long MEMaddress;
    int i;
    sector = strtol(IO[15], NULL, 16); // sector that we write to in decimal
    MEMaddress = strtol(IO[16], NULL, 16); // address of the buffer in the main memory that we read from in decimal
    for (i = 0; i < 128; i++) {
        disk[sector][i] = mem[MEMaddress];
        MEMaddress++;
    }
    IO[14] = "00";
}

void read_from_disk(){
    long sector;
    long MEMaddress;
    int i;
    sector = strtol(IO[15], NULL, 16); // sector that we read from in decimal
    MEMaddress = strtol(IO[16], NULL, 16); // address of the buffer in the main memory that we write to in decimal
    for (i = 0; i < 128; i++){
        mem[MEMaddress] = disk[sector][i];
        MEMaddress++;
    }
    IO[14] = "00";
}

void update_disk_timer(){
    if (IO[14] = "00" && IO[17] == "1") { // if we executed the read/write command we wait 1024 cycles while the disk is busy
        disk_timer++;
    }
}

void update_irq(){ // check the irq status and update. only turns them on, the ISR needs to turn them off.
    if (timer == 0) {// irq0
        IO[3] = "1";
    }
    if (disk_timer == 1024 && IO[17] == "1") {// irq1
        IO[4] = "1"; 
        disk_timer = 0;
        IO[17] == "0";
    }
    if (havenextirq2 && cyclecount == nextirq2) {// irq2
        IO[5] = "1";
        get_next_irq2(); 
    }
    irq = ((IO[0] == "1" && IO[3] == "1") || (IO[1] == "1" && IO[4] == "1") || (IO[2] == "1" && IO[5] == "1"));
}

void update_timer(){ 
    if (IO[11] == "1"){//todo: change to strcmp
        if (timer == max_timer){
            timer = 0;
            IO[12] = "00000000"
        }
        else{
            timer++;
            sprintf(IO[12],"%x", timer); // update timecurrent
        }
    }
}

void handle_parallel_systems(){
    cyclecount++;
    update_timer();
    update_disk_timer();
    update_irq();
}

//-------------------------------- MAIN LOOP -----------------------------------//

bool run_program(){ //main func that runs while we didn't get HALT instruction
    while(running){
        //pre-instruction
        //todo: add handle irq. maybe updates PC
        
        
        
        if (PC > MAX_NUM_OF_LINES){ break; } // or error?
        char* codedinst;
        strcpy(codedinst, asmcode[PC]);
        if (codedinst == NULL){ break; } // or error?
        decode_instruction(codedinst);
        prepare_instruction();
        
        //output
        //writing to output is here since we want to see the immediates in the trace in regs[1], regs[2] but not anything else we have done
        output2trace();

        //instruction
        run_instruction();

        //post instruction
        handle_parallel_systems();
    }
}

//------------------------------------FINALIZE-------------------------------------------//
void close_files(){
    fclose(imemin);
	fclose(dmemin);
	fclose(diskin);
	fclose(irq2in);
	fclose(dmemout);
	fclose(regout);
	fclose(trace);
	fclose(hwregtrace);
	fclose(cycles);
	fclose(leds);
	fclose(display7seg);
	fclose(diskout);
	fclose(monitortxt);
	fclose(monitoryuv);
}

bool finalize(){ //release everything for program end and print last things for output files if needed
    write_to_output();
    close_files();
}

//----------------------------------------- MAIN ----------------------------------------//

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