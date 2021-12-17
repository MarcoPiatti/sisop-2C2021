#!/usr/bin/python

import sys, os, _thread, os.path as pt

path = os.getcwd()

tests = ["batallaPorNordelta", "asignacionFija", "asignacionDinamica", "saludoAlHumedal", "deadlock", "mmuClock", "mmuLRU", "planificacionSJF", "planificacionHRRN", "suspension", "swamp", "tlbFIFO", "tlbLRU"]

def cleanLog():
    # Limpiar logs
    os.system("rm {}/*.log".format(path))
    print("Logs eliminados")

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print('Uso: python test.py "nombreTest" \n \t Reemplazar "nombreTest" por "list" para ver lista de todos los tests')
        sys.exit()

    if not os.path.exists("{}/build".format(path)):
        print("No se ha compilado el programa")
        sys.exit()
        

    test = sys.argv[1]

    cleanLog()

    print("Working directory: {}".format(path))

    if test == "batallaPorNordelta":
        print('Ejecutando /build/BatallaPorNordelta /build/aKernel.config')
        os.system("{}/build/BatallaPorNordelta {}/build/aKernel.config".format(path, path))

    elif "asignacion" in test:
        print("/build/PruebaAsignacion /build/aKernel.config")
        os.system("{}/build/PruebaAsignacion {}/build/aKernel.config".format(path, path))
        
    elif test == "saludoAlHumedal":
        print("Este test debe ser ejecutado a mano. Se recomienda el uso de 'screen'")

    elif test == "deadlock":
        print("/build/PruebaDeadlock /build/aKernel.config")
        os.system("{}/build/PruebaDeadlock {}/build/aKernel.config".format(path, path))

    elif test == "mmuClock":
        print("/build/PruebaMMU /build/aKernel.config")
        os.system("{}/build/PruebaMMU {}/build/aKernel.config".format(path, path))

    elif test == "mmuLRU":
        print("/build/PruebaAsignacion /build/aKernel.config")
        os.system("{}/build/PruebaAsignacion {}/build/aKernel.config".format(path, path))

    elif test == "planificacionSJF":
        print("/build/PruebaPlanificacion /build/aKernel.config")
        os.system("{}/build/PruebaPlanificacion {}/build/aKernel.config".format(path, path))

    elif test == "planificacionHRRN":
        print("/build/PruebaPlanificacion /build/aKernel.config")
        os.system("{}/build/PruebaPlanificacion {}/build/aKernel.config".format(path, path))

    elif test == "suspension":
        print("/build/PruebaSuspension /build/aKernel.config")
        os.system("{}/build/PruebaSuspension {}/build/aKernel.config".format(path, path))

    elif test == "swamp":
        print("/build/prueba_swamp /build/aMemoria.config")
        os.system("{}/build/prueba_swamp {}/build/aMemoria.config".format(path, path))

    elif test == "tlbFIFO":
        print("/build/prueba_tlb_fifo /build/aKernel.config")
        os.system("{}/build/prueba_tlb_fifo {}/build/aKernel.config".format(path, path))

    elif test == "tlbLRU":
        print("/build/prueba_tlb_lru /build/aKernel.config")
        os.system("{}/build/prueba_tlb_lru {}/build/aKernel.config".format(path, path))

    else:
        print("Tests reconocidos:")
        for test in tests:
            print("\t {}".format(test))
    

