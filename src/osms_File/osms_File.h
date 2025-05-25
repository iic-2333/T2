#pragma once
#include <stdint.h>
#include <stdio.h>
// Aquí puedes agregar utilidades y tipos auxiliares para manejo de archivos y memoria

// Estructura para archivos abiertos
typedef struct osrmsFile {
    int process_id;
    char file_name[15]; // 14 + null
    int file_entry_index;
    int file_size;
    unsigned int virtual_addr;
    char mode; // 'r' o 'w'
    int current_offset;
} osrmsFile;

// Funciones de manejo de archivos
osrmsFile* os_open(int process_id, char* file_name, char mode);
int os_read_file(osrmsFile* file_desc, char* dest);
int os_write_file(osrmsFile* file_desc, char* src);
void os_delete_file(int process_id, char* file_name);
void os_close(osrmsFile* file_desc);
// BONUS
int os_cp(int pid_src, char* fname_src, int pid_dst, char* fname_dst);
