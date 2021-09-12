#include "kernelConfig.h"

t_kernelConfig* getKernelConfig(char* path){
    t_config* config = config_create(path);
    t_kernelConfig* kernelConfig = malloc(sizeof(t_kernelConfig));
    kernelConfig->kernelIP = config_get_string_value(config, "IP_KERNEL");
    kernelConfig->kernelPort = config_get_string_value(config, "PUERTO_KERNEL");
    kernelConfig->memoryIP = config_get_string_value(config, "IP_MEMORIA");
    kernelConfig->memoryPort = config_get_string_value(config, "PUERTO_MEMORIA");
    kernelConfig->schedulerAlgorithm = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    kernelConfig->initialEstimator = config_get_int_value(config, "ESTIMACION_INICIAL");
    kernelConfig->alpha = config_get_double_value(config, "ALFA");
    kernelConfig->IODeviceNames = config_get_array_value(config, "DISPOSITIVOS_IO");
    kernelConfig->IODeviceDelays = config_get_array_value(config, "DURACIONES_IO");
    kernelConfig->CPUDelay = config_get_int_value(config, "RETARDO_CPU");
    kernelConfig->multiprogram = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    kernelConfig->multiprocess = config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");
    config_destroy(config);
    return kernelConfig;
}

void destroyKernelConfig(t_kernelConfig* config){
    free(config->kernelIP);
    free(config->kernelPort);
    free(config->memoryIP);
    free(config->memoryPort);
    free(config->schedulerAlgorithm);
    for(int i = 0; config->IODeviceNames[i]; i++){
        free(config->IODeviceNames[i]);
        free(config->IODeviceDelays[i]);
    }
    free(config->IODeviceNames);
    free(config->IODeviceDelays);
    free(config);
}