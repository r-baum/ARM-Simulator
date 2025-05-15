#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "machine.h"
#include "code.h"

struct machine_t machine;

/*
 * Allocate more space to keep track of values on the simulated stack.
 */
void grow_stack(uint64_t new_sp) {
    // Grow the stack upwards
    if (new_sp < machine.stack_top) {
        // Round down to a multiple of word size
        if (new_sp % WORD_SIZE_BYTES != 0) {
            new_sp -= new_sp % WORD_SIZE_BYTES;
        }

        // Allocate space and copy over old values 
        void *new_stack = malloc(machine.stack_bot - new_sp + 1);
        memset(new_stack, 0, machine.stack_top - new_sp);
        if (machine.stack != NULL) {
            memcpy(new_stack + (machine.stack_top - new_sp), machine.stack, machine.stack_bot - machine.stack_top + 1);
            free(machine.stack);
        }

        // Update machine
        machine.stack = new_stack;
        machine.stack_top = new_sp;
    }
    // Grow the stack downwards
    else if (new_sp > machine.stack_bot) {
        // Round up to a multiple of word size
        if (new_sp % WORD_SIZE_BYTES != 0) {
            new_sp += WORD_SIZE_BYTES - (new_sp % WORD_SIZE_BYTES);
        }
        else {
            new_sp += WORD_SIZE_BYTES;
        }

        // Allocate space and copy over old values 
        void *new_stack = malloc(new_sp - machine.stack_top);
        memset(new_stack + (machine.stack_bot - machine.stack_top), 0, new_sp - machine.stack_bot);
        if (machine.stack != NULL) {
            memcpy(new_stack, machine.stack, machine.stack_bot - machine.stack_top);
            free(machine.stack);
        }

        // Update machine
        machine.stack = new_stack;
        machine.stack_bot = new_sp - 1;
    }
}

/*
 * Initialize the machine
 */
void init_machine(uint64_t sp, uint64_t pc, char *code_filepath) {
    // Populate general purpose registers
    for (int i = 0; i <= 30; i++) {
        machine.registers[i] = REGISTER_NULL;
    }
    
    // Populate special purpose registers
    machine.sp = sp;
    machine.pc = pc;
    
    // Load code
    machine.code = parse_file(code_filepath, &(machine.code_top), &(machine.code_bot));

    // Prepare stack
    machine.stack_top = sp;
    machine.stack_bot = sp + WORD_SIZE_BYTES - 1;
    machine.stack = malloc(WORD_SIZE_BYTES);
    memset(machine.stack, 0, WORD_SIZE_BYTES);

    // Clear all condition codes
    machine.conditions = 0;
}

void print_memory() {
    // Print condition codes
    printf("Condition codes:");
    if (machine.conditions & CONDITION_ZERO) {
        printf(" Z");
    }
    if (machine.conditions & CONDITION_NEGATIVE) {
        printf(" N");
    }
    if (machine.conditions & CONDITION_POSITIVE) {
        printf(" P");
    }
    printf("\n");

    // Print the value of all used registers
    printf("Registers:\n");
    for (int i = 0; i <= 30; i++) {
        if (machine.registers[i] != REGISTER_NULL) {
            printf("\tw/x%d = 0x%lx\n", i, machine.registers[i]);
        }
    }
    printf("\tsp = 0x%lX\n", machine.sp);
    printf("\tpc = 0x%lX\n", machine.pc);

    // If necessary, grow the stack before printing it
    if (machine.sp < machine.stack_top || machine.sp > machine.stack_bot) {
        grow_stack(machine.sp);
    }

    // Print the value of all words on the stack
    printf("Stack:\n");
    unsigned char *stack = machine.stack;
    for (int i = 0; i < (machine.stack_bot - machine.stack_top); i += 8) {
        printf("\t");

        if (machine.sp == i + machine.stack_top) {
            printf("%10s ", "sp->");
        }
        else {
            printf("           ");
        }

        printf("+-------------------------+\n");
        printf("\t0x%08lX | ", i + machine.stack_top);
        for (int j = 0; j < 8; j++) {
            printf("%02X ", stack[i+j]);
        }
        printf("|\n");
    }
    printf("\t           +-------------------------+\n");
}

