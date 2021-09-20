#ifndef SWAPCONFIG_H_
#define SWAPCONFIG_H_

#include <commons/config.h>

typedef struct swapConfig{
    t_config* config;
    char* swapIP;
    char* swapPort;
    char** swapFiles;         
    int fileSize;
    int pageSize;
    int maxFrames;
    int delay;
} t_swapConfig;

t_swapConfig* getswapConfig(char* path);

void destroyswapConfig(t_swapConfig* config);

#endif // !swapCONFIG_H_