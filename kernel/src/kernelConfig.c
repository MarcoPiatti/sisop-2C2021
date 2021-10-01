#include "kernelConfig.h"
#include <stdlib.h>

t_kernelConfig* getKernelConfig(char* path) {
    t_kernelConfig* kernelConfig = malloc(sizeof(t_kernelConfig));
    kernelConfig->config = config_create(path);
    kernelConfig->ip = config_get_int_value(kernelConfig->config, "IP");
    kernelConfig->port = config_get_int_value(kernelConfig->config, "PORT");
    kernelConfig->memoryIP = config_get_string_value(kernelConfig->config, "IP_MEMORIA");
    kernelConfig->memoryPort = config_get_int_value(kernelConfig->config, "PUERTO_MEMORIA");
    kernelConfig->algorithm = config_get_string_value(kernelConfig->config, "ALGORITMO_PLANIFICACION");
    kernelConfig->initialEstimation = config_get_double_value(kernelConfig->config, "ESTIMACION_INICIAL");
    kernelConfig->alpha = config_get_double_value(kernelConfig->config, "ALFA");
    kernelConfig->IODevices = config_get_string_value(kernelConfig->config, "DISPOSITIVOS_IO");
    kernelConfig->IODurations = config_get_string_value(kernelConfig->config, "DURACIONES_IO");
    kernelConfig->multiprogram = config_get_int_value(kernelConfig->config, "GRADO_MULTIPROGRAMACION");
    kernelConfig->multiprocess = config_get_int_value(kernelConfig->config, "GRADO_MULTIPROCESAMIENTO");
    kernelConfig->deadlockTime = config_get_int_value(kernelConfig->config, "TIEMPO_DEADLOCK");
    return kernelConfig;
}

void destroyKernelConfig(t_kernelConfig* config) {

    free(config->ip);
    free(config->memoryIP);
    free(config->algorithm);

    int i = 0;
    while(config->IODevices[i]) {
        free(config->IODevices[i]);
        i++;
    }
    free(config->IODevices[i]);
    free(config->IODevices);

    int j = 0;
    while(config->IODurations[j]) {
        free(config->IODurations[j]);
        j++;
    }
    free(config->IODurations[j]);
    free(config->IODurations);

    config_destroy(config);

    free(config);
}