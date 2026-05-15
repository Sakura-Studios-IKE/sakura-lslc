/* types.c - LSL type system helpers */
#include "lsl.h"
#include <string.h>

const char *type_name(TypeKind t) {
    switch (t) {
        case T_VOID:     return "void";
        case T_INTEGER:  return "integer";
        case T_FLOAT:    return "float";
        case T_STRING:   return "string";
        case T_KEY:      return "key";
        case T_VECTOR:   return "vector";
        case T_ROTATION: return "rotation";
        case T_LIST:     return "list";
        case T_ANY:      return "<any>";
        case T_ERROR:    return "<error>";
    }
    return "<?>";
}

TypeKind type_from_keyword(const char *s) {
    if (!s) return T_ERROR;
    if (!strcmp(s, "integer"))    return T_INTEGER;
    if (!strcmp(s, "float"))      return T_FLOAT;
    if (!strcmp(s, "string"))     return T_STRING;
    if (!strcmp(s, "key"))        return T_KEY;
    if (!strcmp(s, "vector"))     return T_VECTOR;
    if (!strcmp(s, "rotation"))   return T_ROTATION;
    if (!strcmp(s, "quaternion")) return T_ROTATION;
    if (!strcmp(s, "list"))       return T_LIST;
    return T_ERROR;
}

int type_is_numeric(TypeKind t) {
    return t == T_INTEGER || t == T_FLOAT;
}

/*
 * Implicit assignability rules for LSL:
 *   integer  <- integer
 *   float    <- integer | float
 *   string   <- string | key            (key auto-coerces to string and vice
 *   key      <- key | string             versa in assignment context)
 *   vector   <- vector
 *   rotation <- rotation
 *   list     <- list
 *   any      <- anything (used for list-builder context)
 *
 * Note: LSL does NOT implicitly cast e.g. integer -> string. That requires
 * an explicit (string) cast.
 */
int type_implicit_assignable(TypeKind to, TypeKind from) {
    if (to == T_ERROR || from == T_ERROR) return 1;  /* suppress cascades */
    if (to == T_ANY) return 1;
    if (to == from) return 1;
    if (to == T_FLOAT && from == T_INTEGER) return 1;
    if (to == T_KEY && from == T_STRING) return 1;
    if (to == T_STRING && from == T_KEY) return 1;
    return 0;
}

/*
 * Explicit casts. LSL allows the following casts (see wiki: Typecast):
 *   (integer) string, (integer) float, (integer) integer
 *   (float)   string, (float) integer, (float) float
 *   (string)  ANYTHING            (every type stringifies)
 *   (key)     string, (key) key
 *   (vector)  string, (vector) vector
 *   (rotation)/quaternion string, rotation
 *   (list)    ANY single typed value (or list)
 */
int type_explicit_castable(TypeKind to, TypeKind from) {
    if (to == T_ERROR || from == T_ERROR) return 1;
    if (to == from) return 1;
    if (to == T_STRING) return 1;
    if (to == T_LIST)   return 1;
    switch (to) {
        case T_INTEGER:  return from == T_FLOAT || from == T_STRING;
        case T_FLOAT:    return from == T_INTEGER || from == T_STRING;
        case T_KEY:      return from == T_STRING;
        case T_VECTOR:   return from == T_STRING;
        case T_ROTATION: return from == T_STRING;
        default: break;
    }
    /* Reverse: integer/float can be obtained only from above */
    return 0;
}

/* Compute the result type of a binary operator. Caller still does its
 * own permission check. Returns T_ERROR if the combination is invalid. */
TypeKind type_binop_result(int op, TypeKind l, TypeKind r) {
    if (l == T_ERROR || r == T_ERROR) return T_ERROR;
    /* All comparison/logical ops yield integer (LSL booleans are integer). */
    switch (op) {
        case TK_LAND: case TK_LOR:
        case TK_EQ:   case TK_NEQ:
        case TK_LT:   case TK_GT: case TK_LE: case TK_GE:
            return T_INTEGER;
        default: break;
    }
    switch (op) {
        case TK_PLUS:
            if (l == T_LIST || r == T_LIST) return T_LIST;
            if (l == T_STRING && r == T_STRING) return T_STRING;
            if (l == T_VECTOR && r == T_VECTOR) return T_VECTOR;
            if (l == T_ROTATION && r == T_ROTATION) return T_ROTATION;
            if (type_is_numeric(l) && type_is_numeric(r))
                return (l == T_FLOAT || r == T_FLOAT) ? T_FLOAT : T_INTEGER;
            return T_ERROR;
        case TK_MINUS:
            if (l == T_VECTOR && r == T_VECTOR) return T_VECTOR;
            if (l == T_ROTATION && r == T_ROTATION) return T_ROTATION;
            if (type_is_numeric(l) && type_is_numeric(r))
                return (l == T_FLOAT || r == T_FLOAT) ? T_FLOAT : T_INTEGER;
            return T_ERROR;
        case TK_STAR:
            /* float = vector * vector  (dot product) */
            if (l == T_VECTOR && r == T_VECTOR) return T_FLOAT;
            /* vector = vector * scalar  (and scalar * vector) */
            if (l == T_VECTOR && type_is_numeric(r)) return T_VECTOR;
            if (type_is_numeric(l) && r == T_VECTOR) return T_VECTOR;
            /* vector = vector * rotation */
            if (l == T_VECTOR && r == T_ROTATION) return T_VECTOR;
            /* rotation = rotation * rotation */
            if (l == T_ROTATION && r == T_ROTATION) return T_ROTATION;
            if (type_is_numeric(l) && type_is_numeric(r))
                return (l == T_FLOAT || r == T_FLOAT) ? T_FLOAT : T_INTEGER;
            return T_ERROR;
        case TK_SLASH:
            if (l == T_VECTOR && type_is_numeric(r)) return T_VECTOR;
            if (l == T_VECTOR && r == T_ROTATION) return T_VECTOR;
            if (l == T_ROTATION && r == T_ROTATION) return T_ROTATION;
            if (type_is_numeric(l) && type_is_numeric(r))
                return (l == T_FLOAT || r == T_FLOAT) ? T_FLOAT : T_INTEGER;
            return T_ERROR;
        case TK_PERCENT:
            /* cross product on vectors */
            if (l == T_VECTOR && r == T_VECTOR) return T_VECTOR;
            if (l == T_INTEGER && r == T_INTEGER) return T_INTEGER;
            return T_ERROR;
        case TK_AND: case TK_OR: case TK_XOR:
        case TK_SHL: case TK_SHR:
            if (l == T_INTEGER && r == T_INTEGER) return T_INTEGER;
            return T_ERROR;
        default: break;
    }
    return T_ERROR;
}
