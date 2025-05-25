#include "osms_File.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> // For close()
#include "../osms_API/osms_API.h"

// Declaración externa para la función os_exists implementada en osms_API.c
extern int os_exists(int process_id, char* file_name);

// Funciones auxiliares para manejo de archivos y memoria

// Función para acceder al archivo de memoria
extern FILE* open_mem(const char* mode);

// Encuentra una entrada de archivo en la tabla de archivos de un proceso
int find_file_entry(int process_id, const char* file_name, unsigned char* pcb_entry) {
    FILE* mem_file = open_mem("rb");
    bool process_found = false;

    // Buscar el PCB del proceso
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        fread(pcb_entry, 1, PCB_ENTRY_SIZE, mem_file);

        unsigned char state = pcb_entry[0];
        unsigned char pid = pcb_entry[15];

        if (state == 0x01 && pid == process_id) {
            process_found = true;
            break;
        }
    }

    if (!process_found) {
        fclose(mem_file);
        return -1; // Proceso no encontrado
    }

    // Buscar el archivo en la tabla de archivos del proceso
    for (int j = 0; j < FILE_TABLE_ENTRIES; j++) {
        int file_offset = FILE_TABLE_REL_START + j * FILE_TABLE_ENTRY_SIZE;
        unsigned char valid = pcb_entry[file_offset];
        char fname[15];
        memcpy(fname, &pcb_entry[file_offset + 1], 14);
        fname[14] = '\0'; // Asegurar que termina en null

        if (valid == 0x01 && strncmp(fname, file_name, 14) == 0) {
            fclose(mem_file);
            return j; // Índice de la entrada del archivo
        }
    }

    fclose(mem_file);
    return -2; // Archivo no encontrado
}

// Encuentra un frame libre en el bitmap
int find_free_frame() {
    FILE* mem_file = open_mem("rb");
    fseek(mem_file, FRAME_BITMAP_START, SEEK_SET);

    unsigned char byte;
    for (int i = 0; i < FRAME_BITMAP_SIZE; i++) {
        fread(&byte, 1, 1, mem_file);
        for (int bit = 0; bit < 8; bit++) {
            if ((byte & (1 << bit)) == 0) {
                // Frame libre encontrado
                fclose(mem_file);
                return (i * 8) + bit;
            }
        }
    }

    fclose(mem_file);
    return -1; // No hay frames libres
}

// Marcar un frame como usado o libre en el bitmap
void set_frame_state(int frame_number, bool used) {
    FILE* mem_file = open_mem("r+b");

    int byte_offset = frame_number / 8;
    int bit_position = frame_number % 8;

    fseek(mem_file, FRAME_BITMAP_START + byte_offset, SEEK_SET);
    unsigned char byte;
    fread(&byte, 1, 1, mem_file);

    if (used) {
        byte |= (1 << bit_position); // Marcar como usado
    } else {
        byte &= ~(1 << bit_position); // Marcar como libre
    }

    fseek(mem_file, FRAME_BITMAP_START + byte_offset, SEEK_SET);
    fwrite(&byte, 1, 1, mem_file);

    fclose(mem_file);
}

// Busca una entrada libre en la tabla de páginas invertida y la asigna
int assign_inverted_page_entry(int process_id, int vpn) {
    FILE* mem_file = open_mem("r+b");

    for (int i = 0; i < INVERTED_PT_ENTRIES; i++) {
        fseek(mem_file, INVERTED_PT_START + i * INVERTED_PT_ENTRY_SIZE, SEEK_SET);

        // Leer entrada de 3 bytes
        unsigned char entry[3];
        fread(entry, 1, 3, mem_file);

        // Verificar si la entrada está libre (bit de validez = 0)
        if ((entry[0] & 0x80) == 0) {
            // Crear nueva entrada
            // - 1 bit de validez (1)
            // - 10 bits para ID de proceso (2 bits no significativos)
            // - 13 bits para VPN (1 bit no significativo)

            unsigned char new_entry[3] = {0};

            // Bit de validez en 1
            new_entry[0] = 0x80;

            // Agregar los bits del ID de proceso (8 bits)
            new_entry[0] |= (process_id >> 2) & 0x3F;
            new_entry[1] = (process_id << 6) & 0xC0;

            // Agregar los bits del VPN (12 bits)
            new_entry[1] |= (vpn >> 6) & 0x3F;
            new_entry[2] = (vpn << 2) & 0xFC;

            // Escribir la nueva entrada
            fseek(mem_file, INVERTED_PT_START + i * INVERTED_PT_ENTRY_SIZE, SEEK_SET);
            fwrite(new_entry, 1, 3, mem_file);

            fclose(mem_file);
            return i; // Retorna el PFN
        }
    }

    fclose(mem_file);
    return -1; // No hay entradas libres
}

