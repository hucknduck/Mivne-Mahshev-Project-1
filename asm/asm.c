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

FILE * program;
FILE *imemin;
FILE *dmemin; /*asm.exe program.asm imemin.txt dmemin.txt*/
struct labels {
	char* label_name;
	int label_address;
	struct labels* next;
};

struct labels* label;

int* dmem;

bool is_empty_line_or_comment(char* line){
	if (*line == '\n')
		return true;
	char* comment_ptr = strstr(line, "#");
	if (comment_ptr == NULL)
		return false; // not a comment
	while (line < comment_ptr){
		if (*line == ' ' ||
			*line == '\t'){
				line++;
			}
		else 
			return false; 
	}
	return true;
}

bool is_line_dot_word(char* line){
	char* word_ptr = strstr(line, ".word");
	char* comment_ptr;
	if (word_ptr == NULL) {
		return false; // no .word
	}
	comment_ptr = strstr(line, "#");
	if (comment_ptr == NULL) {
		return true; // yes .word and no comment
	}
	return comment_ptr - word_ptr > 0; // the comment appears after the ".word" --> true
}

bool is_line_a_label(char* line) { // examples: L4:, Olnmfda:, L1:
    char* end;
	if (line == NULL || *line == '\0') {
		return false;
	}
	
	end = line;
	while (*end != '\0') {
		if (*end == '#') 
		    return false; // If The semicolon appears after a comment it doesn't count
        if (*end == ':') 
		    return true; // If not, return false
		end++;
	}
	return false;
}


int save_label(char* name, int address) {
	char* nam = name;
	int len = 0;
	while(*nam != '\0' && *nam != ':'){
		if (*nam != ' ' && *nam != '\t' && *nam != '\n' && *nam != ':')
			len++;
		nam++;
	}
	//printf("len: %d\n",len);

	if (label == NULL) {
		label = (struct labels*)malloc(sizeof(struct labels));
		if (label == NULL)
			return EXIT_FAILURE;
		label->label_address = address;
		label->label_name = (char*)malloc(sizeof(char) * len);
		if (label->label_name == NULL){return -1;}
		int i = 0;
		while (i < len) {
			if (*name != ' ' && *name != '\t' && *name != '\n' && *name != ':') {
				label->label_name[i] = *name;
				i++;
			}
			name++;
		}
		label->label_name[len] = '\0';
		label->next = NULL;
       //printf("saved label: %s\n",label->label_name);
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
			tmp->next->label_name = (char*)malloc(sizeof(char) * len);
			if (tmp->next->label_name == NULL){return -1;}
			int i = 0;
			while (i < len) {
				if (*name != ' ' && *name != '\t' && *name != '\n' && *name != ':') {
						tmp->next->label_name[i] = *name;
						i++;
					}
				name++;
			}
			tmp->next->label_name[len] = '\0';
			tmp->next->next = NULL;
           // printf("saved label: %s\n",tmp->label_name);
			return EXIT_SUCCESS;
		}
		tmp = tmp->next;
	}
	return EXIT_FAILURE; //should never reach
}

bool my_str_cmpr(char* str1, char* str2){
	while(str1 != NULL && str2 != NULL && *str1 != '\0' && *str2 != '\0'){
		if (*str1 != *str2)
			return false;
		str1++;
		str2++;
	}
	return true;
}

struct labels* get_label(char* label_name){
	struct labels* tmp = label;
	while (tmp != NULL) {
		if ( tmp->label_name != NULL && my_str_cmpr(tmp->label_name,label_name))
			return tmp;
		//printf("%s is not %s\n",tmp->label_name,label_name);
		tmp = tmp->next;
	}

	return NULL;
}

void check_dot_word(char* line){
	char* helper = (char *)malloc(MAX_LINE_LEN * sizeof(char));
	char* to_free = helper;
	if (helper == NULL) { perror("MALLOC ERROR"); return;}
	helper = strtok(line, "\t. ");
	
	int index = 0;
	int val = 0;
	if (strcmp(helper,"word") != 0){ //helper isn't word;
		return;
	}
	helper = strtok(NULL, " ");
	index = strtol(helper, NULL , 10);
	
	helper = strtok(NULL, " ");
	val = strtol(helper, NULL, 10);
	
	dmem[index] = val;
	free(to_free);
}

