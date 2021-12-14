#!/usr/bin/python

import sys, os

path = os.getcwd()

def replace_module_config(module, config):
    # Copiar config deseada al modulo especificado
    sourceFilePath = '{}/cfg/{}.config'.format(module, config)
    targetFilePath = '{}/cfg/{}.config'.format(module, module)
    os.system("cp {} {}".format(sourceFilePath, targetFilePath))
    print("Ejecutado: cp {} {}".format(sourceFilePath, targetFilePath))

modulos = ["kernel", "swamp", "memory"]
tests = ["batallaPorNordelta", "asignacionFija", "asignacionDinamica", "saludoAlHumedal", "deadlock", "mmuClock", "mmuLRU", "planificacionSJF", "planificacionHRRN", "suspension", "swamp", "tlbFIFO", "tlbLRU"]

def cleanLogs():
    # Limpiar logs
    for modulo in modulos:
        os.system("rm ./{}/{}.log".format(modulo, modulo))
        print("rm ./{}/{}.log".format(modulo, modulo))
    


if __name__ == "__main__":

    if len(sys.argv) != 2:
        print('Uso: python testear.py "nombreTest" \n \t Reemplazar "nombreTest" por "list" para ver lista de todos los tests')
        

    test = sys.argv[1]

    print("Working directory: {}".format(path))

    if test == "batallaPorNordelta":
        for modulo in modulos:
            replace_module_config(modulo, "batallaPorNordelta")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "BatallaPorNordelta" conectado a kernel.')

    elif "asignacion" in test:
        camelCaseTest = test.replace("asignacion", "Asignacion")
        replace_module_config("kernel", "memoriaAsignacion")
        replace_module_config("swamp", "memoriaAsignacion")
        replace_module_config("memory", "memoria{}".format(camelCaseTest))  
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaAsignacion" conectado a kernel.')
        
    elif test == "saludoAlHumedal":
        for modulo in modulos:
            replace_module_config(modulo, "saludoAlHumedal")
        print("Deben ejecutarse los 3 modulos")
        print("Deben ejecutarse los carpinchos PruebaBase_Carpincho1 y PruebaBase_Carpincho2. Conectados a kernel.")

    elif test == "deadlock":
        replace_module_config("kernel", "kernelDeadlock")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaDeadlock" conectado a kernel.')

    elif test == "mmuClock":
        replace_module_config("memory", "reemplazoMMUClock") 
        replace_module_config("kernel", "memoriaReemplazoMMU")
        replace_module_config("swamp", "memoriaReemplazoMMU")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaMMU" conectado a kernel.')

    elif test == "mmuLRU":
        replace_module_config("memory", "reemplazoMMULRU")  
        replace_module_config("kernel", "memoriaReemplazoMMU")
        replace_module_config("swamp", "memoriaReemplazoMMU")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaAsignacion" conectado a kernel.')

    elif test == "planificacionSJF":
        replace_module_config("kernel", "planificacionSJF") 
        replace_module_config("memory", "kernelPlanificacion")
        replace_module_config("swamp", "kernelPlanificacion")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaPlanificacion" conectado a kernel.')

    elif test == "planificacionHRRN":
        replace_module_config("kernel", "planificacionHRRN") 
        replace_module_config("memory", "kernelPlanificacion")
        replace_module_config("swamp", "kernelPlanificacion")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaPlanificacion" conectado a kernel.')

    elif test == "suspension":
        for modulo in modulos:
            replace_module_config(modulo, "kernelSuspension")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "PruebaSuspension" conectado a kernel.')

    elif test == "swamp":
        replace_module_config("memory", "swamp")
        replace_module_config("swamp", "swampTest")
        print("Deben ejecutarse SOLO los modulos de MEMORIA y SWAMP")
        print('Debe ejecutarse el carpincho "prueba_swamp" conectado a MEMORIA.')

    elif test == "tlbFIFO":
        replace_module_config("kernel", "memoriaTLB")
        replace_module_config("memory", "memoriaTLBFIFO") 
        replace_module_config("swamp", "memoriaTLB")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "prueba_tlb_fifo" conectado a kernel.')

    elif test == "tlbLRU":
        replace_module_config("kernel", "memoriaTLB")
        replace_module_config("memory", "memoriaTLBLRU") 
        replace_module_config("swamp", "memoriaTLB")
        print("Deben ejecutarse los 3 modulos")
        print('Debe ejecutarse el carpincho "prueba_tlb_lru" conectado a kernel.')

    else:
        print("Tests reconocidos:")
        for test in tests:
            print("\t {}".format(test))
    