// Liberar una entrada de la tabla de páginas invertida
void free_inverted_page_entries(int process_id, int vpn) {
    FILE* mem_file = open_mem("r+b");

    for (int i = 0; i < INVERTED_PT_ENTRIES; i++) {
        fseek(mem_file, INVERTED_PT_START + i * INVERTED_PT_ENTRY_SIZE, SEEK_SET);

        // Leer entrada de 3 bytes
        unsigned char entry[3];
        fread(entry, 1, 3, mem_file);

        // Verificar si la entrada está válida
        if ((entry[0] & 0x80) != 0) {
            // Extraer PID y VPN
            int entry_pid = ((entry[0] & 0x3F) << 2) | ((entry[1] & 0xC0) >> 6);
            int entry_vpn = ((entry[1] & 0x3F) << 6) | ((entry[2] & 0xFC) >> 2);

            if (entry_pid == process_id && entry_vpn == vpn) {
                // Invalidar la entrada
                entry[0] &= 0x7F;
                fseek(mem_file, INVERTED_PT_START + i * INVERTED_PT_ENTRY_SIZE, SEEK_SET);
                fwrite(entry, 1, 3, mem_file);

                // Liberar el frame correspondiente
                set_frame_state(i, false);
                break;
            }
        }
    }

    fclose(mem_file);
}

// Busca una entrada libre en la tabla de archivos del proceso
int find_free_file_entry(unsigned char* pcb_entry) {
    for (int j = 0; j < FILE_TABLE_ENTRIES; j++) {
        int file_offset = FILE_TABLE_REL_START + j * FILE_TABLE_ENTRY_SIZE;
        unsigned char valid = pcb_entry[file_offset];

        if (valid == 0x00) {
            return j;
        }
    }
    return -1; // No hay entradas libres
}

// Traducir dirección virtual a física
unsigned long translate_address(int process_id, unsigned int virtual_addr) {
    // Extraer el VPN y offset de la dirección virtual
    unsigned int vpn = (virtual_addr >> 15) & 0xFFF;  // 12 bits de VPN
    unsigned int offset = virtual_addr & 0x7FFF;      // 15 bits de offset

    FILE* mem_file = open_mem("rb");

    // Buscar en la tabla invertida la entrada que corresponde al par (PID, VPN)
    for (int i = 0; i < INVERTED_PT_ENTRIES; i++) {
        fseek(mem_file, INVERTED_PT_START + i * INVERTED_PT_ENTRY_SIZE, SEEK_SET);

        unsigned char entry[3];
        fread(entry, 1, 3, mem_file);

        // Verificar si la entrada está válida
        if ((entry[0] & 0x80) != 0) {
            // Extraer PID y VPN de la entrada
            int entry_pid = ((entry[0] & 0x3F) << 2) | ((entry[1] & 0xC0) >> 6);
            int entry_vpn = ((entry[1] & 0x3F) << 6) | ((entry[2] & 0xFC) >> 2);

            if (entry_pid == process_id && entry_vpn == vpn) {
                // Encontrado! El índice i corresponde al PFN
                unsigned long physical_addr = FRAMES_START + (unsigned long)i * FRAMES_ENTRY_SIZE + offset;
                fclose(mem_file);
                return physical_addr;
            }
        }
    }

    fclose(mem_file);
    return 0; // No se encontró la traducción
}

