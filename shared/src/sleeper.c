#include "sleeper.h"

void milliSleep(int milliseconds){
    for(int i = 0; i< milliseconds; i++){
        usleep(1000);
    }
}