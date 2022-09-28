#ifndef MON_C_INTERPRETER_OBJECT_H
#define MON_C_INTERPRETER_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj {
    ObjType type;
};

#endif //MON_C_INTERPRETER_OBJECT_H