osrmsFile* os_open(int process_id, char* file_name, char mode) {
    if (mode != 'r' && mode != 'w') {
        printf("Error: El modo de apertura debe ser 'r' o 'w'.\n");
        return NULL;
    }

    unsigned char pcb_entry[PCB_ENTRY_SIZE];
    int file_index = find_file_entry(process_id, file_name, pcb_entry);

    // Si es modo lectura, el archivo debe existir
    if (mode == 'r') {
        if (file_index < 0) {
            printf("Error: El archivo %s no existe en el proceso %d.\n", file_name, process_id);
            return NULL;
        }

        // Crear el descriptor de archivo
        osrmsFile* file_desc = (osrmsFile*)malloc(sizeof(osrmsFile));
        if (!file_desc) {
            printf("Error: No se pudo asignar memoria para el descriptor del archivo.\n");
            return NULL;
        }

        file_desc->process_id = process_id;
        strncpy(file_desc->file_name, file_name, 14);
        file_desc->file_name[14] = '\0';
        file_desc->file_entry_index = file_index;
        file_desc->mode = mode;
        file_desc->current_offset = 0;

        // Obtener tamaño y dirección virtual
        int file_offset = FILE_TABLE_REL_START + file_index * FILE_TABLE_ENTRY_SIZE;
        unsigned char fsize[5];
        unsigned char vaddr[4];

        memcpy(fsize, &pcb_entry[file_offset + 15], 5);
        memcpy(vaddr, &pcb_entry[file_offset + 20], 4);

        file_desc->file_size = 0;
        for (int i = 0; i < 5; i++) {
            file_desc->file_size |= ((unsigned long long)fsize[i]) << (8*i);
        }

        file_desc->virtual_addr = 0;
        for (int i = 0; i < 4; i++) {
            file_desc->virtual_addr |= ((unsigned int)vaddr[i]) << (8*i);
        }

        printf("Archivo %s abierto para lectura (proceso %d).\n", file_name, process_id);
        return file_desc;
    }
    // Modo escritura
    else {
        FILE* mem_file = open_mem("r+b");
        int pcb_index = -1;

        // Buscar el PCB del proceso
        for (int i = 0; i < PCB_ENTRIES; i++) {
            fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
            unsigned char state;
            unsigned char pid;

            fread(&state, 1, 1, mem_file);
            fseek(mem_file, 14, SEEK_CUR);
            fread(&pid, 1, 1, mem_file);

            if (state == 0x01 && pid == process_id) {
                pcb_index = i;
                break;
            }
        }

        if (pcb_index < 0) {
            fclose(mem_file);
            printf("Error: Proceso con ID %d no encontrado o no está en ejecución.\n", process_id);
            return NULL;
        }

        // Si el archivo ya existe, lo sobrescribiremos
        if (file_index >= 0) {
            // Liberar los frames asociados a este archivo
            int file_offset = FILE_TABLE_REL_START + file_index * FILE_TABLE_ENTRY_SIZE;
            unsigned char vaddr[4];
            memcpy(vaddr, &pcb_entry[file_offset + 20], 4);

            unsigned int virtual_addr = 0;
            for (int i = 0; i < 4; i++) {
                virtual_addr |= ((unsigned int)vaddr[i]) << (8*i);
            }

            unsigned int vpn = (virtual_addr >> 15) & 0xFFF;
            free_inverted_page_entries(process_id, vpn);

            // Crear el descriptor de archivo usando el mismo índice
            osrmsFile* file_desc = (osrmsFile*)malloc(sizeof(osrmsFile));
            if (!file_desc) {
                fclose(mem_file);
                printf("Error: No se pudo asignar memoria para el descriptor del archivo.\n");
                return NULL;
            }

            file_desc->process_id = process_id;
            strncpy(file_desc->file_name, file_name, 14);
            file_desc->file_name[14] = '\0';
            file_desc->file_entry_index = file_index;
            file_desc->file_size = 0; // El archivo se sobrescribe, comienza vacío
            file_desc->mode = mode;
            file_desc->current_offset = 0;

            // Generar nueva dirección virtual (conservamos el VPN anterior)
            file_desc->virtual_addr = virtual_addr;

            fclose(mem_file);
            printf("Archivo %s abierto para escritura (proceso %d) - Sobrescribiendo.\n", file_name, process_id);
            return file_desc;
        }
        // Si el archivo no existe, buscamos una entrada libre
        else {
            fseek(mem_file, PCB_START + pcb_index * PCB_ENTRY_SIZE, SEEK_SET);
            fread(pcb_entry, 1, PCB_ENTRY_SIZE, mem_file);

            int free_entry_index = find_free_file_entry(pcb_entry);
            if (free_entry_index < 0) {
                fclose(mem_file);
                printf("Error: No hay espacio disponible en la tabla de archivos del proceso %d.\n", process_id);
                return NULL;
            }

            // Crear el descriptor de archivo
            osrmsFile* file_desc = (osrmsFile*)malloc(sizeof(osrmsFile));
            if (!file_desc) {
                fclose(mem_file);
                printf("Error: No se pudo asignar memoria para el descriptor del archivo.\n");
                return NULL;
            }

            file_desc->process_id = process_id;
            strncpy(file_desc->file_name, file_name, 14);
            file_desc->file_name[14] = '\0';
            file_desc->file_entry_index = free_entry_index;
            file_desc->file_size = 0;
            file_desc->mode = mode;
            file_desc->current_offset = 0;

            // Generar nueva dirección virtual
            // Usando un valor probable de VPN (simplemente el índice + 1 para evitar VPN=0)
            unsigned int vpn = free_entry_index + 1;
            file_desc->virtual_addr = (vpn << 15); // Setear los bits de VPN y offset=0

            fclose(mem_file);
            printf("Archivo %s abierto para escritura (proceso %d) - Nuevo archivo.\n", file_name, process_id);
            return file_desc;
        }
    }
}

