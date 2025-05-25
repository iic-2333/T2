#include "osms_API.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

char* memory_path = NULL;

void os_mount(char* mem_path) {
    if (memory_path) free(memory_path);

    memory_path = (char*) malloc(strlen(mem_path) + 1);
    if (memory_path == NULL) {
        fprintf(stderr, "Error al asignar memoria.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(memory_path, mem_path);
    printf("Memoria montada correctamente: %s\n", memory_path);
}

FILE* open_mem(const char* mode) 
{    
    FILE* mem_file = fopen(memory_path, mode);
    if (mem_file == NULL) {
        perror("Error al abrir archivo de memoria");
        exit(EXIT_FAILURE);
    }
    return mem_file;
}

void os_ls_processes() 
{
    FILE* mem_file = open_mem("rb");
    fseek(mem_file, PCB_START, SEEK_SET);
    printf("Procesos en ejecución:\n");

    for (int i = 0; i < PCB_ENTRIES; i++) {
        unsigned char state;
        char name[14];
        unsigned char pid;

        fread(&state, 1, 1, mem_file);
        fread(name, 1, 14, mem_file);
        fread(&pid, 1, 1, mem_file);

        if (state == 0x01) {
            printf("ID: %d, Nombre: %s\n", pid, name);
        }
        fseek(mem_file, PCB_ENTRY_SIZE - 16, SEEK_CUR);
    }
    fclose(mem_file);
}

int os_exists(int process_id, char* file_name) 
{
    FILE* mem_file = open_mem("rb");
    unsigned char pcb_entry[PCB_ENTRY_SIZE];
    unsigned char estado;
    unsigned char pid; 

    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);  
        fread(pcb_entry, 1, PCB_ENTRY_SIZE, mem_file);

        estado = pcb_entry[0];
        if (estado != 0x01) continue;
        pid = pcb_entry[15];
        if (pid != process_id) continue;
        
        for (int j = 0; j < FILE_TABLE_ENTRIES; j++) {
            int file_offset = FILE_TABLE_REL_START + j * FILE_TABLE_ENTRY_SIZE;
            unsigned char valid = pcb_entry[file_offset];
            char fname[14];

            memcpy(fname, &pcb_entry[file_offset + 1], 14);
            if (valid == 0x01 && strncmp(fname, file_name, 14) == 0) {
                fclose(mem_file);
                printf("SI existe el archivo %s en el proceso %d \n", file_name, process_id);
                return 1;
            }
        }
    }
    fclose(mem_file);
    printf("NO existe el archivo %s en el proceso %d \n", file_name, process_id);
    return 0;
}

void os_ls_files(int process_id) 
{
    FILE* mem_file = open_mem("rb");
    unsigned char pcb_entry[PCB_ENTRY_SIZE];
    unsigned char estado;
    unsigned char pid;

    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        fread(pcb_entry, 1, PCB_ENTRY_SIZE, mem_file);
        
        estado = pcb_entry[0];
        if (estado != 0x01) continue;
        pid = pcb_entry[15];

        if (pid == process_id) {
            printf("Archivos del proceso %d:\n", process_id);

            for (int j = 0; j < FILE_TABLE_ENTRIES; j++) {
                int file_offset = FILE_TABLE_REL_START + j * FILE_TABLE_ENTRY_SIZE;
                unsigned char valid = pcb_entry[file_offset];
                char fname[15];
                memcpy(fname, &pcb_entry[file_offset + 1], 14);
                unsigned char fsize[5];
                memcpy(fsize, &pcb_entry[file_offset + 15], 5);
                unsigned char vaddr[4];
                memcpy(vaddr, &pcb_entry[file_offset + 20], 4);

                if (valid == 0x01) {
                    unsigned long long size = 0;
                    for (int k = 0; k < 5; k++) size |= ((unsigned long long)fsize[k]) << (8*k);
                    unsigned int vpn = (vaddr[0] | (vaddr[1]<<8) | (vaddr[2]<<16)) >> 5;
                    unsigned int offset = ((vaddr[2] & 0x1F) << 10) | (vaddr[3] << 2);

                    printf("VPN: %03X - File size: %llu - Virtual Adress 0x", vpn, size);
                    for (int k = 3; k >= 0; k--) printf("%02X", vaddr[k]);
                    printf(" - File name: %s\n", fname);
                }
            }
            fclose(mem_file);
            return;
        }       
    }  
    fclose(mem_file);
    printf("Error: Proceso con ID %d no encontrado o no está en ejecución.\n", process_id);      
}

