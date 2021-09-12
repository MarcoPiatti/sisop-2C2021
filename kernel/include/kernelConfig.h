#ifndef KERNELCONFIG_H_
#define KERNELCONFIG_H_

#include <commons/config.h>

typedef struct kernelConfig{
    char* kernelIP;
    char* kernelPort;
    char* memoryIP;
    char* memoryPort;
    char* schedulerAlgorithm;
    int initialEstimator;
    double alpha;
    char** IODeviceNames;
    char** IODeviceDelays;
    int CPUDelay;
    int multiprogram;
    int multiprocess;
} t_kernelConfig;

t_kernelConfig* getKernelConfig(char* path);

void destroyKernelConfig(t_kernelConfig* config);

#endif // !KERNELCONFIG_H_

