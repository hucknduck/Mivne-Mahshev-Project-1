#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_NUM_OF_LINES 4096
#define NUM_OF_PARAMS 7

#define ADD "00"
#define SUB "01"
#define MAC "02"
#define AND "03"
#define OR "04"
#define XOR "05"
#define SLL "06"
#define SRA "07"
#define SRL "08"
#define BEQ "09"
#define BNE "0A"
#define BLT "0B"
#define BGT "0C"
#define BLE "0D"
#define BGE "0E"
#define JAL "0F"
#define LW "10"
#define SW "11"
#define RETI "12"
#define IN "13"
#define OUT "14"
#define HALT "15"

#define $ZERO "0"
#define $IMM1 "1"
#define $IMM2 "2"
#define $V0 "3"
#define $A0 "4"
#define $A1 "5"
#define $A2 "6"
#define $T0 "7"
#define $T1 "8"
#define $T2 "9"
#define $S0 "A"
#define $S1 "B"
#define $S2 "C"
#define $GP "D"
#define $SP "E"
#define $RA "F"

FILE * program, *imemin, *dmemin; /*asm.exe program.asm imemin.txt dmemin.txt*/
struct labels {
	char* label_name;
	int label_address;
	struct labels* next;
};

struct labels* label;

bool is_line_a_label(char* line) { // examples: L4:, Olnmfda:, L1:
    char* end;
	if (line == NULL || *line == '\0') {
		return false;
	}
	// Check if the first char is not a letter:

	// if (!(('a' <= *line && *line <= 'z') ||
	// 	('A' <= *line && *line <= 'Z'))) {
    //     printf("%s is not a letter",*line);
	// 	return false;
	// }
	// Find the end of the string
	end = line;
	while (*end != '\0') {
        if (*end == ':') 
		    return true; // If not, return false
		end++;
	}
	

	return false;
}

int save_label(char* name, int address) {
	if (label == NULL) {
		label = (struct labels*)malloc(sizeof(struct labels));
		if (label == NULL)
			return EXIT_FAILURE;
		label->label_address = address;
		label->label_name = name;
		label->next = NULL;
        printf("saved label: %s",name);
		return EXIT_SUCCESS;
	}

	struct labels* tmp = label;
	while (tmp != NULL) {
		if (tmp->label_address == address)
			return EXIT_SUCCESS; //if already exists --> exit
		if (tmp->next == NULL) { //if we reach the end --> create a new label
			tmp->next = (struct labels*)malloc(sizeof(struct labels));
			if (tmp->next == NULL)
				return EXIT_FAILURE;
			tmp->next->label_address = address;
			tmp->next->label_name = name;
			tmp->next->next = NULL;
            printf("saved label: %s",name);
			return EXIT_SUCCESS;
		}
		tmp = tmp->next;
	}
	return EXIT_FAILURE; //should never reach
}

void first_pass() {
	// This function remmebers the lables and their addresses
	char buffer[MAX_LINE_LEN];
	int row = 0;

	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), program) != NULL) {
        
		if (is_line_a_label(buffer)) {
			//save the label name to row number.
            printf("buff: %s\n",buffer);
			save_label(buffer, row + 1);//I think maybe save it with row, and not row+1? and not inc row. since we do not want to count these rows in second pass either. todo: DAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAN
            
		}
		else {
            printf("row: %d\n",row);
			row++; //instruction
			//todo : maybe check if the line is actually an instruction
		}
	}
}
char* handle_opcode(char* str){
	if (strcomp(str, "add") == 0){
		return ADD;
	}
	else if (strcomp(str, "sub") == 0){
		return SUB;
	}
	else if (strcomp(str, "mac") == 0){
		return MAC;
	}
	else if (strcomp(str, "and") == 0){
		return AND;
	}
	else if (strcomp(str, "or") == 0){
		return OR;
	}
	else if (strcomp(str, "xor") == 0){
		return XOR;
	}
	else if (strcomp(str, "sll") == 0){
		return SLL;
	}
	else if (strcomp(str, "sra") == 0){
		return SRA;
	}
	else if (strcomp(str, "srl") == 0){
		return SRL;
	}
	else if (strcomp(str, "beq") == 0){
		return BEQ;
	}
	else if (strcomp(str, "bne") == 0){
		return BNE;
	}
	else if (strcomp(str, "blt") == 0){
		return BLT;
	}
	else if (strcomp(str, "bgt") == 0){
		return BGT;
	}
	else if (strcomp(str, "ble") == 0){
		return BLE;
	}
	else if (strcomp(str, "bge") == 0){
		return BGE;
	}
	else if (strcomp(str, "jal") == 0){
		return JAL;
	}
	else if (strcomp(str, "lw") == 0){
		return LW;
	}
	else if (strcomp(str, "sw") == 0){
		return SW;
	}
	else if (strcomp(str, "reti") == 0){
		return RETI;
	}
	else if (strcomp(str, "in") == 0){
		return IN;
	}
	else if (strcomp(str, "out") == 0){
		return OUT;
	}
	else if (strcomp(str, "halt") == 0){
		return HALT;
	}
	else {//error
		return NULL;
	}
}

