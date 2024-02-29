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
#define INST_WDT 13 //Bytes
#define MEM_WDT 9 //Bytes
#define NUM_OF_REGS 16
#define NUM_OF_IO_REGS 23
#define MONITOR_BUFFER_LEN 256*256


#define DISPLAY7SEGREG 10
#define LEDSREG 9
#define MONITORCMDREG 22

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

char* asmcode[MAX_NUM_OF_LINES]; //PC will read from this array to get current instruction
char* mem[MAX_NUM_OF_LINES]; //write dmemin here, and output this to dmemout
int furthestaddresswritten; //so we know when to stop writing "00000000" in dmemout
char* disk[NUM_OF_SECTORS][SECTOR_SIZE];
int furthestsectorwritten; // so we know when to stop writing "00000000" in diskout
int furthestinsector;
char* regs[NUM_OF_REGS]; //hold regs value, same encoding as in PDF
char* IO[NUM_OF_IO_REGS]; //hold IO regs value, same encoding as in PDF
char* monitor[MONITOR_BUFFER_LEN];
bool irq;
bool in_ISR;
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
    char buffer[MAX_LINE_LEN] = "";
    char* err = fgets(buffer, sizeof(buffer), irq2in);
    if (err == NULL){
        havenextirq2 = false;
        return;
    }
    havenextirq2 = true;
    nextirq2 = strtol(buffer, NULL, 0);
}

void sign_extend_immediate(int immediate, char* immstr){
    int err;
    bool ShouldExtend = (immediate >= 2048); //2^11 = 2048, if condition is true it means MSB is 1 
    if (ShouldExtend){ //we checked if MSB is 1
        err = sprintf(immstr, "FFFFF%3X", immediate);
    }
    else {
        err = sprintf(immstr, "00000%03X", immediate);
    }
    if (err < 0){}//handle error?
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
    char* buffer =(char *) malloc(14*sizeof(char));
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, 14*sizeof(char), imemin) != NULL) {
		//translate instruction to hex code:
        asmcode[row] = (char *) malloc(14*sizeof(char));
        strcpy(asmcode[row], buffer);
		// printf("row %d buffer is %s saved is %s\n", row, buffer, asmcode[row]);
        row++;
	}//where can we check for errors?
    // printf("copied %d rows\n", row);
    for (row; row < MAX_NUM_OF_LINES; row++){
        asmcode[row] = (char *) malloc(14*sizeof(char));
        strcpy(asmcode[row], "");
    }
    free(buffer);
    return true;
}

bool init_mem(){
    char* buffer =(char *) malloc(10*sizeof(char));
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), dmemin) != NULL) {
		//translate instruction to hex code:
		mem[row] = (char *) malloc(10*sizeof(char));
        strcpy(mem[row], buffer);
		row++;
	}//where can we check for errors?
    furthestaddresswritten = row;
    for (row; row < MAX_NUM_OF_LINES; row++){
        mem[row] = (char *) malloc(10*sizeof(char));
        strcpy(mem[row], "00000000");
    }
    free(buffer);
    return true;
}

bool init_disk(){//todo
    char* buffer =(char *) malloc(8*sizeof(char));
	int sector = 0;
    int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), diskin) != NULL) {
		//translate instruction to hex code:
		disk[sector][row] = (char *) malloc(8*sizeof(char));
        strcpy(disk[sector][row], buffer);
		row++;
        if (row == SECTOR_SIZE){
            row = 0;
            sector++;
        }
	}//where can we check for errors?
    furthestsectorwritten = sector;
    furthestinsector = row;
    for (row; sector < 128; row++) {
		//translate instruction to hex code:
		disk[sector][row] = (char *) malloc(8*sizeof(char));
        strcpy(disk[sector][row], buffer);
        if (row == SECTOR_SIZE){
            row = 0;
            sector++;
        }
	}
    free(buffer);
    return true;
}

bool init_misc(){
    int i;
    for (i = 0; i < NUM_OF_REGS; i++){
        regs[i] = (char *) malloc(8*sizeof(char));
        strcpy(regs[i], "00000000");
        // printf("regs[%d] is %s\n", i, regs[i]);
    }
    
    for (i = 0; i < NUM_OF_IO_REGS ; i++){
        IO[i] = (char *) malloc(8*sizeof(char));
        strcpy(IO[i], "00000000");
        // printf("IO[%d] is %s\n", i, IO[i]);
    }

    for (i=0; i < MONITOR_BUFFER_LEN; i++){
        monitor[i] = (char *) malloc(2*sizeof(char));
        strcpy(monitor[i], "00");
    }
    get_next_irq2();
    PC = 0;
    cyclecount = 0;
    irq = false;
    in_ISR = false;
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
    fprintf(hwregtrace, "%d %s %s %s\n", cyclecount, rw, IOreg, IO[IOnum]);
}

