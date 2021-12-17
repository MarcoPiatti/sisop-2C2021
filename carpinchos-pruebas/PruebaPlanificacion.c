#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <lib/matelib.h>
#include <commons/log.h>

char *LOG_PATH = "./planificacion.log";
char *PROGRAM_NAME = "planificacion";
sem_t *va_el_2;
sem_t *va_el_3;
t_log *logger;

void imprimir_carpincho_n_hace_algo(int numero_de_carpincho)
{
    log_info(logger, "EJECUTANDO Carpincho %d", numero_de_carpincho);
    sleep(2);
}

void exec_carpincho_1(char *config)
{
    mate_instance self;
    mate_init(&self, config);
    sem_post(va_el_2);
    for (int i = 0; i < 3; i++)
    {
        imprimir_carpincho_n_hace_algo(1);
        mate_call_io(&self, (mate_io_resource) "pelopincho", "Carpincho 1 se va a IO");
    }
    imprimir_carpincho_n_hace_algo(1);
    mate_close(&self);
}

void exec_carpincho_2(char *config)
{
    mate_instance self;
    sem_wait(va_el_2);
    mate_init(&self, config);
    sem_post(va_el_3); //Creo que esta demas, es para que el 3 entre dsp del 2
    for (int i = 0; i < 3; i++)
    {
        imprimir_carpincho_n_hace_algo(2);
        mate_call_io(&self, (mate_io_resource) "pelopincho", "Carpincho 2 se va a IO");
    }
    imprimir_carpincho_n_hace_algo(2);
    mate_close(&self);
}

void exec_carpincho_3(char *config)
{
    mate_instance self;
    sem_wait(va_el_3);
    mate_init(&self, config);
    for (int i = 0; i < 3; i++)
    {
        imprimir_carpincho_n_hace_algo(3);
        imprimir_carpincho_n_hace_algo(3);
        imprimir_carpincho_n_hace_algo(3);
        imprimir_carpincho_n_hace_algo(3);
        imprimir_carpincho_n_hace_algo(3);
        mate_call_io(&self, (mate_io_resource) "pelopincho", "Carpincho 3 se va a IO");
    }
    imprimir_carpincho_n_hace_algo(3);
    imprimir_carpincho_n_hace_algo(3);
    imprimir_carpincho_n_hace_algo(3);
    imprimir_carpincho_n_hace_algo(3);
    imprimir_carpincho_n_hace_algo(3);
    mate_close(&self);
}

void free_all()
{
    sem_destroy(va_el_3);
    free(va_el_3);
    sem_destroy(va_el_2);
    free(va_el_2);

    log_destroy(logger);
}

void init_sems()
{
    va_el_2 = malloc(sizeof(sem_t));
    sem_init(va_el_2, 1, 0);
    va_el_3 = malloc(sizeof(sem_t));
    sem_init(va_el_3, 1, 0);
}

int main(int argc, char *argv[])
{
    logger = log_create(LOG_PATH, PROGRAM_NAME, true, LOG_LEVEL_DEBUG);
    pthread_t carpincho1_thread;
    pthread_t carpincho2_thread;
    pthread_t carpincho3_thread;

    init_sems();

    pthread_create(&carpincho1_thread, NULL, (void *)exec_carpincho_1, argv[1]);
    pthread_create(&carpincho2_thread, NULL, (void *)exec_carpincho_2, argv[1]);
    pthread_create(&carpincho3_thread, NULL, (void *)exec_carpincho_3, argv[1]);
    pthread_join(carpincho1_thread, NULL);
    pthread_join(carpincho2_thread, NULL);
    pthread_join(carpincho3_thread, NULL);
    free_all();
    puts("Termine!");
}