#include "swapConfig.h"
#include <stdlib.h>

t_swapConfig* getswapConfig(char* path){
    t_swapConfig* swapConfig = malloc(sizeof(t_swapConfig));
    swapConfig->config = config_create(path);
    swapConfig->swapIP = config_get_string_value(swapConfig->config, "IP");
    swapConfig->swapPort = config_get_string_value(swapConfig->config, "PUERTO");
    swapConfig->swapFiles = config_get_array_value(swapConfig->config, "ARCHIVOS_SWAP");
    swapConfig->fileSize = config_get_int_value(swapConfig->config, "TAMANIO_SWAP");
    swapConfig->pageSize = config_get_int_value(swapConfig->config, "TAMANIO_PAGINA");
    swapConfig->maxFrames = config_get_int_value(swapConfig->config, "MARCOS_MAXIMOS");
    swapConfig->delay = config_get_int_value(swapConfig->config, "RETARDO_SWAP");
    return swapConfig;
}



void destroyswapConfig(t_swapConfig* config){
    free(config->swapIP);
    free(config->swapPort);
    
    int i = 0;
    while(config->swapFiles[i]){
        free(config->swapFiles[i]);
        i++;
    }
    
    free(config->swapFiles[i]);
    free(config->swapFiles);
    config_destroy(config->config);
    free(config);
}