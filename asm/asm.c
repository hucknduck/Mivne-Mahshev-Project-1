#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_NUM_OF_LINES 4096
#define ADD 0x000000
#define SUB 0x010000
#define MAC 0x020000
#define AND 0x030000
#define OR 0x040000
#define XOR 0x050000
#define SLL 0x060000
#define SRA 0x070000
#define SRL 0x080000
#define BEQ 0x090000
#define BNE 0x0A0000
#define BLT 0x0B0000
#define BGT 0x0C0000
#define BLE 0x0D0000
#define BGE 0x0E0000
#define JAL 0x0F0000
#define LW 0x100000
#define SW 0x110000
#define RETI 0x120000
#define IN 0x130000
#define OUT 0x140000
#define HALT 0x150000
FILE * program, *imein, *dmemin; /*asm.exe program.asm imemin.txt dmemin.txt*/
struct labels {
	char* label_name;
	int label_address;
	struct labels* next;
};

struct labels* label;

bool is_line_a_label(char* line) { // examples: L4:, Olnmfda:, L1:
	if (line == NULL || *line == '\0') {
		return false;
	}
	// Check if the first char is not a letter:
	if (!(('a' <= *line && *line <= 'z') ||
		('A' <= *line && *line <= 'Z'))) {
		return false;
	}
	// Find the end of the string
	char* end = line;
	while (*end != '\0') {
		end++;
	}
	end--; // Move to the last character before the null terminator

	// Check if the last character is a colon
	if (*end != ':') {
		return false; // If not, return false
	}

	return true;
}

int save_label(char* name, int address) {
	if (label == NULL) {
		label = (struct labels*)malloc(sizeof(struct labels));
		if (label == NULL)
			return EXIT_FAILURE;
		label->label_address = address;
		label->label_name = name;
		label->next = NULL;
        printf("saved label: %s, ",name);
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
            printf("saved label: %s, ",name);
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
			save_label(buffer, row + 1);
            printf("buff: %s\n",buffer);
		}
		else {
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