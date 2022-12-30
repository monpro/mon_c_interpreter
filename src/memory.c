#include <stdlib.h>

#include "memory.h"
#include "vm.h"
#include "object.h"

void freeObject(Obj *pObj);

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

void freeObject(Obj *obj) {
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
    }
}

void freeObjects() {
    Obj* object = vm.objects;

    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