/*
 * Get the next instruction to execute
 */
struct instruction_t fetch() {
    int index = (machine.pc - machine.code_top) / 4;
    return machine.code[index];
}

/*
 * Get the value associated with a constant or register operand.
 */
uint64_t get_value(struct operand_t operand) {
    assert(operand.type == OPERAND_constant || operand.type == OPERAND_address || operand.type == OPERAND_register);
    // TODO
    switch (operand.type){
        case OPERAND_constant:
            return operand.constant;
        case OPERAND_address:
            return operand.constant;
        case OPERAND_register:
            switch (operand.reg_type){
                case REGISTER_x:
                    return machine.registers[operand.reg_num];
                case REGISTER_w:
                    return (uint32_t)machine.registers[operand.reg_num]; 
                case REGISTER_sp:
                    return machine.sp;
                case REGISTER_pc:
                    return machine.pc;
            }
    }
    return 0;
}

/*
 * Put a value in a register specified by an operand.
 */
void put_value(struct operand_t operand, uint64_t value) {
    assert(operand.type == OPERAND_register);
    switch (operand.reg_type) {
        case REGISTER_x:
            machine.registers[operand.reg_num] = value;  
            break;
        case REGISTER_w:
            machine.registers[operand.reg_num] = (uint32_t)value; 
            break;
        case REGISTER_sp:
            machine.sp = value; 
            break;
        case REGISTER_pc:
            machine.pc = value;  
            break;
    }
}

/*
 * Get the memory address associated with a memory operand.
 */
uint64_t get_memory_address(struct operand_t operand) {
    assert(operand.type == OPERAND_memory);
    uint64_t base = 0;

    switch (operand.reg_type) {
        case REGISTER_x:
            base = machine.registers[operand.reg_num];
            break;
        case REGISTER_sp:
            base = machine.sp;
            break;
        case REGISTER_pc:
            base = machine.pc;
            break;
    }
    return base + operand.constant;
}

//executes fundamental math operations
void execute_arithmetic(struct instruction_t instruction) {
    uint64_t op1 = get_value(instruction.operands[1]);
    uint64_t op2 = get_value(instruction.operands[2]);
    uint64_t result;
    switch(instruction.operation) {
    case OPERATION_add:
        result = op1 + op2;
        break;
    case OPERATION_sub:
    case OPERATION_subs:
        result = op1 - op2;
        break;
    case OPERATION_mul:
        result = op1 * op2;
        break;
    case OPERATION_sdiv:
    case OPERATION_udiv:
        result = op1 / op2;
        break;
    }

    put_value(instruction.operands[0], result);
}
//executes bitwise math operators using basic bitwise logic on operands
void execute_bitwise(struct instruction_t instruction) {
    uint64_t op1 = get_value(instruction.operands[1]);
    uint64_t op2 = get_value(instruction.operands[2]);
    uint64_t result;
    switch(instruction.operation) {
        case OPERATION_lsl:
        result = op1 << op2;
        break;
    case OPERATION_lsr:
        result = op1 >> op2;
        break;
    case OPERATION_and:
        result = op1 & op2;
        break;
    case OPERATION_orr:
        result = op1 | op2;
        break;
    case OPERATION_eor:
        result = op1 ^ op2;
        break;
    }
    put_value(instruction.operands[0], result);
}

//executes the mov instruction by storing the desired value into the register
void execute_mov(struct instruction_t instruction) {
    put_value(instruction.operands[0],get_value(instruction.operands[1]));
}

//executes the load instructions finds the real address and adds the appropriate offset and then loads the desired value in the appropriate register type
//ChatGPT was used to help with stack adress implementation and casting
//ChatGPT. OpenAI GPT-4. OpenAI, 17 Apr. 2025.

/*
We are supposed to load the value at the second operand's address into the first operand's address.

We get the simulated address from the second operand using get_memory_address, and we use that to calculate
the real stack address. Since machine.stack_top is the sim address of first byte of data, simaddress - machine.stack_top
is the space between the byte we are looking for and the first byte ot data, and we add this to the real address of 
the first byte of data, or machine.stack, to get the real address of the second operand.

Now that we have the real address, we cast it to get the value and then we put that value into the first operand. 
We assume we read/write it as 64-bits unless the register is w, when we read/write it as 32-bits.
*/

