#!/usr/bin/python

import sys, os

path = os.getcwd()
module = "kernel" # TODO: Cambiar según haga falta

def replace_module_config(config):
    # Copiar config deseada al modulo especificado
    sourceFilePath = '{}/cfg/{}.config'.format(path, config)
    targetFilePath = '{}/cfg/{}.config'.format(path, module)
    os.system("cp {} {}".format(sourceFilePath, targetFilePath))
    print("Ejecutado: cp {} {}".format(sourceFilePath, targetFilePath))

tests = ["batallaPorNordelta", "asignacionFija", "asignacionDinamica", "saludoAlHumedal", "deadlock", "mmuClock", "mmuLRU", "planificacionSJF", "planificacionHRRN", "suspension", "swamp", "tlbFIFO", "tlbLRU"]

def cleanLog():
    # Limpiar logs
    os.system("rm {}/{}.log".format(path, module))
    print("Logs eliminados")
    
    


if __name__ == "__main__":

    if len(sys.argv) != 3:
        print('Uso: python test.py "nombreTest" "1"/"0" (Para iniciar módulo) \n \t Reemplazar "nombreTest" por "list" para ver lista de todos los tests')
        

    test = sys.argv[1]

    cleanLog()

    print("Working directory: {}".format(path))

    if test == "batallaPorNordelta":
        replace_module_config("batallaPorNordelta")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "BatallaPorNordelta" conectado a kernel.')

    elif "asignacion" in test:
        camelCaseTest = test.replace("asignacion", "Asignacion")
        replace_module_config("memoriaAsignacion")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaAsignacion" conectado a kernel.')
        
    elif test == "saludoAlHumedal":
        replace_module_config("saludoAlHumedal")
        print("Deben ejecutarse los 3 modulos")
        print("Deben ejecutarse los carpinchos PruebaBase_Carpincho1 y PruebaBase_Carpincho2. Conectados a kernel.")

    elif test == "deadlock":
        replace_module_config("kernelDeadlock")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaDeadlock" conectado a kernel.')

    elif test == "mmuClock":
        replace_module_config("memoriaReemplazoMMU")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaMMU" conectado a kernel.')

    elif test == "mmuLRU":
        replace_module_config("memoriaReemplazoMMU")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaAsignacion" conectado a kernel.')

    elif test == "planificacionSJF":
        replace_module_config("planificacionSJF") 
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaPlanificacion" conectado a kernel.')

    elif test == "planificacionHRRN":
        replace_module_config("planificacionHRRN") 
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaPlanificacion" conectado a kernel.')

    elif test == "suspension":
        replace_module_config("kernelSuspension")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaSuspension" conectado a kernel.')

    elif test == "swamp":
        print("Deben ejecutarse SOLO los modulos de MEMORIA y SWAMP")
        print('Debe ejecutarse el carpincho "prueba_swamp" conectado a MEMORIA.')

    elif test == "tlbFIFO":
        replace_module_config("memoriaTLB")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "prueba_tlb_fifo" conectado a kernel.')

    elif test == "tlbLRU":
        replace_module_config("memoriaTLB")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "prueba_tlb_lru" conectado a kernel.')

    else:
        print("Tests reconocidos:")
        for test in tests:
            print("\t {}".format(test))
        sys.exit()

    if "1" == sys.argv[2]:
        print("Ejecutando {}.out".format(module))
        os.system("./{}.out".format(module))
    

