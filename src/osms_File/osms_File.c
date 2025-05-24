#include "osms_File.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../osms_API/osms_API.h"
// Aquí puedes agregar funciones auxiliares para manejo de archivos y memoria 

osrmsFile* os_open(int process_id, char* file_name, char mode) {
    // TODO: Implementar apertura de archivo virtual (lectura de tabla de archivos, validación, etc)
    // Debe buscar el archivo en la tabla de archivos del proceso y retornar un osrmsFile* válido
    // Si mode == 'w', crear entrada si no existe
    return NULL;
}

int os_read_file(osrmsFile* file_desc, char* dest) {
    // TODO: Implementar lectura de archivo virtual a archivo local
    // Leer los datos del archivo virtual y escribirlos en el archivo local dest
    return 0;
}

int os_write_file(osrmsFile* file_desc, char* src) {
    // TODO: Implementar escritura de archivo local a archivo virtual
    // Leer src y escribir en la memoria virtual del proceso, usando paginación
    return 0;
}

void os_delete_file(int process_id, char* file_name) {
    // TODO: Implementar borrado de archivo virtual
    // Invalidar entrada en tabla de archivos, liberar frames y actualizar bitmap
}

void os_close(osrmsFile* file_desc) {
    // TODO: Implementar cierre de archivo virtual (liberar recursos si es necesario)
}

int os_cp(int pid_src, char* fname_src, int pid_dst, char* fname_dst) {
    // TODO: Implementar copia de archivo entre procesos usando open/read/write
    return -1;
} 