void execute_ldr(struct instruction_t instruction) {
    switch(instruction.operation){
        case OPERATION_ldr: 
            uint64_t simaddress = get_memory_address(instruction.operands[1]);
            uint64_t realaddress = (uint64_t)machine.stack + simaddress - machine.stack_top; 
            switch (instruction.operands[0].reg_type) {
                case REGISTER_sp:
                case REGISTER_pc:
                case REGISTER_x: {
                    uint64_t value = *(uint64_t *)realaddress; 
                    put_value(instruction.operands[0], value);
                    break;
                }
                case REGISTER_w: {
                    uint32_t value = *(uint32_t *)realaddress;
                    put_value(instruction.operands[0], value);
                    break;
                }
            
        }
        break;
    }
}

//executes the instructions of str by finding the real address, adding the appropriate offset, and then storing thee value based on the appropriate register
//ChatGPT was used to help with stack address implementation
//ChatGPT. OpenAI GPT-4. OpenAI, 17 Apr. 2025.

/*
We are supposed to store the value of the first operand into the second operand's address. We use get_value to get the
value of the first operand. To find the real address of the second operand which we store the value at, we 
get the simulated address using get_memory_address and calculate the real address, exactly as we did it for ldr. 

We then change the value of the stack pointer at that real address, and if it's register w we read/write it as 32-bits.
*/

void execute_str(struct instruction_t instruction) {
    switch(instruction.operation){
    case OPERATION_str: 
        uint64_t value = get_value(instruction.operands[0]);  
        uint64_t simaddress = get_memory_address(instruction.operands[1]);
        uint64_t realaddress = (uint64_t)machine.stack + simaddress - machine.stack_top;
        switch (instruction.operands[0].reg_type) {
            case REGISTER_w:
                *(uint32_t *)realaddress = (uint32_t)value; 
                break;
            case REGISTER_x:
                *(uint64_t *)realaddress = value;
                break;
    }
    break;
    }
}

//executes the cmp instruction by checking the values of operands and setting appropriate flags
void execute_cmp(struct instruction_t instruction){
    switch(instruction.operation){
        case OPERATION_cmp:
            uint64_t op1 = get_value(instruction.operands[0]);
            uint64_t op2 = get_value(instruction.operands[1]);
            if(op1 > op2){
                machine.conditions = CONDITION_POSITIVE;
            }
            else if(op1 == op2){
                machine.conditions = CONDITION_ZERO;
            }
            else{
                machine.conditions = CONDITION_NEGATIVE;
            }
            break; 
    }
}

//executes branching instruction by setting program counter equal to the value of current operand
void execute_b(struct instruction_t instruction){
    uint64_t next = get_value(instruction.operands[0]);
    machine.pc = next;
}

//executes the return instruction by setting program counter equal to return register value
void execute_ret(struct instruction_t instruction){
    machine.pc = machine.registers[30];
}

//executes branch linking instruction by storing next instruction in link register and then branching
void execute_bl(struct instruction_t instruction){
    machine.registers[30] = machine.pc + 4;
    execute_b(instruction);

}

//executes branching instructions by checking the flags set by cmp and branching accordingly
void execute_branch_equality(struct instruction_t instruction){
    switch(instruction.operation){
        case OPERATION_bne:
            if(machine.conditions != CONDITION_ZERO){
                // branch
                execute_b(instruction);
            }
            break;
        case OPERATION_beq:
            if(machine.conditions == CONDITION_ZERO){
                execute_b(instruction);
            }
            break;
        case OPERATION_blt:
            if(machine.conditions == CONDITION_NEGATIVE){
                execute_b(instruction);
            }
            break;
        case OPERATION_bgt:
            if(machine.conditions == CONDITION_POSITIVE){\
                execute_b(instruction);
            }
            break;
        case OPERATION_bge:
            if(machine.conditions == CONDITION_POSITIVE || machine.conditions == CONDITION_ZERO){
                execute_b(instruction);
            }
            break;
        case OPERATION_ble:
            if(machine.conditions == CONDITION_NEGATIVE || machine.conditions == CONDITION_ZERO){
                execute_b(instruction);
            }
            break;
    }
}