void first_pass() {
	// This function remmebers the lables and their addresses, also remmembers the .words
	char buffer[MAX_LINE_LEN];
	int row = 0;

	//for every line of code inputed:
	while(fgets(buffer, sizeof(buffer), program) != NULL) {
        
		if (is_line_a_label(buffer)) {
			//save the label name to row number.
            //printf("buff: %s\n",buffer);
			save_label(buffer, row); //The current row and not row + 1
            
		}
		else {
            //printf("row: %d\n",row);
			if (!is_line_dot_word(buffer) && !is_empty_line_or_comment(buffer)){
				row++; //instruction
			}
			check_dot_word(buffer);
			//todo : maybe check if the line is actually an instruction
		}
	}
}
char* handle_opcode(char** str){
	if (strcmp(*str, "add") == 0){
		return ADD;
	}
	else if (strcmp(*str, "sub") == 0){
		return SUB;
	}
	else if (strcmp(*str, "mac") == 0){
		return MAC;
	}
	else if (strcmp(*str, "and") == 0){
		return AND;
	}
	else if (strcmp(*str, "or") == 0){
		return OR;
	}
	else if (strcmp(*str, "xor") == 0){
		return XOR;
	}
	else if (strcmp(*str, "sll") == 0){
		return SLL;
	}
	else if (strcmp(*str, "sra") == 0){
		return SRA;
	}
	else if (strcmp(*str, "srl") == 0){
		return SRL;
	}
	else if (strcmp(*str, "beq") == 0){
		return BEQ;
	}
	else if (strcmp(*str, "bne") == 0){
		return BNE;
	}
	else if (strcmp(*str, "blt") == 0){
		return BLT;
	}
	else if (strcmp(*str, "bgt") == 0){
		return BGT;
	}
	else if (strcmp(*str, "ble") == 0){
		return BLE;
	}
	else if (strcmp(*str, "bge") == 0){
		return BGE;
	}
	else if (strcmp(*str, "jal") == 0){
		return JAL;
	}
	else if (strcmp(*str, "lw") == 0){
		return LW;
	}
	else if (strcmp(*str, "sw") == 0){
		return SW;
	}
	else if (strcmp(*str, "reti") == 0){
		return RETI;
	}
	else if (strcmp(*str, "in") == 0){
		return IN;
	}
	else if (strcmp(*str, "out") == 0){
		return OUT;
	}
	else if (strcmp(*str, "halt") == 0){
		return HALT;
	}
	else {//error
		return NULL;
	}
}

char* handle_register(char** str){
	if (strcmp(*str, "$zero") == 0){
		return $ZERO;
	}
	else if (strcmp(*str, "$imm1") == 0){
		return $IMM1;
	}
	else if (strcmp(*str, "$imm2") == 0){
		return $IMM2;
	}
	else if (strcmp(*str, "$v0") == 0){
		return $V0;
	}
	else if (strcmp(*str, "$a0") == 0){
		return $A0;
	}
	else if (strcmp(*str, "$a1") == 0){
		return $A1;
	}
	else if (strcmp(*str, "$a2") == 0){
		return $A2;
	}
	else if (strcmp(*str, "$t0") == 0){
		return $T0;
	}
	else if (strcmp(*str, "$t1") == 0){
		return $T1;
	}
	else if (strcmp(*str, "$t2") == 0){
		return $T2;
	}
	else if (strcmp(*str, "$s0") == 0){
		return $S0;
	}
	else if (strcmp(*str, "$s1") == 0){
		return $S1;
	}
	else if (strcmp(*str, "$s2") == 0){
		return $S1;
	}
	else if (strcmp(*str, "$gp") == 0){
		return $GP;
	}
	else if (strcmp(*str, "$sp") == 0){
		return $SP;
	}
	else if (strcmp(*str, "$ra") == 0){
		return $RA;
	}
	else{//error
		return NULL;
	}
}

void handle_immediate(char* str, char* hex){
	int base_ten = 10;
	struct labels* lbl = get_label(str);
	if (lbl != NULL){ // If it's a label
		//printf("lbl: %s\n", lbl->label_name);
		snprintf(hex, 12,"%3X", lbl->label_address);
		for (int i = 0; i < 3; i++){
			if (hex[i] == ' ')
				hex[i] = '0';
		}
		return;
	}
	//else, return value of immediate in hex, 12B length
	if (*str == '0')
		base_ten = 0; //just tells the strtol that the string is written with 0x before the number
	int strvalue = strtol(str, NULL, base_ten); // convert to decimal
	snprintf(hex, 12,"%3X", strvalue); //THIS FUNC CONTAINS IMPLICIT MALLOC, MEM IS FREED IN translate_instruction()
	for (int i = 0; i < 3; i++){
		if (hex[i] == ' ')
			hex[i] = '0';
	}
	return;
}