void os_frame_bitmap() {
    FILE* mem_file = open_mem("rb");
    fseek(mem_file, FRAME_BITMAP_START, SEEK_SET);
    int usados = 0, libres = 0;
    for (int i = 0; i < FRAME_BITMAP_SIZE; i++) {
        unsigned char byte;
        fread(&byte, 1, 1, mem_file);
        for (int b = 0; b < 8; b++) {
            if (byte & (1 << b)) usados++;
            else libres++;
        }
    }
    printf("USADOS %d\nLIBRES %d\n", usados, libres);
    fclose(mem_file);
}

int os_start_process(int process_id, char* process_name) 
{
    if (strlen(process_name) > 14) {
        printf("Error: El nombre del proceso no puede superar los 14 caracteres.\n");
        return -1;
    }

    FILE* mem_file = open_mem("r+b");
    unsigned char state;
    char name[14];
    unsigned char pid;

    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        fread(&state, 1, 1, mem_file);
        fread(name, 1, 14, mem_file);
        fread(&pid, 1, 1, mem_file);

        if (pid == process_id) {
            if (state == 0x01) {
                printf("El proceso con ID %d ya estaba ejecutándose.\n", process_id);
            } else if (state == 0x00) {
                state = 0x01;
                fseek(mem_file, -16, SEEK_CUR);
                fwrite(&state, 1, 1, mem_file);
                printf("El proceso con ID %d ahora está en ejecución.\n", process_id);
            }
            fclose(mem_file);
            return 0;
        }
    }

    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        fread(&state, 1, 1, mem_file);

        if (state == 0x00) {            
            fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
            state = 0x01;
            fwrite(&state, 1, 1, mem_file);
            memset(name, 0, 14);
            strncpy(name, process_name, 14);
            fwrite(name, 1, 14, mem_file);
            fwrite(&process_id, 1, 1, mem_file);
            
            unsigned char cero[FILE_TABLE_SIZE] = {0};
            fwrite(cero, 1, FILE_TABLE_SIZE, mem_file);
            fclose(mem_file);
            printf("Proceso %s con ID %d ha sido iniciado.\n", process_name, process_id);
            return 0;
        }
    }
    fclose(mem_file);
    printf("Error: No se pudo iniciar este proceso.\n");
    return -1;
}

int os_finish_process(int process_id) {
    FILE* mem_file = open_mem("r+b");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char state;
        fread(&state, 1, 1, mem_file);
        if (state == 0x01) {
            fseek(mem_file, 14, SEEK_CUR);
            unsigned char pid;
            fread(&pid, 1, 1, mem_file);
            if (pid == process_id) {
                // Liberar memoria: invalidar archivos y limpiar entrada
                fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
                state = 0x00;
                fwrite(&state, 1, 1, mem_file);
                char cero[PCB_ENTRY_SIZE-1] = {0};
                fwrite(cero, 1, PCB_ENTRY_SIZE-1, mem_file);
                fclose(mem_file);
                return 0;
            }
        }
    }
    fclose(mem_file);
    return -1;
}

int os_rename_process(int process_id, char* new_name) {
    if (strlen(new_name) > 14) return -1;
    FILE* mem_file = open_mem("r+b");
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char state;
        fread(&state, 1, 1, mem_file);
        if (state == 0x01) {
            char name[14];
            fread(name, 1, 14, mem_file);
            unsigned char pid;
            fread(&pid, 1, 1, mem_file);
            if (pid == process_id) {
                // Cambiar name
                fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE + 1, SEEK_SET);
                char name_nuevo[14] = {0};
                strncpy(name_nuevo, new_name, 14);
                fwrite(name_nuevo, 1, 14, mem_file);
                fclose(mem_file);
                return 0;
            }
        }
    }
    fclose(mem_file);
    return -1;
}

// TODO: Implementar os_open, os_read_file, os_write_file, os_delete_file, os_close, os_cp 