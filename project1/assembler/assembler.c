/* Assembler code fragment for LC-2K */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINELENGTH 1000

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int treatOffsetField(char *, char *, int pc);
int treatFill(char *);

void testReg(char *);

typedef struct labelStruct {
	char name[7];
	int address;
	int value; // .fill
} LABEL;

LABEL labels[MAXLINELENGTH];

int cnt=0; // line cnt or label cnt

int main(int argc, char *argv[]) 
{
	char *inFileString, *outFileString;
	FILE *inFilePtr, *outFilePtr;

	char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH], 
			 arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
	
	if (argc != 3) {
		printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
				argv[0]);
		exit(1);
	}
	inFileString = argv[1];
	outFileString = argv[2];

	inFilePtr = fopen(inFileString, "r");
	if (inFilePtr == NULL) {
		printf("error in opening %s\n", inFileString);
		exit(1);
	}
	outFilePtr = fopen(outFileString, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileString);
		exit(1);
	}

	//* TODO: Phase-1 label calculation */
	while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)){
		//* duplicated label test
		for (int k=1; k<cnt; k++){
			if (strcmp(labels[k].name, "") && !strcmp(labels[k].name, label)){
				printf("personal_error: Duplicate definition of labels\n");
				exit(1);
			}
		}
		// labeling
		strcpy(labels[cnt].name, label);
		labels[cnt].address = cnt;

		//* label validation test
		if ((strlen(label) > 6)||isNumber(&label[0])){
			printf("personal_error-raising label: %s\n", label);
			if (strlen(label) > 6){
				printf("personal_error: too long label\n");
			}
			if (isNumber(&label[0])){
				printf("personal_error: label cannot start with number\n");
			}
			exit(1);
		}
		
		if (!strcmp(opcode, ".fill")){
			// address
			if (isNumber(arg0)){
				labels[cnt].value = atoi(arg0);
			}
			// label
			else {
				int check=0;
				for (int j=0; j<MAXLINELENGTH; j++){
					if (!strcmp(arg0, labels[j].name)) {
						labels[cnt].value = labels[j].address;
						check=1;
						break;
					}
				}
				//* undefined label test
				if (check==0) {
					printf("personal_error: Use of undefined labels\n");
					exit(1);
				}
			}
		}
		cnt++;
	}

	/* this is how to rewind the file ptr so that you start reading from the
		 beginning of the file */
	rewind(inFilePtr);

	//* TODO: Phase-2 generate machine codes to outfile */
	int pc = 0;
	while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)){
		int res = 0;
		
		/* R-type
			bit 31-25 : unused(0)
			bit 24-22 : opcode
			bit 21-19 : regA
			bit 18-16 : regB
			bit 15-3  : unused(0)
			bit 2-0   : destReg
		*/
		if (!strcmp(opcode, "add") || !strcmp(opcode, "nor")){
			testReg(arg0);
			testReg(arg1);
			testReg(arg2);
			// opcode
			if (!strcmp(opcode, "add")) 		{ res = 0 << 22; }
			else if (!strcmp(opcode, "nor"))	{ res = 1 << 22; }
			res |= atoi(arg0) << 19; // regA
			res |= atoi(arg1) << 16; // regB
			res |= atoi(arg2); 		 // destReg
		}
		/* I-type
			bit 31-25 : unused(0)
			bit 24-22 : opcode
			bit 21-19 : regA
			bit 18-16 : regB
			bit 15-0  : offsetField
		*/
		else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw") || !strcmp(opcode, "beq")){
			testReg(arg0);
			testReg(arg1);
			// opcode
			if (!strcmp(opcode, "lw")) 		{ res = 2 << 22; }
			else if (!strcmp(opcode, "sw"))	{ res = 3 << 22; }
			else if (!strcmp(opcode, "beq"))	{ res = 4 << 22; }
			res |= atoi(arg0) << 19; // regA
			res |= atoi(arg1) << 16; // regB
			res |= treatOffsetField(opcode, arg2, pc); // offsetField
		}
		/* J-type
			bit 31-25 : unused(0)
			bit 24-22 : opcode
			bit 21-19 : regA
			bit 18-16 : regB
			bit 15-0  : unused(0)
		*/
		else if (!strcmp(opcode, "jalr")){
			res = 5 << 22 ;
			testReg(arg0);
			testReg(arg1);
			res |= atoi(arg0) << 19; // regA
			res |= atoi(arg1) << 16; // regB
		}
		/* O-type
			bit 31-25 : unused(0)
			bit 24-22 : opcode
			bit 21-0  : unused(0)
		*/
		else if (!strcmp(opcode, "halt") || !strcmp(opcode, "noop")){
			if (!strcmp(opcode, "halt")) 		{ res = 6 << 22; }
			else if (!strcmp(opcode, "noop"))	{ res = 7 << 22; }
		}
		/* .fill
			number : address
			label : label address
		*/
		else if (!strcmp(opcode, ".fill")){
			res = treatFill(arg0);
		}
		else { //* invalid instruction test
			printf("personal_error: Unrecognized opcodes\n");
			exit(1);
		}

		fprintf(outFilePtr, "%d\n", res);
		pc++;
	}

	if (inFilePtr) 	{close(inFilePtr);	}
	if (outFilePtr) {fclose(outFilePtr);}
	return(0);
}