void translate_instruction(char* line, char* instruction) {
	int wordnum = 0;
	//char instruction[48] = {0};
	char* pch = "";
	char immediate[8] = "";
	for (wordnum = 0; wordnum < NUM_OF_PARAMS; wordnum++){// can use for loop since non-label lines always have 6 params
		if (wordnum == 0){
			line = line+1; // avoid the tab
			pch = strtok(line, "\t, $");
		}else{
			pch = strtok(NULL, ", #");
			// printf("%d strtok pch:%s\n",wordnum,pch);
		}
		if (wordnum == 0){//first param, is opcode
			strcat(instruction, handle_opcode(&pch));
			// printf("%s @@@\n",instruction);
		}
	 	else if (1 <= wordnum && wordnum <= 4){//1-4 param, register
			strcat(instruction, handle_register(&pch));
			// printf("%s @@@\n",instruction);
		}
		else{//5-6 param, immediates
			handle_immediate(pch, immediate);
			// printf("%s ###\n",immediate);
			int last_three_bytes = strlen(immediate) - 3;

			strcat(instruction, &(immediate[last_three_bytes]));
			// printf("%s ###\n",instruction);
		}
	}
}

void writetoimemin(char* nextline, int row){
	fseek ( imemin , 0, SEEK_CUR);
	fputs( nextline, imemin ); 
	fputs( "\n", imemin );
}


void second_pass() {
	/* the second pass already has the linked list of all the lables,
	now we can create the two output files.*/
	char buffer[MAX_LINE_LEN];
	int row = 0;
	//for every line of code inputed:
	while (fgets(buffer, sizeof(buffer), program) != NULL) {
		if (!is_line_a_label(buffer) && !is_line_dot_word(buffer) && !is_empty_line_or_comment(buffer)) {
			//translate instruction to hex code:
			//printf("translating row %d\n",row);
			char* instruction = (char*) calloc(sizeof(char),48); // todo add check
			translate_instruction(buffer, instruction);
			printf("%s\n",instruction);
			writetoimemin(instruction, row);
			free(instruction);
			row++;
		}
	}
}

int find_biggest_index(){
	for (int i = 4095; i >= 0; i--){
		if (dmem[i] != 0)
			{return i;}
	}
	return 0;
}

void create_dmem(){
	char buff[9];
	int biggest_num = find_biggest_index();
	printf("%s","dmem\n");
	for (int i = 0; i <= biggest_num; i++){
		snprintf(buff, 1+sizeof(buff),"%8X", dmem[i]); //THIS FUNC CONTAINS IMPLICIT MALLOC, MEM IS FREED IN translate_instruction()
		for (int j = 0; j < 8; j++){
			if (buff[j] == ' ')
				buff[j] = '0';
		}
		printf("%s\n",buff);
		fseek ( dmemin , 0, SEEK_CUR);
		fputs( buff,  dmemin); 
		fputs( "\n", dmemin );
	}
}

void free_labels(){
	struct labels* tmp = label;
	struct labels* tmp_next = label;
	while(tmp != NULL){
		tmp_next = tmp->next;
		free(tmp->label_name);
		free(tmp);
		tmp = tmp_next;
	}
}

int main(int argc, char* argv[]) {
	/*command-line parsing */
	/*asm.exe program.asm imemin.txt dmemin.txt*/
	if (argc != 4) {
		printf("Wrong number of args");
		return EXIT_FAILURE;
	}
	program = fopen(argv[1], "r");
	if (program == NULL) {
		printf("Couldn't open the .asm");
		return EXIT_FAILURE;
	}
	/*Creating the output files*/
	imemin = fopen(argv[2], "w"); // imemin.txt, if exists it erases it
	if (imemin == NULL) {
		printf("Couldn't open imemin");
		return EXIT_FAILURE;
	}
	dmemin = fopen(argv[3], "w"); // dmemin.txt, if exists it erases it
	if (dmemin == NULL) {
		printf("Couldn't open dmemin");
		return EXIT_FAILURE;
	}
	/*end of command-line parsing */

	dmem = (int *)calloc(4096,sizeof(int)); // remember .word for dmemin
	/* first pass- remmbering lables */
	first_pass();
	//printf("First pass is done.\n");
	program = fopen(argv[1], "r"); // read the file again.
	if (program == NULL) {
		return EXIT_FAILURE;
	}

	second_pass();
	
	create_dmem();
	//todo free the linked list
	fclose(program);
	fclose(imemin);
	fclose(dmemin);

	free_labels();
	printf("Done\n");
	return 0;
}