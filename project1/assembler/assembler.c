/* Assembler code fragment for LC-2K */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINELENGTH 1000

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int treatOffsetField(char *);
int treatFill(char *);

void testReg(char *);

typedef struct LABEL {
	char name;
	int address;
	char value; // .fill
} LABEL;

LABEL labels[MAXLINELENGTH]={0,};
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

	/* TODO: Phase-1 label calculation */
	while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)){
		//* duplicated label test
		for (int k=0; k<cnt;k++){
			if (labels[k].name == label){
				printf("error: Duplicate definition of labels\n");
				exit(1);
			}
		}
		// labeling
		labels[cnt].name = label;
		labels[cnt].address = cnt;

		if (opcode == ".fill"){
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
					printf("error: Use of undefined labels\n");
					exit(1);
				}
			}
		}
	}
	
	/* this is how to rewind the file ptr so that you start reading from the
		 beginning of the file */
	rewind(inFilePtr);

	/* TODO: Phase-2 generate machine codes to outfile */
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
		if (opcode == "add" || opcode == "nor"){
			testReg(arg0);
			testReg(arg1);
			testReg(arg2);
			// opcode
			if (opcode == "add") 		{ res = 0 << 22; }
			else if (opcode == "nor")	{ res = 1 << 22; }
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
		else if (opcode == "lw" || opcode == "sw" || opcode == "beq"){
			testReg(arg0);
			testReg(arg1);
			testReg(arg2);
			// opcode
			if (opcode == "lw") 		{ res = 2 << 22; }
			else if (opcode == "nor")	{ res = 3 << 22; }
			else if (opcode == "beq")	{ res = 4 << 22; }
			res |= atoi(arg0) << 19; // regA
			res |= atoi(arg1) << 16; // regB
			res |= treatOffsetField(arg2); // offsetField
		}
		/* J-type
			bit 31-25 : unused(0)
			bit 24-22 : opcode
			bit 21-19 : regA
			bit 18-16 : regB
			bit 15-0  : unused(0)
		*/
		else if (opcode == "jalr"){
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
		else if (opcode == "halt" || opcode == "noop"){
			if (opcode == "halt") 		{ res = 6 << 22; }
			else if (opcode == "noop")	{ res = 7 << 22; }
		}
		/* .fill
			number : address
			label : label address
		*/
		else if (opcode == ".fill"){
			testReg(arg0);
			res = treatFill(arg0);
		}
		else { //* invalid instruction test
			printf("error: Unrecognized opcodes\n");
			exit(1);
		}
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
	if (isNumber(arg0)){
		res = atoi(arg0);
	}
	else{
		int check=0;
		for (int i=0;i<cnt;i++){
			if (labels[i].name == arg0){
				res = labels[i].address;
				check=1;
				break;
			}
		}
		if (check==0) {
			printf("error: Use of undefined labels\n");
			exit(1);
		}
	}
	return res;
}

// 16bit 니까 int.
int treatOffsetField(char *offset){
	int res=0;
	if (isNumber(offset)){
		long test = 0;
		test = atol(offset);
		if (test > 32767 || test < -32768){
			printf("error: offsetFields that don't fit in 16 bits\n");
			exit(1);
		}
		res = atoi(offset);
	}
	else { //label
		int check=0;
		for (int i=0; i<MAXLINELENGTH; i++){
			if (labels[i].name == offset){
				res = labels[i].value;
				check=1;
				break;
			}
		}
		if (check == 0){
			printf("error: Use of undefined labels\n");
			exit(1);
		}
	}
	return res;
}

void testReg(char *reg){
	if(!isNumber(reg)){
		printf("error: Non-integer register arguments\n");
		exit(1);
	}
	int regNum = atoi(reg);
	if (regNum < 0 || regNum > 7){
		printf("error: Register outside the range [0,7], input regNum is %d\n", regNum);
		exit(1);
	}
}