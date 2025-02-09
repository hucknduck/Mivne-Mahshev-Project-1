#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_NUM_OF_LINES 4096
#define NUM_OF_SECTORS 128
#define SECTOR_SIZE 128 //words
#define INST_WDT 13 //Bytes + 1 for '/0'
#define MEM_WDT 9 //Bytes + 1 for '/0'
#define NUM_OF_REGS 16
#define NUM_OF_IO_REGS 23
#define MONITOR_BUFFER_LEN 256*256

#define LEDSREG 9
#define DISPLAY7SEGREG 10
#define DISKCMDREG 14
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
int furthestpixel;
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
FILE* imemin, * dmemin, * diskin, * irq2in; //input files
FILE* dmemout, * regout, * trace, * hwregtrace, * cycles, * leds, * display7seg, * diskout, * monitortxt, * monitoryuv; //output files

int opcode, rd, rs, rt, rm, imm1, imm2; //the 7 arguments per instruction

//--------------------------------------------------- Necessary Headers ---------------------------------//
void read_from_disk(void);
void write2disk(void);
void get_next_irq2(void);
void save_in_monitor(void);

// -------------------------------------------------- HELPERS ------------------------------------------//
void hex2bin(char* hexstr, char* binstr) {
	int index = 0;
	char* temp = "";
	while (hexstr[index] != '\0') {
		if (hexstr[index] == '0') {
			temp = "0000";
		}
		else if (hexstr[index] == '1') {
			temp = "0001";
		}
		else if (hexstr[index] == '2') {
			temp = "0010";
		}
		else if (hexstr[index] == '3') {
			temp = "0011";
		}
		else if (hexstr[index] == '4') {
			temp = "0100";
		}
		else if (hexstr[index] == '5') {
			temp = "0101";
		}
		else if (hexstr[index] == '6') {
			temp = "0110";
		}
		else if (hexstr[index] == '7') {
			temp = "0111";
		}
		else if (hexstr[index] == '8') {
			temp = "1000";
		}
		else if (hexstr[index] == '9') {
			temp = "1001";
		}
		else if (hexstr[index] == 'a' || hexstr[index] == 'A') {
			temp = "1010";
		}
		else if (hexstr[index] == 'b' || hexstr[index] == 'B') {
			temp = "1011";
		}
		else if (hexstr[index] == 'c' || hexstr[index] == 'C') {
			temp = "1100";
		}
		else if (hexstr[index] == 'd' || hexstr[index] == 'D') {
			temp = "1101";
		}
		else if (hexstr[index] == 'e' || hexstr[index] == 'E') {
			temp = "1110";
		}
		else if (hexstr[index] == 'f' || hexstr[index] == 'F') {
			temp = "1111";
		}
		else {
			printf("INVALID HEX STRING\n");
		}
		strcat(binstr, temp);
		index++;
	}
}

int get_bin_value(char* strval) {
	char temp[33] = "";
	hex2bin(strval, temp);
	temp[32] = '\0';
	int out = strtol(temp + 1, NULL, 2);
	if (temp[0] == '1') {// if MSB is 1
		out -= (1LL << 31);
	}
	return out;
}

void sign_extend_immediate(int immediate, char* immstr) {
	int err;
	bool ShouldExtend = (immediate >= 2048); //2^11 = 2048, if condition is true it means MSB is 1 
	if (ShouldExtend) { //we checked if MSB is 1
		err = sprintf(immstr, "fffff%3x", immediate);
	}
	else {
		err = sprintf(immstr, "00000%03x", immediate);
	}
	if (err < 0) {}//handle error?
}

