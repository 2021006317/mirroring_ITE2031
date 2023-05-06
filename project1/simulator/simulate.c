/* LC-2K Instruction-level simulator */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory ... 16bit */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *);
int convertNum(int num);

void parse(stateType *, int *, int *, int *, int *);
void testReg(int *);
void testOffset(int *);

int main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /* read in the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
            state.numMemory++) {

        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

	/* TODO: */
    int halted=0;
    while(1){
        printState(&state);
        if (halted==1) break;
        int opcode, arg0, arg1, arg2;
        parse(&state, &opcode, &arg0, &arg1, &arg2);
        state.pc++;
        /* opcode : add(0), nor(1), lw(2), sw(3), beq(4), jalr(5), halt(6), noop(7) */
        /* R-type */
        if (opcode==0 || opcode==1){
            testReg(arg0);
            testReg(arg1);
            testReg(arg2);
            // add
            if (opcode==0){
                state.reg[arg2] = state.reg[arg0] + state.reg[arg1];
            }
            // nor : or 연산에 not
            else if (opcode==1){
                state.reg[arg2] = ~(state.reg[arg0] || state.reg[arg1]);
            }
        }
        /* I-type */
        else if (opcode==2 || opcode==3 || opcode==4){
            testReg(arg0);
            testReg(arg1);
            testOffset(arg2);
            /* lw
                lw regA(arg0) regB(arg1) offset(arg2)
                Load regB from memory(offset+A).
                Memory address is formed by adding offsetField with the contents of regA.
            */
            if (opcode==2){
                state.reg[arg1] = state.mem[state.mem[arg0] + convertNum(arg2)];
            }
            /* sw
                sw regA(arg0) regB(arg1) offset(arg2)
                Store regB into memory(offset+A).
                Memory address is formed by adding offsetField with the contents of regA.
            */
            else if (opcode==3){
                state.mem[state.reg[arg0]+ convertNum(arg2)] = state.reg[arg1];
            }
            /* beq
                beq regA regB offset
                if the contents of regA and regB are the same,
                then branch to the address PC+1+offsetField,
                where PC is the address of this beq instruction.
            */
            else if (opcode==4){
                if (state.reg[arg0]==state.reg[arg1]){
                    state.pc += convertNum(arg2);
                }
            }
        }
        /* J-type */
        // First, store PC+1 into regB, where PC is the address of this "jalr" instruction.
        // Then branch to the address contained in regA.
        // Note that this implies if regA and regB refer to the same register, the net effect will be jumping to Pc+1.
        else if (opcode==5){
            testReg(arg0);
            testReg(arg1);
            state.reg[arg1] = state.pc;
            state.pc = state.reg[arg0];
        }
        /* O-type */
        else if (opcode==6 || opcode==7){
            if (opcode==6){ // halt
                halted=1;
            }
            else if (opcode==7){ // noop
                continue;
            }
        }
        else{
            printf("error: Unrecognized opcodes\n");
            exit(1);
        }
    }

    fclose(filePtr);
    exit(0);
}

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i = 0; i < statePtr->numMemory; i++) {
        printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i = 0; i < NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
	/* convert a 16-bit number into a 32-bit Linux integer */
	if (num & (1 << 15)) {
		num -= (1 << 16);
	}
	return (num);
}

void parse(stateType *statePtr, int *opcode, int *arg0, int *arg1, int *arg2){
    int currentMemory = statePtr->mem[statePtr->pc];
    /*
        opcode : bit 24-22
        fld0 : bit 21-19
        fld1 : bit 18-16
        fld2 : bit 15-0
    */
    *opcode = (currentMemory >> 22) & 0b111;
    *arg0 = (currentMemory >> 19) & 0b111;
    *arg1 = (currentMemory >> 16) & 0b111;
    *arg2 = currentMemory & 0xffff;
}

void testReg(int *regNum){
	if (regNum < 0 || regNum > 7){
		printf("error: Register outside the range [0,7], input regNum is %d\n", regNum);
		exit(1);
	}
}

void testOffset(int *offset){
    if (offset > 32767 || offset < -32768){
        printf("error: offsetFields that don't fit in 16 bits\n");
        exit(1);
    }
}