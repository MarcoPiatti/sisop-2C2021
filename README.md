# Trabajo Práctico 2C2021 - Sistemas Operativos UTN FRBA

[Link a la Consigna](https://docs.google.com/document/d/1BDpr5lfzOAqmOOgcAVg6rUqvMPUfCpMSz1u1J_Vjtac/edit)

## Vistazo general

Sistema distribuído concurrente que simula algunos aspectos del funcionamiento de un S.O de libro como visto en el Stallings o el Silberschatz.

Cuenta con 3 procesos principales: `kernel`, `memoria` y `swap`. Los cuales pueden ejecutarse en diferentes direcciones de red.

A través de una biblioteca de C `matelib`, los clientes pueden conectarse por red y hacer uso de los siguientes servicios:

### Asignación de Memoria (Memoria y Swap):
- Solicitar y liberar N bytes de memoria, de manera simil al `malloc()` en C.
- Leer y escribir de/hacia los tramos de memoria asignada, de manera simil al `memcpy()` en C.

### Sincronización con otros clientes (Kernel):
- Crear y eliminar semáforos.
- Hacer wait y post en semáforos existentes.
- Detección y solución de Deadlocks dando de baja a la cantidad mínima de clientes involucrados.

Los clientes pueden conectarse directamente a la Memoria, u opcionalmente al Kernel para todas las funcionalidades.

### Funcionamiento general (Memoria y Swap):
- El modulo Memoria implementa un esquema de paginación pura, con una tabla de páginas por cliente.
- Dentro del espacio de direccionamiento lógico de cada cliente, la memoria se asigna bajo criterio First Fit.
- El modulo Memoria cuenta con una TLB, con algoritmos de reeplazo de entradas FIFO o LRU.
- En caso de estar llena la memoria principal, esta mueve páginas al módulo Swap.
- Criterios de asignación de frames: Tamaño fijo con reemplazo local o Dinámico con reemplazo global.
- Algoritmos de selección de frame para reemplazos: Clock-Modificado o LRU.

### Funcionamiento general (Kernel):
- El modulo Kernel simula un planificador con grado de multiprocesamiento N y multiprogramacion M.
- Los clientes conectados al modulo kernel son tratados como procesos a ser planificados.
- Algoritmos de planificación SJF o HRRN.
- No hay desalojo forzoso, un cliente solo deja de ejecutar si se desconecta o al bloquearse en un semaforo.
- Suspensión de procesos bloqueados al ingresar nuevos clientes y estar lleno el grado de multiprogramación.
