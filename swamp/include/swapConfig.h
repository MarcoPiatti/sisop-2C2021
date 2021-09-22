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

/**
 * @DESC: Crea una estructura con todos los contenidos del config del swap
 */
t_swapConfig* getswapConfig(char* path);

/**
 * @DESC: Destruye de memoria una estructura con contenidos de config del swap
 */
void destroyswapConfig(t_swapConfig* config);

#endif // !swapCONFIG_H_