int os_read_file(osrmsFile* file_desc, char* dest) {
    if (!file_desc || file_desc->mode != 'r') {
        printf("Error: Descriptor de archivo inválido o no abierto para lectura.\n");
        return -1;
    }

    // Verificar que el archivo existe
    unsigned char pcb_entry[PCB_ENTRY_SIZE];
    int file_index = find_file_entry(file_desc->process_id, file_desc->file_name, pcb_entry);
    if (file_index < 0) {
        printf("Error: El archivo %s no existe en el proceso %d.\n", file_desc->file_name, file_desc->process_id);
        return -1;
    }

    // Abrir el archivo de destino
    FILE* dest_file = fopen(dest, "wb");
    if (!dest_file) {
        printf("Error: No se pudo abrir el archivo de destino %s.\n", dest);
        return -1;
    }

    FILE* mem_file = open_mem("rb");

    // Calcular dirección física inicial
    unsigned int virtual_addr = file_desc->virtual_addr;
    unsigned long physical_addr = translate_address(file_desc->process_id, virtual_addr);

    if (physical_addr == 0) {
        fclose(mem_file);
        fclose(dest_file);
        printf("Error: No se pudo traducir la dirección virtual.\n");
        return -1;
    }

    // Leer el archivo y escribirlo en destino
    const int buffer_size = 4096; // 4KB buffer
    unsigned char buffer[buffer_size];
    unsigned long bytes_left = file_desc->file_size;
    unsigned long bytes_read = 0;

    while (bytes_left > 0) {
        // Determinar cuántos bytes leer en esta iteración
        unsigned int to_read = (bytes_left > buffer_size) ? buffer_size : bytes_left;

        // Leer desde la memoria
        fseek(mem_file, physical_addr + bytes_read, SEEK_SET);
        unsigned int actually_read = fread(buffer, 1, to_read, mem_file);

        if (actually_read == 0) break; // Error o fin de archivo

        // Escribir en el destino
        fwrite(buffer, 1, actually_read, dest_file);

        bytes_read += actually_read;
        bytes_left -= actually_read;

        // Si cruzamos un límite de página, retraducir la dirección
        if ((file_desc->virtual_addr + bytes_read) / PAGE_SIZE !=
            (file_desc->virtual_addr + bytes_read - actually_read) / PAGE_SIZE) {
            physical_addr = translate_address(file_desc->process_id, file_desc->virtual_addr + bytes_read);
            if (physical_addr == 0) break; // Error de traducción
        }
    }

    fclose(mem_file);
    fclose(dest_file);

    printf("Archivo %s leído correctamente. %lu bytes copiados a %s.\n",
           file_desc->file_name, bytes_read, dest);

    return bytes_read;
}