/*
 Read and parse a line of the assembly-language file.  Fields are returned
 in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 allocated to them).
 
 Return values:
    0 if reached end of file
    1 if all went well
 
 exit(1) if line is too long.
*/
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
		char *arg1, char *arg2)
{
	char line[MAXLINELENGTH];
	char *ptr = line;

	/* delete prior values */
	label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

	/* read the line from the assembly-language file */
	if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
		/* reached end of file */
		return(0);
	}

	/* check for line too long (by looking for a \n) */
	if (strchr(line, '\n') == NULL) {
		/* line too long */
		printf("error: line too long\n");
		exit(1);
	}

	/* is there a label? */
	ptr = line;
	if (sscanf(ptr, "%[^\t\n\r ]", label)) {
		/* successfully read label; advance pointer over the label */
		ptr += strlen(label);
	}

	/*
	 Parse the rest of the line.  Would be nice to have real regular
	 expressions, but scanf will suffice.
	*/
	sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%"
			"[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]", opcode, arg0, arg1, arg2);
	return(1);
}

int isNumber(char *string)
{
	/* return 1 if string is a number */
	int i;
	return( (sscanf(string, "%d", &i)) == 1);
}

//* .fill + 숫자 : 그대로, + 라벨 : 라벨의 주소
int treatFill(char *arg0){
	int res=0;
	if (isNumber(arg0)){ // number
		res = atoi(arg0);
	}
	else{ // label
		int check=0;
		for (int i=0; i<cnt; i++){
			if (!strcmp(labels[i].name, arg0)){
				res = labels[i].address;
				check=1;
				break;
			}
		}
		if (check==0) {
			printf("personal_error: Use of undefined labels\n");
			exit(1);
		}
	}
	return res;
}

// 16bit 니까 int.
int treatOffsetField(char *opcode, char *offset, int pc){
	int res = 0;
	if (isNumber(offset)){
		int temp = atoi(offset);
		if (temp > 32767 || temp < -32768){
			printf("personal_error: offsetFields that don't fit in 16 bits\n");
			exit(1);
		}
		temp &= 0xFFFF;
		res = temp;
	}
	else { //label
		int check=0;
		for (int i=0; i<MAXLINELENGTH; i++){
			if (!strcmp(labels[i].name, offset)){
				int temp = labels[i].address;
				if (!strcmp(opcode, "beq")){res = temp - pc - 1;}
				else res = temp;
	  			if (res > 32767 || res < -32768){
					printf("personal_error: offsetFields that don't fit in 16 bits\n");
					exit(1);
				}
				res &= 0xffff;
				check = 1;
				break;
			}
		}
		if (check == 0){
			printf("personal_error: Use of undefined labels\n");
			exit(1);
		}
	}
	return res;
}

void testReg(char *reg){
	if(!isNumber(reg)){
		printf("reg: %s\n", reg);
		printf("personal_error: Non-integer register arguments\n");
		exit(1);
	}
	int regNum = atoi(reg);
	if (regNum < 0 || regNum > 7){
		printf("personal_error: Register outside the range [0,7], input regNum is %d\n", regNum);
		exit(1);
	}
}
