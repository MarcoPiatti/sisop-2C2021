#ifndef KERNELCONFIG_H_
#define KERNELCONFIG_H_

#include<commons/config.h>

typedef struct kernelConfig {
    t_config* config;
    char* ip;
    char* port;
    char* memoryIP;
    char* memoryPort;
    char* algorithm;
    double initialEstimation;
    double alpha;
    char** IODevices;
    char** IODurations;
    int multiprogram;
    int multiprocess;
    int deadlockTime;
} t_kernelConfig;

t_kernelConfig* getKernelConfig(char* path);

void destroyKernelConfig(t_kernelConfig* config);

#endif