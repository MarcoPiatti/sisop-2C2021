#ifndef LOGS_H_
#define LOGS_H_

#include <commons/log.h>
#include <pthread.h>

t_log* logger;
pthread_mutex_t mutex_log;

#endif // !LOGS_H_