int os_write_file(osrmsFile* file_desc, char* src) {
    if (!file_desc || file_desc->mode != 'w') {
        printf("Error: Descriptor de archivo inválido o no abierto para escritura.\n");
        return -1;
    }

    // Abrir el archivo fuente
    FILE* src_file = fopen(src, "rb");
    if (!src_file) {
        printf("Error: No se pudo abrir el archivo fuente %s.\n", src);
        return -1;
    }

    // Obtener el tamaño del archivo fuente
    fseek(src_file, 0, SEEK_END);
    unsigned long src_size = ftell(src_file);
    fseek(src_file, 0, SEEK_SET);

    // Calcular cuántas páginas necesitamos
    unsigned int pages_needed = (src_size + PAGE_SIZE - 1) / PAGE_SIZE;

    // Extraer el VPN de la dirección virtual
    unsigned int vpn = (file_desc->virtual_addr >> 15) & 0xFFF;

    // Para cada página, necesitamos asignar un frame
    unsigned long bytes_written = 0;
    unsigned int current_page = 0;

    while (bytes_written < src_size && current_page < pages_needed) {
        // Encontrar un frame libre
        int pfn = find_free_frame();
        if (pfn < 0) {
            printf("Error: No hay frames libres disponibles.\n");
            fclose(src_file);
            return bytes_written;
        }

        // Asignar la entrada en la tabla de páginas invertida
        assign_inverted_page_entry(file_desc->process_id, vpn + current_page);

        // Marcar el frame como usado
        set_frame_state(pfn, true);

        // Calcular la dirección física del frame
        unsigned long physical_addr = FRAMES_START + (unsigned long)pfn * FRAMES_ENTRY_SIZE;

        // Determinar cuántos bytes escribir en este frame
        unsigned int bytes_to_write = PAGE_SIZE;
        if (bytes_written + bytes_to_write > src_size) {
            bytes_to_write = src_size - bytes_written;
        }

        // Leer del archivo fuente
        unsigned char buffer[PAGE_SIZE];
        unsigned int actually_read = fread(buffer, 1, bytes_to_write, src_file);

        if (actually_read == 0) break; // Error o fin de archivo

        // Escribir en la memoria
        FILE* mem_file = open_mem("r+b");
        fseek(mem_file, physical_addr, SEEK_SET);
        fwrite(buffer, 1, actually_read, mem_file);
        fclose(mem_file);

        bytes_written += actually_read;
        current_page++;
    }

    fclose(src_file);

    // Actualizar la entrada en la tabla de archivos
    FILE* mem_file = open_mem("r+b");

    // Buscar el PCB del proceso
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char state;
        unsigned char pid;

        fread(&state, 1, 1, mem_file);
        fseek(mem_file, 14, SEEK_CUR);
        fread(&pid, 1, 1, mem_file);

        if (state == 0x01 && pid == file_desc->process_id) {
            // Encontrado el PCB, actualizar la entrada del archivo
            int file_offset = FILE_TABLE_REL_START + file_desc->file_entry_index * FILE_TABLE_ENTRY_SIZE;

            // Escribir flag de validez
            fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE + file_offset, SEEK_SET);
            unsigned char valid = 0x01;
            fwrite(&valid, 1, 1, mem_file);

            // Escribir nombre del archivo
            char name[14] = {0};
            strncpy(name, file_desc->file_name, 14);
            fwrite(name, 1, 14, mem_file);

            // Escribir tamaño del archivo (5 bytes, little endian)
            for (int j = 0; j < 5; j++) {
                unsigned char size_byte = (bytes_written >> (j * 8)) & 0xFF;
                fwrite(&size_byte, 1, 1, mem_file);
            }

            // Escribir dirección virtual (4 bytes, little endian)
            for (int j = 0; j < 4; j++) {
                unsigned char addr_byte = (file_desc->virtual_addr >> (j * 8)) & 0xFF;
                fwrite(&addr_byte, 1, 1, mem_file);
            }

            break;
        }
    }

    fclose(mem_file);

    // Actualizar el tamaño del archivo en el descriptor
    file_desc->file_size = bytes_written;

    printf("Archivo %s escrito correctamente. %lu bytes copiados desde %s.\n",
           file_desc->file_name, bytes_written, src);

    return bytes_written;
}