char* handle_register(char* str){
	if (strcomp(str, "$zero") == 0){
		return $ZERO;
	}
	else if (strcomp(str, "$imm1") == 0){
		return $IMM1;
	}
	else if (strcomp(str, "$imm2") == 0){
		return $IMM2;
	}
	else if (strcomp(str, "$v0") == 0){
		return $V0;
	}
	else if (strcomp(str, "$a0") == 0){
		return $A0;
	}
	else if (strcomp(str, "$a1") == 0){
		return $A1;
	}
	else if (strcomp(str, "$a2") == 0){
		return $A2;
	}
	else if (strcomp(str, "$t0") == 0){
		return $T0;
	}
	else if (strcomp(str, "$t1") == 0){
		return $T1;
	}
	else if (strcomp(str, "$t2") == 0){
		return $T2;
	}
	else if (strcomp(str, "$s0") == 0){
		return $S0;
	}
	else if (strcomp(str, "$s1") == 0){
		return $S1;
	}
	else if (strcomp(str, "$s2") == 0){
		return $S1;
	}
	else if (strcomp(str, "$gp") == 0){
		return $GP;
	}
	else if (strcomp(str, "$sp") == 0){
		return $SP;
	}
	else if (strcomp(str, "$ra") == 0){
		return $RA;
	}
	else{//error
		return NULL;
	}
}

void handle_immediate(char* str, char* outpointer){
	if (immediate_is_label(str)){//if is label, return value of label
		return get_label_value(str);
	} //else, return value of immediate in hex, 12B length
	int strvalue = strtol(str, NULL, 10);
	int formattedStrResult = asprintf(&outpointer, "%012X", strvalue); //THIS FUNC CONTAINS IMPLICIT MALLOC, MEM IS FREED IN translate_instruction()
	printf("outpointer is %s", outpointer);//DEBUG
	//todo: check if this prints correctly with 0x or without (we want it without)
	if(formattedStrResult > 0){	
    	return;
	} else {
    	// todo some error?
	}
}

char* translate_instruction(char* line) {
	int wordnum = 0;
	char* instruction = "";
	char* pch;
	char* currword = "";
	char* temptofree;
	for (wordnum = 0; wordnum < NUM_OF_PARAMS; wordnum++){// can use for loop since non-label lines always have 6 params
		pch = strtok(line, " ,$"); //need to change this since it is incorrect, maybe only go by " ,"? todo: DAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAN
		if (wordnum == 0){//first param, is opcode
			instruction = strcat(instruction, handle_opcode(pch));
		}
	 	else if (1 <= wordnum || wordnum <= 4){//1-4 param, register
			instruction = strcat(instruction, handle_register(pch));
		}
		else{//5-6 param, immediates
			handle_immediate(pch, temptofree);
			instruction = strcat(instruction, temptofree);
			free(temptofree);//mem allocated in handle_immediate
		}
	}
	return instruction;
}

void second_pass() {
	/* the second pass already has the linked list of all the lables,
	now we can create the two output files.*/
	char buffer[MAX_LINE_LEN];
	char* nextline;
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), program) != NULL) {
		if (!is_line_a_label(buffer)) {
			//translate instruction to hex code:
			nextline = translate_instruction(buffer);
			writetoimemin(nextline, row);
			row++;
		}
	}
}

int main(int argc, char* argv[]) {
	/*command-line parsing */
	/*asm.exe program.asm imemin.txt dmemin.txt*/
	if (argc != 4) {
		return EXIT_FAILURE;
	}
	program = fopen(argv[1], "r");
	if (program == NULL) {
		return EXIT_FAILURE;
	}
	/*Creating the output files*/
	imemin = fopen(argv[2], "w"); // imemin.txt, if exists it erases it
	if (imemin == NULL) {
		return EXIT_FAILURE;
	}
	dmemin = fopen(argv[3], "w"); // dmemin.txt, if exists it erases it
	if (dmemin == NULL) {
		return EXIT_FAILURE;
	}
	/*end of command-line parsing */

	/* first pass- remmbering lables */
	first_pass();

	program = fopen(argv[1], "r"); // read the file again.
	if (program == NULL) {
		return EXIT_FAILURE;
	}

	second_pass();



	return 0;
}