#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_NUM_OF_LINES 4096

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

FILE * program, *imein, *dmemin; /*asm.exe program.asm imemin.txt dmemin.txt*/
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
			save_label(buffer, row + 1);
            
		}
		else {
            printf("row: %d\n",row);
			row++; //instruction
			//todo : maybe check if the line is actually an instruction
		}
	}
}

void translate_instruction(char* line) {
	uint64_t instruction = 0;
	char* pch = strtok(line, " ,$");
	while (pch != NULL)
	{
		//printf("%s\n", pch);
		pch = strtok(NULL, " ,$");
	}
}

void second_pass() {
	/* the second pass already has the linked list of all the lables,
	now we can create the two output files.*/
	char buffer[MAX_LINE_LEN];
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), program) != NULL) {
		if (!is_line_a_label(buffer)) {
			row++;
			//translate instruction to hex code:
			translate_instruction(buffer);
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
	imein = fopen(argv[2], "w"); // imemin.txt, if exists it erases it
	if (imein == NULL) {
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