//function for executing clz instruction based on the type of register, 
//adds appropriate amount of zeroes to the end of the value in the base register,
//based one whether it is an x register or w register
//we then iterate through the value depending on the amount of bits (32 or 64)
//then we store the counter variable into the desired register
//ChatGPT used for number iteration feature 
//"Explanation of iteration system in CLZ (Count Leading Zeros) Operation in C." 
// ChatGPT. OpenAI GPT-4. OpenAI, 17 Apr. 2025.
void execute_clz(struct instruction_t instruction){
    uint64_t ret = 0;
    switch (instruction.operands[1].reg_type) {
        case REGISTER_x: {
            uint64_t value = get_value(instruction.operands[1]);
            for(int i = 63; i >= 0; i--){ 
                if((value >> i ) & 1){
                    break;
                }
                ret++;
            }
            break;
        }
        case REGISTER_w: {
            uint32_t value = get_value(instruction.operands[1]);
            for(int i = 31; i >= 0; i--){ 
                if((value >> i ) & 1){
                    break;
                }
                ret++;
            }
            break;
        }
    }
    put_value(instruction.operands[0],ret);
}

//executes ldrb instruction by performing a normal load but with only one byte from the original address using type casting
//Casting implementation inspired by ChatGPT
//ChatGPT. OpenAI GPT-4. OpenAI, 17 Apr. 2025.
void execute_ldrb(struct instruction_t instruction){
    switch (instruction.operation){
        case OPERATION_ldrb:
            uint64_t simaddress = get_memory_address(instruction.operands[1]);
            uint64_t realaddress = (uint64_t)machine.stack + simaddress - machine.stack_top;
            uint64_t byteaddr = *(uint8_t *)realaddress;
            put_value(instruction.operands[0],byteaddr);
            break;
    }
}
//executes strb instruction by carrying out a normal store but with modifying amount of bytes with type casting
//Casting implementation inspired by ChatGPT
//ChatGPT. OpenAI GPT-4. OpenAI, 17 Apr. 2025.
void execute_strb(struct instruction_t instruction ){
    switch (instruction.operation){
        case OPERATION_strb:
            uint64_t value = get_value(instruction.operands[0]);  
            uint64_t sim_address = get_memory_address(instruction.operands[1]);
            uint64_t realaddress = (uint64_t)machine.stack + sim_address - machine.stack_top;
            *(uint8_t *)realaddress = (uint8_t)value;
            break;
    }
}

/*
 * Execute an instruction
 */
 
void execute(struct instruction_t instruction) {
    switch(instruction.operation) {
    case OPERATION_add:
    case OPERATION_sub:
    case OPERATION_subs:
    case OPERATION_mul:
    case OPERATION_sdiv:
    case OPERATION_udiv:
        execute_arithmetic(instruction);
        break;
    case OPERATION_lsl:
    case OPERATION_lsr:
    case OPERATION_and:
    case OPERATION_orr:
    case OPERATION_eor:
        execute_bitwise(instruction);
        break;
    case OPERATION_mov:
        execute_mov(instruction);
        break;
    case OPERATION_ldr:
        execute_ldr(instruction);
        break;
    case OPERATION_str:
        execute_str(instruction);
        break;
    case OPERATION_cmp:
        execute_cmp(instruction);
        break;
    case OPERATION_beq:
    case OPERATION_bne:
    case OPERATION_blt:
    case OPERATION_bgt:
    case OPERATION_ble:
    case OPERATION_bge:
        execute_branch_equality(instruction);
        break;
    case OPERATION_b:
        execute_b(instruction);
        break;
    case OPERATION_bl:
        execute_bl(instruction);
        break;
    case OPERATION_ret:
        execute_ret(instruction);
        break;
    case OPERATION_nop:
        //do nothing
        break;
    case OPERATION_clz:
        execute_clz(instruction);
        break;
    case OPERATION_strb:
        execute_strb(instruction);
        break;
    case OPERATION_ldrb:
        execute_ldrb(instruction);
        break;
    default:
        printf("!!Instruction not implemented!!\n");
    }
}
