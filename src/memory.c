#include <stdlib.h>

#include "memory.h"
#include "vm.h"
#include "object.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

void freeObject(Obj *pObj);

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
    }
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

void freeObject(Obj *obj) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString *string = (ObjString *) obj;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, obj);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) obj;
            freeChunk(&function->chunk);
            FREE(ObjFunction, obj);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, obj);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)obj;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, obj);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(ObjUpvalue, obj);
            break;
        }
    }
}

void markObject(Obj* object) {
    if (object == NULL) return;
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    object->isMarked = true;
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markRoots() {
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }
    markTable(&vm.globals);
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif
    markRoots();
#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

void freeObjects() {
    Obj* object = vm.objects;

    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

