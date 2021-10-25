#ifndef SWAP_H_
#define SWAP_H_
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include "swapConfig.h"
#include "swapFile.h"
#include "sleeper.h"

t_swapConfig* swapConfig;

t_log *swapLogger;

int (*asignacion) (int PID, int page, void* content);

extern void (*petitionHandler[MAX_MEM_PETITIONS])(t_packet *received, int clientSocket);

#endif // !SWAP_H_