char* get_IO_reg_name(int regnum) {
	switch (regnum) {
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

bool init_files(char* argv[]) { //open all FILE structs. return false if failed
	//input files
	imemin = fopen(argv[1], "r");
	if (imemin == NULL) {
		printf("ERROR OPENING IMEMIN\n");
		return false;
	}
	dmemin = fopen(argv[2], "r");
	if (dmemin == NULL) {
		printf("ERROR OPENING DMEMIN\n");
		return false;
	}
	diskin = fopen(argv[3], "r");
	if (diskin == NULL) {
		printf("ERROR OPENING DISKIN\n");
		return false;
	}
	irq2in = fopen(argv[4], "r");
	if (irq2in == NULL) {
		printf("ERROR OPENING IRQ2IN\n");
		return false;
	}

	//output files
	dmemout = fopen(argv[5], "w");
	if (dmemout == NULL) {
		printf("ERROR OPENING DMEMOUT\n");
		return false;
	}
	regout = fopen(argv[6], "w");
	if (regout == NULL) {
		printf("ERROR OPENING REGOUT\n");
		return false;
	}
	trace = fopen(argv[7], "w");
	if (trace == NULL) {
		printf("ERROR OPENING TRACE\n");
		return false;
	}
	hwregtrace = fopen(argv[8], "w");
	if (hwregtrace == NULL) {
		printf("ERROR OPENING HWREGTRACE\n");
		return false;
	}
	cycles = fopen(argv[9], "w");
	if (cycles == NULL) {
		printf("ERROR OPENING CYCLES\n");
		return false;
	}
	leds = fopen(argv[10], "w");
	if (leds == NULL) {
		printf("ERROR OPENING LEDS\n");
		return false;
	}
	display7seg = fopen(argv[11], "w");
	if (display7seg == NULL) {
		printf("ERROR OPENING DISPLAY7SEG\n");
		return false;
	}
	diskout = fopen(argv[12], "w");
	if (diskout == NULL) {
		printf("ERROR OPENING DISKOUT\n");
		return false;
	}
	monitortxt = fopen(argv[13], "w");
	if (monitortxt == NULL) {
		printf("ERROR OPENING MONITORTXT\n");
		return false;
	}
	monitoryuv = fopen(argv[14], "wb");
	if (monitoryuv == NULL) {
		printf("ERROR OPENING MONITORYUV\n");
		return false;
	}

	return true;
}

bool init_code() {
	char* buffer = (char*)malloc((INST_WDT + 1) * sizeof(char)); //line size is 14: 12 for inst, 1 for '\n' and 1 for '\0'
	int row = 0;
	while (fgets(buffer, (INST_WDT + 1) * sizeof(char), imemin) != NULL) {
		asmcode[row] = (char*)malloc(INST_WDT * sizeof(char));
		asmcode[row][12] = '\0';
		strncpy(asmcode[row], buffer, INST_WDT - 1);//copies line without '\n'
		row++;
	}
	for (row; row < MAX_NUM_OF_LINES; row++) {
		asmcode[row] = (char*)malloc(12 * sizeof(char));
		strcpy(asmcode[row], "");
	}
	free(buffer);
	return true;
}

bool init_mem() {
	char* buffer = (char*)malloc((MEM_WDT + 1) * sizeof(char));//line size is 10: 8 for word, 1 for '\n' and 1 for '\0'
	int row = 0;
	while (fgets(buffer, (MEM_WDT + 1) * sizeof(char), dmemin) != NULL) {
		mem[row] = (char*)malloc(MEM_WDT * sizeof(char));
		strncpy(mem[row], buffer, 8);
		mem[row][8] = '\0';
		row++;
	}
	furthestaddresswritten = row;
	for (row; row < MAX_NUM_OF_LINES; row++) {
		mem[row] = (char*)malloc(MEM_WDT * sizeof(char));
		strcpy(mem[row], "00000000");
		mem[row][8] = '\0';
	}
	free(buffer);
	return true;
}

bool init_disk() {
	char* buffer = (char*)malloc((MEM_WDT + 1) * sizeof(char));//line size is 10: 8 for word, 1 for '\n' and 1 for '\0'
	int sector = 0;
	int row = 0;
	while (fgets(buffer, (MEM_WDT + 1) * sizeof(char), diskin) != NULL) {
		disk[sector][row] = (char*)malloc(MEM_WDT * sizeof(char));
		strncpy(disk[sector][row], buffer, 8);
		disk[sector][row][8] = '\0';
		row++;
		if (row == SECTOR_SIZE) {
			row = 0;
			sector++;
		}
	}
	furthestsectorwritten = sector;
	furthestinsector = row;
	for (sector; sector < NUM_OF_SECTORS; sector++) {
		for (row; row < SECTOR_SIZE; row++) {
			disk[sector][row] = (char*)malloc(MEM_WDT * sizeof(char));
			strncpy(disk[sector][row], "00000000", 8);
			disk[sector][row][8] = '\0';
		}
		row = 0;
	}
	free(buffer);
	return true;
}

bool init_misc() {
	int i;
	for (i = 0; i < NUM_OF_REGS; i++) {
		regs[i] = (char*)malloc(MEM_WDT * sizeof(char));
		strcpy(regs[i], "00000000");
		regs[i][8] = '\0';
	}

	for (i = 0; i < NUM_OF_IO_REGS; i++) {
		IO[i] = (char*)malloc(MEM_WDT * sizeof(char));
		strcpy(IO[i], "00000000");
		IO[i][8] = '\0';
	}

	for (i = 0; i < MONITOR_BUFFER_LEN; i++) {
		monitor[i] = (char*)malloc(3 * sizeof(char));// 1B + '\0'
		strcpy(monitor[i], "00");
		monitor[i][2] = '\0';
	}
	get_next_irq2();
	PC = 0;
	cyclecount = 0;
	irq = false;
	in_ISR = false;
	furthestpixel = 0;
	running = true;
	return true;
}

bool init_values() { //init values of regs, IO, and copy imemin and dmemin into asmcode and mem. return false if failed
	if (!init_code()) {
		return false;
	}
	if (!init_mem()) {
		return false;
	}
	if (!init_disk()) {
		return false;
	}
	if (!init_misc()) {
		return false;
	}
	return true;
}

//------------------------------------------------OUTPUT-------------------------------------------------//

void write2hwtrace(char* rw, int IOnum) {
	char* IOreg = get_IO_reg_name(IOnum);
	fprintf(hwregtrace, "%d %s %s %s\n", cyclecount, rw, IOreg, IO[IOnum]);
	fflush(hwregtrace);
}

void write2hw7seg() {
	fprintf(display7seg, "%d %s\n", cyclecount, IO[DISPLAY7SEGREG]);
	fflush(display7seg);
}

void write2hwleds() {
	fprintf(leds, "%d %s\n", cyclecount, IO[LEDSREG]);
	fflush(leds);
}

void output2trace() {
	fprintf(trace, "%03X %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", PC, asmcode[PC], regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
	fflush(trace);
}

void write2dmemout() {
	int row = 0;
	for (row; row < furthestaddresswritten; row++) {
		fprintf(dmemout, "%s\n", mem[row]);
	}
	fflush(dmemout);
}

void write2regout() {
	int regnum = 3;
	for (regnum; regnum < NUM_OF_REGS; regnum++) {
		fprintf(regout, "%s\n", regs[regnum]);
		fflush(regout);
	}
}

void write2cycles() {
	fprintf(cycles, "%d\n", cyclecount);
	fflush(cycles);
}

void write2diskout() {
	int sector = 0;
	int row = 0;
	if (furthestsectorwritten == 0) { return; }
	for (sector; sector < furthestsectorwritten; sector++) {
		for (row = 0; row < SECTOR_SIZE; row++) {
			fprintf(diskout, "%s\n", disk[sector][row]);
		}
	}
	if (furthestinsector != 0) {
		for (row = 0; row < furthestinsector; row++) {
			fprintf(diskout, "%s\n", disk[sector][row]);
		}
	}
	fflush(diskout);
}

void write2monitor() {
	for (int i = 0; i < furthestpixel; i++) {
		fprintf(monitortxt, "%s\n", monitor[i]);
		fflush(monitortxt);
		unsigned char pxl = (unsigned char)strtol(monitor[i], NULL, 16);
		fwrite(&pxl, 1, 1, monitoryuv);
		fflush(monitoryuv);
	}
}

void write_to_output() {
	//write at end: *dmemout, *regout,   *cycles,   *diskout, *monitortxt, *monitoryuv;
	write2dmemout();
	fflush(dmemout);

	write2regout();
	fflush(regout);

	write2cycles();
	fflush(cycles);

	write2diskout();
	fflush(diskout);

	write2monitor();
}

//-------------------------------------------------------- CPU RUN -------------------------------------------------//

void decode_instruction(char* instruction) {
	//opcode: 2 digits, rd: 1 digit, rs: 1 digit, rt: 1 digit, rm: 1 digit, imm1: 3 digits, imm2: 3 digits

	char* temp1 = (char*)malloc(2 * sizeof(char));
	char* temp2 = (char*)malloc(3 * sizeof(char));
	char* temp3 = (char*)malloc(4 * sizeof(char));

	temp1[1] = '\0';
	temp2[2] = '\0';
	temp3[3] = '\0';

	temp2[0] = instruction[0];
	temp2[1] = instruction[1];
	opcode = strtol(temp2, NULL, 16);

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
	imm1 = strtol(temp3, NULL, 16);

	temp3[0] = instruction[9];
	temp3[1] = instruction[10];
	temp3[2] = instruction[11];
	imm2 = strtol(temp3, NULL, 16);

	free(temp1);
	free(temp2);
	free(temp3);
	return;
}

void prepare_instruction() {
	sign_extend_immediate(imm1, regs[1]);
	sign_extend_immediate(imm2, regs[2]);
}

void handle_input() {
	int regnum = get_bin_value(regs[rt]) + get_bin_value(regs[rs]);
	if (regnum > 22 || regnum < 0) { return; }//out of bounds
	strncpy(regs[rd], IO[regnum], 8);
	if (regnum == MONITORCMDREG) {
		strncpy(regs[rd], "00000000", 8);
	}
	write2hwtrace("READ", regnum);
}

void handle_output() {
	int regnum = (get_bin_value(regs[rt]) + get_bin_value(regs[rs]));
	if (regnum > 22 || regnum < 0) { return; }//out of bounds
	bool cantwrite = (regnum == 17 || regnum == 18 || regnum == 19);//rsvd or diskstatus
	if (cantwrite) { return; }
	strncpy(IO[regnum], regs[rm], 8);
	write2hwtrace("WRITE", regnum);
	switch (regnum) { //if we need something special from specific IO reg
	case LEDSREG:
		write2hwleds();
		break;
	case DISPLAY7SEGREG:
		write2hw7seg();
		break;
	case DISKCMDREG:
		if (strncmp(IO[regnum], "00000001", 8) == 0) {
			strncpy(IO[17], "00000001", 8); // disk is now busy
			read_from_disk();
		}
		else if (strncmp(IO[regnum], "00000002", 8) == 0) {
			strncpy(IO[17], "00000001", 8);// disk is now busy
			write2disk();
		}
		break;
	case MONITORCMDREG:
		if (strncmp(IO[regnum], "00000001", 8) == 0) {
			save_in_monitor();
		}
		break;
	default:
		break;
	}
}

void run_instruction() {
	int val;
	bool branch;
	bool rdiswritable = !(rd == 0 || rd == 1 || rd == 2);//false when trying to write to $zero or $imm1,2
	int err;
	int rsval = get_bin_value(regs[rs]);
	int rtval = get_bin_value(regs[rt]);
	int rmval = get_bin_value(regs[rm]);
	switch (opcode) {
	case Add:
		val = rsval + rtval + rmval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		};
		PC++;
		break;
	case Sub:
		val = rsval - rtval - rmval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Mac:
		val = rsval * rtval + rmval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case And:
		val = rsval & rtval & rmval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Or:
		val = rsval | rtval | rmval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Xor:
		val = rsval ^ rtval ^ rmval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Sll:
		val = rsval << rtval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Sra: //arithmetic shift with sign extension
		val = rsval >> rtval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Srl: // logic shift vacant bits filled up with zeros
		val = ((unsigned int)rsval) >> rtval;
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Beq:
		branch = (rsval == rtval);
		PC = branch ? rmval : PC + 1;
		break;
	case Bne:
		branch = (rsval != rtval);
		PC = branch ? rmval : PC + 1;
		break;
	case Blt:
		branch = (rsval < rtval);
		PC = branch ? rmval : PC + 1;
		break;
	case Bgt:
		branch = (rsval > rtval);
		PC = branch ? rmval : PC + 1;
		break;
	case Ble:
		branch = (rsval <= rtval);
		PC = branch ? rmval : PC + 1;
		break;
	case Bge:
		branch = (rsval >= rtval);
		PC = branch ? rmval : PC + 1;
		break;
	case Jal:
		if (rdiswritable) {
			err = sprintf(regs[rd], "%08x", PC + 1);
			if (err < 0) {}//handle error?   
		}
		PC = rmval;
		break;
	case Lw:
		if (rdiswritable) {
			strcpy(regs[rd], mem[(rsval + rtval) % MAX_NUM_OF_LINES]);
			val = get_bin_value(regs[rd]) + rmval;
			err = sprintf(regs[rd], "%08x", val);
			if (err < 0) {}//handle error?
		}
		PC++;
		break;
	case Sw:
		val = get_bin_value(regs[rd]) + rmval;
		err = sprintf(mem[(rsval + rtval) % MAX_NUM_OF_LINES], "%08X", val);
		if (err < 0) {}//handle error?
		if (rsval + rtval + 1 > furthestaddresswritten) {
			furthestaddresswritten = rsval + rtval + 1;
		}
		PC++;
		break;
	case Reti:
		PC = get_bin_value(IO[7]);
		in_ISR = false;
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
		printf("PASSED INVALID OPCODE ON CYCLE %d. opcode is %d\n", cyclecount, opcode);
		return;//invalid opcode. error
	}
}

//---------------------------------------------- PERIPHERALS --------------------------------------//
void write2disk() {
	int sector;
	int MEMaddress;
	int i;
	sector = get_bin_value(IO[15]); // sector that we write to in decimal
	MEMaddress = get_bin_value(IO[16]); // address of the buffer in the main memory that we read from in decimal
	for (i = 0; i < SECTOR_SIZE; i++) {
		strncpy(disk[sector][i], mem[MEMaddress], 8);
		MEMaddress++;
	}
	if ((sector + 1) > furthestsectorwritten) {
		furthestsectorwritten = sector + 1;
		furthestinsector = 0;
	}
	strncpy(IO[14], "00000000", 8);
}

void read_from_disk() {
	int sector;
	int MEMaddress;
	int i;
	sector = get_bin_value(IO[15]); // sector that we read from in decimal
	MEMaddress = get_bin_value(IO[16]); // address of the buffer in the main memory that we write to in decimal
	for (i = 0; i < SECTOR_SIZE; i++) {
		strcpy(mem[MEMaddress], disk[sector][i]);
		MEMaddress++;
	}
	strncpy(IO[14], "00000000", 8);
}

void update_disk_timer() {
	if (strncmp(IO[14], "00000000", 8) == 0 && strncmp(IO[17], "00000001", 8) == 0) { // if we executed the read/write command we wait 1024 cycles while the disk is busy
		disk_timer++;
	}
}

void update_irq() { // check the irq status and update. only turns them on, the ISR needs to turn them off.
	if (timer == 0 && strcmp(IO[11], "00000001") == 0) {// irq0
		strncpy(IO[3], "00000001", 8);
	}
	if (disk_timer == 1024 && strcmp(IO[17], "00000001") == 0) {// irq1
		strncpy(IO[4], "00000001", 8);
		disk_timer = 0;
		strncpy(IO[17], "00000000", 8);
	}
	if (havenextirq2 && cyclecount == nextirq2) {// irq2
		strncpy(IO[5], "00000001", 8);
		get_next_irq2();
	}
	irq = ((strncmp(IO[0], "00000001", 8) == 0 && strncmp(IO[3], "00000001", 8) == 0) || (strncmp(IO[1], "00000001", 8) == 0 && strncmp(IO[4], "00000001", 8) == 0) || (strncmp(IO[2], "00000001", 8) == 0 && strncmp(IO[5], "00000001", 8) == 0));
}

void handle_irq() { // go to ISR and handle 
	sprintf(IO[7], "%08x", PC);
	PC = get_bin_value(IO[6]);
	in_ISR = true;
}

void update_timer() {
	if (strncmp(IO[11], "00000001", 8) == 0) {
		if (timer == max_timer) {
			timer = 0;
			strncpy(IO[12], "00000000", 8);
		}
		else {
			timer++;
			sprintf(IO[12], "%08x", timer); // update timecurrent
		}
	}
	else {
		timer = 0;
		strncpy(IO[12], "00000000", 8);
	}
}

void get_next_irq2() {
	char buffer[MAX_LINE_LEN] = "";
	char* err = fgets(buffer, sizeof(buffer), irq2in);
	if (err == NULL) {
		havenextirq2 = false;
		return;
	}
	havenextirq2 = true;
	nextirq2 = strtol(buffer, NULL, 0);
}

void save_in_monitor() {
	int monitoraddr = get_bin_value(IO[20]);
	if (monitoraddr + 1 > furthestpixel) {
		furthestpixel = monitoraddr + 1;
	}
	monitor[monitoraddr][0] = IO[21][6];
	monitor[monitoraddr][1] = IO[21][7];//take the last 2 hexa digits in IO reg
}

void handle_parallel_systems() {
	cyclecount++;
	update_timer();
	update_disk_timer();
	update_irq();
}

//-------------------------------- MAIN LOOP -----------------------------------//

void run_program() { //main func that runs while we didn't get HALT instruction
	while (running) {
		//pre-instruction
		if (PC > MAX_NUM_OF_LINES) { break; } // or error?
		char codedinst[INST_WDT];
		codedinst[12] = '\0';
		strncpy(codedinst, asmcode[PC], 12);
		if (codedinst == NULL) { break; } // or error?
		decode_instruction(codedinst);
		prepare_instruction();

		//output
		output2trace();

		//instruction
		run_instruction();

		//post instruction
		if (irq && !in_ISR) { handle_irq(); }
		handle_parallel_systems();
	}
}

//------------------------------------FINALIZE-------------------------------------------//
void close_files() {
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

void free_code() {
	int row = 0;
	for (row; row < MAX_NUM_OF_LINES; row++) {
		if (asmcode[row] != NULL) {
			free(asmcode[row]);
		}
	}
}

void free_mem() {
	int row = 0;
	for (row; row < MAX_NUM_OF_LINES; row++) {
		if (mem[row] != NULL) {
			free(mem[row]);
		}
	}
}

void free_disk() {
	int sector, row;
	for (sector = 0; sector < 128; sector++) {
		for (row = 0; row < SECTOR_SIZE; row++) {
			if (disk[sector][row] != NULL) {
				free(disk[sector][row]);
			}
			else {
				break;
			}
		}
	}
}

void free_regs() {
	int row = 0;
	for (row; row < NUM_OF_REGS; row++) {
		if (regs[row] != NULL) {
			free(regs[row]);
		}
	}
}

void free_IO() {
	int row = 0;
	for (row; row < NUM_OF_IO_REGS; row++) {
		if (IO[row] != NULL) {
			free(IO[row]);
		}
	}
}

void free_monitor() {
	int row = 0;
	for (row; row < MONITOR_BUFFER_LEN; row++) {
		if (monitor[row] != NULL) {
			free(monitor[row]);
		}
	}
}

void free_all() {
	free_code();
	free_mem();
	free_disk();
	free_regs();
	free_IO();
	free_monitor();
}

void finalize() { //release everything for program end and print last things for output files if needed
	write_to_output();

	free_all();

	close_files();
}

//----------------------------------------- MAIN ----------------------------------------//

int main(int argc, char* argv[]) {
	if (argc != 15) {
		return EXIT_FAILURE;
	}

	if (!init_files(argv)) {
		//error
		return EXIT_FAILURE;
	}

	if (!init_values()) {
		//error
		return EXIT_FAILURE;
	}

	run_program();

	finalize();

	return EXIT_SUCCESS;
};