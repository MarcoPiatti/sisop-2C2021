#ifndef MEMORYCONFIG_H_
#define MEMORYCONFIG_H_

#include <commons/config.h>

typedef struct memoryConfig{
    t_config* config;
    char* memoryIp;
    char* memoryPort;
    char* swapIp;
    char* swapPort;
    int ramSize;
    int pageSize;
    char* tipoAsignacion;
    char* algoritmoMMU;
    int maxFramesMMU;
    int TLBSize;
    char* algoritmoTLB;
    int delayTLBHit;
    int delayTLBMiss;
    char* dumpPathTLB;
} t_memoryConfig;

/**
 * @DESC: genera una estructura con la config de la memoria a partir de un path
 */
t_memoryConfig* getMemoryConfig(char* path);

/**
 * @DESC: Destruye la estructura con los datos de config
 */
void destroyMemoryConfig(t_memoryConfig* config);

#endif // !MEMORYCONFIG_H_