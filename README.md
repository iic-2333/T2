# OSRMS - Sistema de Gestión de Memoria Principal

## Introducción

Este proyecto implementa una API para manejar el contenido de una memoria principal organizada en frames. La API permite gestionar procesos simulados, asignar memoria física, liberar memoria y manejar archivos en la memoria virtual de cada proceso.

## Estructura de la Memoria

La memoria está organizada en los siguientes segmentos:
- **Tabla de PCBs**: 8 KB iniciales
- **Tabla Invertida de Páginas**: 192 KB siguientes
- **Frame Bitmap**: 8 KB siguientes
- **Frames**: 2 GB finales, divididos en frames de 32 KB

## Funciones Implementadas

### Funciones Generales
- `os_mount`: Monta el archivo de memoria
- `os_ls_processes`: Lista los procesos activos
- `os_exists`: Verifica si un archivo existe en la memoria de un proceso
- `os_ls_files`: Lista los archivos de un proceso
- `os_frame_bitmap`: Muestra el estado del bitmap de frames

### Funciones de Procesos
- `os_start_process`: Inicia un nuevo proceso
- `os_finish_process`: Finaliza un proceso
- `os_rename_process`: Renombra un proceso

### Funciones de Archivos
- `os_open`: Abre un archivo en modo lectura o escritura
- `os_read_file`: Lee un archivo de la memoria virtual a un archivo local
- `os_write_file`: Escribe un archivo local a la memoria virtual
- `os_delete_file`: Elimina un archivo de la memoria virtual
- `os_close`: Cierra un archivo abierto
- `os_cp`: (BONUS) Copia un archivo entre procesos

## Compilación y Ejecución

Para compilar el proyecto:
```
make
```

Para ejecutar:
```
./osms <ruta_a_memoria>
```

o

```
./osms_memfilled <ruta_a_memoria>
```

## Notas de Implementación

- Todas las operaciones de memoria se realizan en little-endian
- El sistema de paginación utiliza una tabla de páginas invertida
- No se implementa compactación ni defragmentación de memoria
- Al eliminar archivos o procesos, los datos permanecen en memoria pero los frames se marcan como disponibles