void os_delete_file(int process_id, char* file_name) {
    unsigned char pcb_entry[PCB_ENTRY_SIZE];
    int file_index = find_file_entry(process_id, file_name, pcb_entry);

    if (file_index < 0) {
        printf("Error: No se encontró el archivo %s en el proceso %d.\n", file_name, process_id);
        return;
    }

    // Obtener la dirección virtual y liberar las páginas
    int file_offset = FILE_TABLE_REL_START + file_index * FILE_TABLE_ENTRY_SIZE;
    unsigned char vaddr[4];
    memcpy(vaddr, &pcb_entry[file_offset + 20], 4);

    unsigned int virtual_addr = 0;
    for (int i = 0; i < 4; i++) {
        virtual_addr |= ((unsigned int)vaddr[i]) << (8*i);
    }

    // Obtener el tamaño del archivo
    unsigned char fsize[5];
    memcpy(fsize, &pcb_entry[file_offset + 15], 5);

    unsigned long long file_size = 0;
    for (int i = 0; i < 5; i++) {
        file_size |= ((unsigned long long)fsize[i]) << (8*i);
    }

    // Calcular cuántas páginas ocupa el archivo
    unsigned int pages_used = (file_size + PAGE_SIZE - 1) / PAGE_SIZE;

    // Extraer el VPN base
    unsigned int vpn_base = (virtual_addr >> 15) & 0xFFF;

    // Liberar cada página utilizada por el archivo
    for (unsigned int i = 0; i < pages_used; i++) {
        free_inverted_page_entries(process_id, vpn_base + i);
    }

    // Marcar la entrada del archivo como no válida
    FILE* mem_file = open_mem("r+b");

    // Buscar el PCB del proceso
    for (int i = 0; i < PCB_ENTRIES; i++) {
        fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE, SEEK_SET);
        unsigned char state;
        unsigned char pid;

        fread(&state, 1, 1, mem_file);
        fseek(mem_file, 14, SEEK_CUR);
        fread(&pid, 1, 1, mem_file);

        if (state == 0x01 && pid == process_id) {
            // Invalidar la entrada del archivo
            fseek(mem_file, PCB_START + i * PCB_ENTRY_SIZE + FILE_TABLE_REL_START + file_index * FILE_TABLE_ENTRY_SIZE, SEEK_SET);
            unsigned char invalid = 0x00;
            fwrite(&invalid, 1, 1, mem_file);
            break;
        }
    }

    fclose(mem_file);
    printf("Archivo %s eliminado correctamente del proceso %d.\n", file_name, process_id);
}

void os_close(osrmsFile* file_desc) {
    if (file_desc != NULL) {
        printf("Cerrando archivo %s del proceso %d.\n", file_desc->file_name, file_desc->process_id);
        free(file_desc);
    }
}

int os_cp(int pid_src, char* fname_src, int pid_dst, char* fname_dst) {
    // Verificar que el archivo fuente existe
    if (!os_exists(pid_src, fname_src)) {
        printf("Error: El archivo fuente %s no existe en el proceso %d.\n", fname_src, pid_src);
        return -1;
    }

    // Abrir el archivo fuente para lectura
    osrmsFile* src_file = os_open(pid_src, fname_src, 'r');
    if (!src_file) {
        return -1;
    }

    // Abrir o crear el archivo destino para escritura
    osrmsFile* dst_file = os_open(pid_dst, fname_dst, 'w');
    if (!dst_file) {
        os_close(src_file);
        return -1;
    }

    // Crear un archivo temporal en el sistema para la transferencia
    char temp_file[] = "temp_transfer_XXXXXX";
    int fd = mkstemp(temp_file);
    if (fd == -1) {
        printf("Error: No se pudo crear archivo temporal.\n");
        os_close(src_file);
        os_close(dst_file);
        return -1;
    }
    close(fd);

    // Leer del archivo fuente al archivo temporal
    if (os_read_file(src_file, temp_file) < 0) {
        printf("Error en la lectura del archivo fuente.\n");
        os_close(src_file);
        os_close(dst_file);
        remove(temp_file);
        return -1;
    }

    // Escribir del archivo temporal al archivo destino
    int bytes = os_write_file(dst_file, temp_file);

    // Cerrar los archivos
    os_close(src_file);
    os_close(dst_file);

    // Eliminar el archivo temporal
    remove(temp_file);

    if (bytes < 0) {
        printf("Error en la escritura del archivo destino.\n");
        return -1;
    }

    printf("Archivo copiado correctamente de proceso %d:%s a proceso %d:%s. %d bytes transferidos.\n",
           pid_src, fname_src, pid_dst, fname_dst, bytes);

    return bytes;
}
