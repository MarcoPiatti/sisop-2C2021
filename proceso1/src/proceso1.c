#include "proceso1.h"

int main(int argc, char** argv)
{
    if (argc > 1 && strcmp(argv[1], "-test") == 0) {
        run_tests();
        return 0;
    }
    t_log* logger = log_create("./cfg/proceso1.log", "PROCESO1", true, LOG_LEVEL_INFO);
    log_info(logger, "Soy el proceso 1!");

    log_destroy(logger);
}