void write2hw7seg(){
    fprintf(display7seg, "%d %s\n", cyclecount, IO[DISPLAY7SEGREG]);
}

void write2hwleds(){
    fprintf(leds, "%d %s\n", cyclecount, IO[LEDSREG]);
}

void output2trace(){
    fprintf(trace, "%03X %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", PC, asmcode[PC], regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
}

void write2dmemout(){
    int row = 0;
    for (row; row<furthestaddresswritten; row++){
        fprintf(dmemout, "%s\n", mem[row]);
    }
}

void write2regout(){
    fprintf(regout, "%s %s %s %s %s %s %s %s %s %s %s %s %s\n", regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
}

void write2cycles(){
    fprintf(cycles, "%d\n", cyclecount);
}

void write2diskout(){
    int sector = 0;
    int row = 0;
    for (sector; sector < furthestsectorwritten; sector++){
        for (row = 0; row < 128; row++){
            fprintf(diskout, "%s\n", disk[sector][row]);
        }
    }
    if (furthestsectorwritten != 0) {
        for (row = 0; row < furthestinsector; row++){
            fprintf(cycles, "%s\n", disk[sector][row]);
        }
    }
}

void write2monitor(){
    for (int i = 0; i < MONITOR_BUFFER_LEN; i++){
        fprintf(monitortxt, "%s\n", monitor[i]);
        fprintf(monitoryuv, "%s\n", monitor[i]);
    }
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
    // printf("%s\n", instruction);
    char* temp1 = (char*) malloc(sizeof(char));
    char* temp2 = (char*) malloc(2*sizeof(char));
    char* temp3 = (char*) malloc(3*sizeof(char));

    temp2[0] = instruction[0];
    temp2[1] = instruction[1];
    printf("temp2: first %c, second %c\n", temp2[0], temp2[1]);
    opcode = strtol(temp2, NULL, 16);
    printf("CODE IS %d\n", opcode);

    temp1[0] = instruction[2];
    rd = strtol(temp1, NULL, 16);

    temp1[0] = instruction[3];
    rs = strtol(temp1, NULL, 16);

    temp1[0] = instruction[4];
    rt = strtol(temp1, NULL, 16);

    temp1[0] = instruction[5];
    rm = strtol(temp1, NULL, 16);

    temp3[0] = instruction[6];
    temp3[1] = instruction[7];
    temp3[2] = instruction[8];
    // printf("temp3: first %c, second %c, third %c\n", temp3[0], temp3[1], temp3[2]);
    imm1 = strtol(temp3, NULL, 16);
    // printf("imm1 %d", imm1);
    
    temp3[0] = instruction[9];
    temp3[1] = instruction[10];
    temp3[2] = instruction[11];
    // printf("temp3: first %c, second %c, third %c\n", temp3[0], temp3[1], temp3[2]);
    imm2 = strtol(temp3, NULL, 16);
    // printf("imm2 %d", imm2);
    // printf("values are: oppcode %d, rd %d, rs %d, rt %d, rm %d, imm1 %d, imm2 %d", opcode, rd, rs, rt, rm , imm1, imm2);
    
    free(temp1);
    free(temp2);
    free(temp3);
    return;
}

void prepare_instruction(){
    sign_extend_immediate(imm1, regs[1]);
    sign_extend_immediate(imm2, regs[2]);
}

void save_in_monitor(){
    int monitoraddr = strtol(IO[20], NULL, 16);
    monitor[monitoraddr][0] = IO[21][6];
    monitor[monitoraddr][1] = IO[21][7];//take the last 2 hexa digits in IO reg
}

void handle_input(){
    int regnum = strtol(regs[rt], NULL, 16) + strtol(regs[rs], NULL, 16);
    // printf("HANDLE INPUT ON IO %d\n", regnum);
    if (regnum > 22 || regnum < 0) {return;}//out of bounds
    strcpy(regs[rd], IO[regnum]);
    if (regnum == MONITORCMDREG) {
        strcpy(regs[rd], "00000000");
    }
    write2hwtrace("READ", regnum);
}

void handle_output(){
    int regnum = (strtol(regs[rt], NULL, 16) + strtol(regs[rs], NULL, 16));
    // printf("HANDLE OUTPUT ON IO %d\n", regnum);
    if (regnum > 22 || regnum < 0){return;}//out of bounds
    bool cantwrite = (regnum == 3 || regnum == 4 || regnum == 5 || regnum == 17 || regnum == 18 || regnum == 19);//irq or rsvd or diskstatus
    if (!cantwrite) { return; }
    strcpy(IO[regnum], regs[rd]);
    //conditionally write to led.txt or 7seg.txt
    write2hwtrace("WRITE", regnum);
    switch (regnum){
        case DISPLAY7SEGREG:
            write2hw7seg();
            break;
        case LEDSREG:
            write2hwleds();
            break;
        case MONITORCMDREG:
            if (strcmp(IO[regnum], "00000001") == 0){
                save_in_monitor();
            }
            break;
        default:
            break;
    }
}

void run_instruction(){
    printf("PC IS %d\n", PC);
    printf("opcode is %d\n", opcode);
    printf("rd is %d\n", rd);
    printf("rs is %d\n", rs );
    printf("rt is %d\n", rt);
    printf("rm is %d\n", rm);
    printf("imm1 is %d\n", imm1);
    printf("imm2 is %d\n", imm2);
    printf("regs[rs] is %s\n", regs[rs]);
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
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            };
            PC++;
            break;
        case Sub:
            val = rsval - rtval - rmval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Mac:
            val = rsval * rtval + rmval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case And:
            val = rsval & rtval & rmval;
            if (rdiswritable) {    
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Or:
            val = rsval | rtval | rmval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Xor:
            val = rsval ^ rtval ^ rmval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Sll:
            // printf("SLL\n");
            val = rsval << rtval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Sra: //arithmetic shift with sign extension
            val =  rsval >> rtval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Srl: // logic shift vacant bits filled up with zeros
            val =  ((unsigned int)rsval) >> rtval;
            if (rdiswritable) {
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
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
                err = sprintf(regs[rd], "%08X", PC+1);
                if (err < 0){}//handle error?   
                // printf("JAL1\n");
            }
            PC = rmval;
            // printf("JAL2 NEWPC %d writing to reg %d\n", PC, rd);
            break;
        case Lw:
            // printf("LW\n");
            if (rdiswritable) {
                strcpy(regs[rd], mem[(rsval + rtval) % MAX_NUM_OF_LINES]);
                val = strtol(regs[rd], NULL, 16) + rmval;
                err = sprintf(regs[rd], "%08X", val);
                if (err < 0){}//handle error?
            }
            PC++;
            break;
        case Sw:
            val = strtol(regs[rd], NULL, 16) + rmval;
            err = sprintf(mem[(rsval + rtval) % MAX_NUM_OF_LINES], "%08X", val);
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
            printf("-------------------------------------------------REALLY BAD PLACE TO BE-------------------------------------------------\n");
            return;//invalid opcode. error
    }
}

//---------------------------------------------- PERIPHERALS --------------------------------------//
void write2disk(){
    int sector;
    int MEMaddress;
    int i;
    sector = strtol(IO[15], NULL, 16); // sector that we write to in decimal
    MEMaddress = strtol(IO[16], NULL, 16); // address of the buffer in the main memory that we read from in decimal
    for (i = 0; i < 128; i++) {
        strcpy(disk[sector][i], mem[MEMaddress]);
        MEMaddress++;
    }
    strcpy(IO[14], "00000000");
}

void read_from_disk(){
    int sector;
    int MEMaddress;
    int i;
    sector = strtol(IO[15], NULL, 16); // sector that we read from in decimal
    MEMaddress = strtol(IO[16], NULL, 16); // address of the buffer in the main memory that we write to in decimal
    for (i = 0; i < 128; i++){
        strcpy(mem[MEMaddress], disk[sector][i]);
        MEMaddress++;
    }
    strcpy(IO[14], "00000000");
}

void update_disk_timer(){
    if (strcmp(IO[14], "00000000") == 0 && strcmp(IO[17], "00000001") == 0) { // if we executed the read/write command we wait 1024 cycles while the disk is busy
        disk_timer++;
    }
}

void update_irq(){ // check the irq status and update. only turns them on, the ISR needs to turn them off.
    if (timer == 0) {// irq0
        strcpy(IO[3], "00000001");
    }
    if (disk_timer == 1024 && strcmp(IO[17], "00000001") == 0) {// irq1
        strcpy(IO[4], "00000001"); 
        disk_timer = 0;
        strcpy(IO[17], "00000000");
    }
    if (havenextirq2 && cyclecount == nextirq2) {// irq2
        strcpy(IO[5], "00000001");
        get_next_irq2(); 
    }
    irq = ((strcmp(IO[0],"00000001") == 0 && strcmp(IO[3], "00000001") == 0) || (strcmp(IO[1], "00000001") == 0 && strcmp(IO[4], "00000001") == 0) || (strcmp(IO[2], "00000001") == 0 && strcmp(IO[5], "00000001") == 0));
}
void handle_irq(){ // go to ISR and handle 
    sprintf(IO[7], "%08X", PC);
    PC = strtol(IO[6], NULL, 16);
    in_ISR = true;
}

void update_timer(){ 
    if (strcmp(IO[11], "00000001") == 0){//todo: change to strcmp
        if (timer == max_timer){
            timer = 0;
            strcpy(IO[12], "00000000");
        }
        else{
            timer++;
            sprintf(IO[12],"0x%X", timer); // update timecurrent
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

void run_program(){ //main func that runs while we didn't get HALT instruction
    while(running){
        printf("RUNNING CYCLE %d\n", cyclecount);
        //pre-instruction
        if (PC > MAX_NUM_OF_LINES){ break; } // or error?
        char codedinst[INST_WDT + 10];
        if (irq && !in_ISR){handle_irq();}
        if (PC == strtol(IO[15], NULL, 16)){in_ISR = false;}
        strcpy(codedinst, asmcode[PC]);
        // printf("codedinst is %s\n", codedinst);
        if (codedinst == NULL){ break; } // or error?
        decode_instruction(codedinst);
        prepare_instruction();
        //output
        //writing to output is here since we want to see the immediates in the trace in regs[1], regs[2] but not anything else we have done
        output2trace();

        // printf("codedinst is %s, inst is %d\n", codedinst, opcode);

        //instruction
        run_instruction();
        //post instruction
        handle_parallel_systems();
        // if (cyclecount == 3){break;}
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
/*char* asmcode[MAX_NUM_OF_LINES]; //PC will read from this array to get current instruction
char* mem[MAX_NUM_OF_LINES]; //write dmemin here, and output this to dmemout
int furthestaddresswritten; //so we know when to stop writing "00000000" in dmemout
char* disk[NUM_OF_SECTORS][SECTOR_SIZE];
int furthestsectorwritten; // so we know when to stop writing "00000000" in diskout
int furthestinsector;
char* regs[NUM_OF_REGS]; //hold regs value, same encoding as in PDF
char* IO[NUM_OF_IO_REGS]; //hold IO regs value, same encoding as in PDF
char* monitor[MONITOR_BUFFER_LEN];*/
void free_code(){
    int row = 0;
    for (row; row < MAX_NUM_OF_LINES; row++){
        if (asmcode[row] != NULL){
            free(asmcode[row]);
        }
    }
}

void free_mem(){
    int row = 0;
    for (row; row < MAX_NUM_OF_LINES; row++){
        if (mem[row] != NULL){
            free(mem[row]);
        }
    }
}

void free_disk(){
    int sector, row;
    for (sector = 0; sector<128; sector++){
        for (row = 0; row < MAX_NUM_OF_LINES; row++){
            if (disk[sector][row] != NULL){
                free(disk[sector][row]);
            }
        }
    }
}

void free_regs(){
    int row = 0;
    for (row; row < NUM_OF_REGS; row++){
        if (regs[row] != NULL){
            free(regs[row]);
        }
    }
}

void free_IO(){
    int row = 0;
    for (row; row < NUM_OF_IO_REGS; row++){
        if (IO[row] != NULL){
            free(IO[row]);
        }
    }
}

void free_monitor(){
    int row = 0;
    for (row; row < MONITOR_BUFFER_LEN; row++){
        if (IO[row] != NULL){
            free(IO[row]);
        }
    }
}

void free_all(){
    free_code();
    free_mem();
    free_disk();
    free_regs();
    free_IO();
    free_monitor();
}

void finalize(){ //release everything for program end and print last things for output files if needed
    write_to_output();
    free_all();
    close_files();
}

//----------------------------------------- MAIN ----------------------------------------//

int main(int argc, char* argv[]) {
    if (argc != 15) {
		return EXIT_FAILURE;
	}
    printf("STARTING INIT\n");
    if (!init_files(argv)){
        //error
        return EXIT_FAILURE;
    }
    printf("STARTING INIT 2\n");
    if (!init_values()){
        //error
        return EXIT_FAILURE;
    }
    printf("INIT DONE\n");
    run_program();
    printf("RUN PROGRAM\n");
    finalize();
    printf("FINALIZE\n");
    
    return EXIT_SUCCESS;
};