/**
 * @file: kernelConfig.c
 * @author pepinOS 
 * @DESC: TAD para un archivo de configuracion del Kernel
 * @version 0.1
 * @date: 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "kernelConfig.h"
#include <stdlib.h>

t_kernelConfig* getKernelConfig(char* path){
    t_kernelConfig* kernelConfig = malloc(sizeof(t_kernelConfig));
    kernelConfig->config = config_create(path);
    kernelConfig->kernelIP = config_get_string_value(kernelConfig->config, "IP_KERNEL");
    kernelConfig->kernelPort = config_get_string_value(kernelConfig->config, "PUERTO_KERNEL");
    kernelConfig->memoryIP = config_get_string_value(kernelConfig->config, "IP_MEMORIA");
    kernelConfig->memoryPort = config_get_string_value(kernelConfig->config, "PUERTO_MEMORIA");
    kernelConfig->schedulerAlgorithm = config_get_string_value(kernelConfig->config, "ALGORITMO_PLANIFICACION");
    kernelConfig->initialEstimator = config_get_double_value(kernelConfig->config, "ESTIMACION_INICIAL");
    kernelConfig->alpha = config_get_double_value(kernelConfig->config, "ALFA");
    kernelConfig->IODeviceNames = config_get_array_value(kernelConfig->config, "DISPOSITIVOS_IO");
    kernelConfig->IODeviceDelays = config_get_array_value(kernelConfig->config, "DURACIONES_IO");
    kernelConfig->DeadlockDelay = config_get_int_value(kernelConfig->config, "TIEMPO_DEADLOCK");
    kernelConfig->multiprogram = config_get_int_value(kernelConfig->config, "GRADO_MULTIPROGRAMACION");
    kernelConfig->multiprocess = config_get_int_value(kernelConfig->config, "GRADO_MULTIPROCESAMIENTO");
    return kernelConfig;
}

void destroyKernelConfig(t_kernelConfig* config){
    free(config->kernelIP);
    free(config->kernelPort);
    free(config->memoryIP);
    free(config->memoryPort);
    free(config->schedulerAlgorithm);
    
    int i = 0;
    while(config->IODeviceNames[i]){
        free(config->IODeviceNames[i]);
        free(config->IODeviceDelays[i]);
        i++;
    }
    
    free(config->IODeviceNames[i]);
    free(config->IODeviceDelays[i]);
    free(config->IODeviceNames);
    free(config->IODeviceDelays);

    config_destroy(config->config);
    free(config);
}