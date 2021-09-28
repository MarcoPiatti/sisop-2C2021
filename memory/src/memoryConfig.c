#include "memoryConfig.h"

#include <stdlib.h>
#include <unistd.h>

t_memoryConfig* getMemoryConfig(char* path){
    t_memoryConfig* memoryConfig = malloc(sizeof(t_memoryConfig));
    memoryConfig->config = config_create(path);
    memoryConfig->memoryIp = config_get_string_value(memoryConfig->config, "IP");
    memoryConfig->memoryPort = config_get_string_value(memoryConfig->config, "PUERTO");
    memoryConfig->swapIp = config_get_string_value(memoryConfig->config, "IP_SWAP");
    memoryConfig->swapPort = config_get_string_value(memoryConfig->config, "PUERTO_SWAP");
    memoryConfig->ramSize = config_get_int_value(memoryConfig->config, "TAMANIO");
    memoryConfig->pageSize = config_get_int_value(memoryConfig->config, "TAMANIO_PAGINA");
    memoryConfig->tipoAsignacion = config_get_string_value(memoryConfig->config, "TIPO_ASIGNACION");
    memoryConfig->algoritmoMMU = config_get_string_value(memoryConfig->config, "ALGORITMO_REEMPLAZO_MMU");
    memoryConfig->maxFramesMMU = config_get_int_value(memoryConfig->config, "MARCOS_MAXIMOS");
    memoryConfig->TLBSize = config_get_int_value(memoryConfig->config, "CANTIDAD_ENTRADAS_TLB");
    memoryConfig->algoritmoTLB = config_get_string_value(memoryConfig->config, "ALGORITMO_REEMPLAZO_TLB");
    memoryConfig->delayTLBHit = config_get_int_value(memoryConfig->config, "RETARDO_ACIERTO_TLB");
    memoryConfig->delayTLBMiss = config_get_int_value(memoryConfig->config, "RETARDO_FALLO_TLB");
    memoryConfig->dumpPathTLB = config_get_string_value(memoryConfig->config, "DUMP_PATH");
    return memoryConfig;
}



void destroyMemoryConfig(t_memoryConfig* config){
    free(config->memoryIp);
    free(config->memoryPort);
    free(config->swapIp);
    free(config->swapPort);
    free(config->tipoAsignacion);
    free(config->algoritmoMMU);
    free(config->algoritmoTLB);
    free(config->dumpPathTLB);
    config_destroy(config->config);
    free(config);
}