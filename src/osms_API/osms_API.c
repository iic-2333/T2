#include "osms_API.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
static char* memory_path = NULL;

void os_mount(char* mem_path) {
    if (memory_path) free(memory_path);
    memory_path = (char*) malloc(strlen(mem_path) + 1);
    if (!memory_path) {
        fprintf(stderr, "Error al asignar memoria.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(memory_path, mem_path);
    printf("Memoria montada: %s\n", memory_path);
}

static FILE* open_mem(const char* mode) {
    if (!memory_path) {
        fprintf(stderr, "Memoria no montada.\n");
        exit(EXIT_FAILURE);
    }
    FILE* f = fopen(memory_path, mode);
    if (!f) {
        perror("Error al abrir archivo de memoria");
        exit(EXIT_FAILURE);
    }
    return f;
}

void os_ls_processes() {
    FILE* f = open_mem("rb");
    fseek(f, PCB_TABLE_START, SEEK_SET);
    printf("Procesos en ejecución:\n");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        unsigned char estado;
        char nombre[15] = {0};
        unsigned char pid;
        fread(&estado, 1, 1, f);
        fread(nombre, 1, 14, f);
        fread(&pid, 1, 1, f);
        if (estado == 0x01) {
            printf("%d %s\n", pid, nombre);
        }
        fseek(f, PCB_ENTRY_SIZE - 16, SEEK_CUR);
    }
    fclose(f);
}

int os_exists(int process_id, char* file_name) {
    FILE* f = open_mem("rb");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(f, PCB_TABLE_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char estado;
        fread(&estado, 1, 1, f);
        if (estado != 0x01) continue;
        fseek(f, 14, SEEK_CUR); // nombre
        unsigned char pid;
        fread(&pid, 1, 1, f);
        if (pid != process_id) continue;
        fseek(f, 1, SEEK_CUR); // skip to file table
        for (int j = 0; j < FILE_TABLE_ENTRIES; j++) {
            unsigned char valid;
            char fname[15] = {0};
            fseek(f, PCB_TABLE_START + i * PCB_ENTRY_SIZE + 16 + j * FILE_TABLE_ENTRY_SIZE, SEEK_SET);
            fread(&valid, 1, 1, f);
            fread(fname, 1, 14, f);
            if (valid == 0x01 && strncmp(fname, file_name, 14) == 0) {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

void os_ls_files(int process_id) {
    FILE* f = open_mem("rb");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(f, PCB_TABLE_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char estado;
        fread(&estado, 1, 1, f);
        if (estado != 0x01) continue;
        fseek(f, 14, SEEK_CUR); // nombre
        unsigned char pid;
        fread(&pid, 1, 1, f);
        if (pid != process_id) continue;
        fseek(f, 1, SEEK_CUR); // skip to file table
        printf("Archivos del proceso %d:\n", process_id);
        for (int j = 0; j < FILE_TABLE_ENTRIES; j++) {
            unsigned char valid;
            char fname[15] = {0};
            unsigned char fsize[5];
            unsigned char vaddr[4];
            fseek(f, PCB_TABLE_START + i * PCB_ENTRY_SIZE + 16 + j * FILE_TABLE_ENTRY_SIZE, SEEK_SET);
            fread(&valid, 1, 1, f);
            fread(fname, 1, 14, f);
            fread(fsize, 1, 5, f);
            fread(vaddr, 1, 4, f);
            if (valid == 0x01) {
                unsigned long long size = 0;
                for (int k = 0; k < 5; k++) size |= ((unsigned long long)fsize[k]) << (8*k);
                unsigned int vpn = (vaddr[0] | (vaddr[1]<<8) | (vaddr[2]<<16)) >> 5;
                unsigned int offset = ((vaddr[2] & 0x1F) << 10) | (vaddr[3] << 2);
                printf("%03X %llu 0x", vpn, size);
                for (int k = 3; k >= 0; k--) printf("%02X", vaddr[k]);
                printf(" %s\n", fname);
            }
        }
    }
    fclose(f);
}

void os_frame_bitmap() {
    FILE* f = open_mem("rb");
    fseek(f, FRAME_BITMAP_START, SEEK_SET);
    int usados = 0, libres = 0;
    for (int i = 0; i < FRAME_BITMAP_SIZE; i++) {
        unsigned char byte;
        fread(&byte, 1, 1, f);
        for (int b = 0; b < 8; b++) {
            if (byte & (1 << b)) usados++;
            else libres++;
        }
    }
    printf("USADOS %d\nLIBRES %d\n", usados, libres);
    fclose(f);
}

int os_start_process(int process_id, char* process_name) {
    if (strlen(process_name) > 14) return -1;
    FILE* f = open_mem("r+b");
    // Buscar entrada libre
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(f, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char estado;
        fread(&estado, 1, 1, f);
        if (estado == 0x00) {
            // Escribir estado, nombre, id y limpiar tabla de archivos
            fseek(f, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
            estado = 0x01;
            fwrite(&estado, 1, 1, f);
            char nombre[14] = {0};
            strncpy(nombre, process_name, 14);
            fwrite(nombre, 1, 14, f);
            unsigned char pid = (unsigned char)process_id;
            fwrite(&pid, 1, 1, f);
            // Limpiar tabla de archivos
            unsigned char cero[FILE_TABLE_SIZE] = {0};
            fwrite(cero, 1, FILE_TABLE_SIZE, f);
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return -1;
}

int os_finish_process(int process_id) {
    FILE* f = open_mem("r+b");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(f, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char estado;
        fread(&estado, 1, 1, f);
        if (estado == 0x01) {
            fseek(f, 14, SEEK_CUR);
            unsigned char pid;
            fread(&pid, 1, 1, f);
            if (pid == process_id) {
                // Liberar memoria: invalidar archivos y limpiar entrada
                fseek(f, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
                estado = 0x00;
                fwrite(&estado, 1, 1, f);
                char cero[PCB_ENTRY_SIZE-1] = {0};
                fwrite(cero, 1, PCB_ENTRY_SIZE-1, f);
                fclose(f);
                return 0;
            }
        }
    }
    fclose(f);
    return -1;
}

int os_rename_process(int process_id, char* new_name) {
    if (strlen(new_name) > 14) return -1;
    FILE* f = open_mem("r+b");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(f, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char estado;
        fread(&estado, 1, 1, f);
        if (estado == 0x01) {
            char nombre[14];
            fread(nombre, 1, 14, f);
            unsigned char pid;
            fread(&pid, 1, 1, f);
            if (pid == process_id) {
                // Cambiar nombre
                fseek(f, PCB_START + i * PCB_ENTRY_SIZE + 1, SEEK_SET);
                char nombre_nuevo[14] = {0};
                strncpy(nombre_nuevo, new_name, 14);
                fwrite(nombre_nuevo, 1, 14, f);
                fclose(f);
                return 0;
            }
        }
    }
    fclose(f);
    return -1;
}

// TODO: Implementar os_open, os_read_file, os_write_file, os_delete_file, os_close, os_cp 