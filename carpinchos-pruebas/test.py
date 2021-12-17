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
        os.system("{}/build/BatallaPorNordelta {}/build/aKernel.config".format(path, path))

    elif "asignacion" in test:
        os.system("{}/build/PruebaAsignacion {}/build/aKernel.config".format(path, path))
        
    elif test == "saludoAlHumedal":
        _thread.start_new_thread(os.system, ("{}/build/PruebaBase_Carpincho1 {}/build/aKernel.config".format(path, path),))
        input("Press Enter to continue...")
        _thread.start_new_thread(os.system, ("{}/build/PruebaBase_Carpincho2 {}/build/aKernel.config".format(path, path),))

    elif test == "deadlock":
        os.system("{}/build/PruebaDeadlock {}/build/aKernel.config".format(path, path))

    elif test == "mmuClock":
        os.system("{}/build/PruebaMMU {}/build/aKernel.config".format(path, path))

    elif test == "mmuLRU":
        os.system("{}/build/PruebaAsignacion {}/build/aKernel.config".format(path, path))

    elif test == "planificacionSJF":
        os.system("{}/build/PruebaPlanificacion {}/build/aKernel.config".format(path, path))

    elif test == "planificacionHRRN":
        os.system("{}/build/PruebaPlanificacion {}/build/aKernel.config".format(path, path))

    elif test == "suspension":
        os.system("{}/build/PruebaSuspension {}/build/aKernel.config".format(path, path))

    elif test == "swamp":
        os.system("{}/build/prueba_swamp {}/build/aMemoria.config".format(path, path))

    elif test == "tlbFIFO":
        os.system("{}/build/prueba_tlb_fifo {}/build/aKernel.config".format(path, path))

    elif test == "tlbLRU":
        os.system("{}/build/prueba_tlb_lru {}/build/aKernel.config".format(path, path))

    else:
        print("Tests reconocidos:")
        for test in tests:
            print("\t {}".format(test))
    

