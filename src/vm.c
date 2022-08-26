#include "common.h"
#include "vm.h"


// Single Global VM Object
VM vm;

InterpretResult run();

void initVM() {

}

void freeVM() {

}

InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
    for (;;) {
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }
#undef READ_BYTE
}


InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

