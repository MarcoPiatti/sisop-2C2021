#include "utils.h"

int32_t min(int32_t a, int32_t b){
    if (a < b) return a;
    return b;
}


int32_t div_roundUp(int32_t a, int32_t b){
    if (a % b == 0) return a/b;
    return a/b + 1;
 }