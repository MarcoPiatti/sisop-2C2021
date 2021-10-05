#ifndef MEMORY_CONFIG_H_
#define MEMORY_CONFIG_H_
#include <commons/config.h>

typedef struct memoryConfig{
    t_config *config;
    char *ip;
    int port;
    int size;
    int pageSize;
    char *MMUreplacementAlgorithm;
    char *assignmentType;
    int framesPerProcess;
    int TLBEntryAmount;
    char *TLBReplacementAlgorithm;
    int TLBHitDelay;
    int TLBMissDelay;
} t_memoryConfig;

t_memoryConfig *getMemoryConfig(char *path);

void destroyMemoryConfig(t_memoryConfig *config);

#endif // !MEMORY_CONFIG_H_