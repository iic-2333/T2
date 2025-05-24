#pragma once
#include "../osms_File/osms_File.h"
#include <stdio.h>

#define PCB_START 0
#define PCB_SIZE 8192
#define PCB_ENTRIES 32
#define PCB_ENTRY_SIZE 256
#define FILE_TABLE_REL_START 16
#define FILE_TABLE_SIZE 240
#define FILE_TABLE_ENTRIES 10
#define FILE_TABLE_ENTRY_SIZE 24 
#define INVERTED_PT_START 8192
#define INVERTED_PT_SIZE (192*1024)
#define INVERTED_PT_ENTRIES 65536
#define INVERTED_PT_ENTRY_SIZE 3
#define FRAME_BITMAP_START (200*1024)
#define FRAME_BITMAP_SIZE (8*1024)
#define FRAME_BITMAP_ENTRIES 8192
#define FRAME_BITMAP_ENTRY_SIZE 1
#define FRAMES_START (208*1024)
#define FRAMES_SIZE (2*1024*1024*1024)
#define FRAMES_ENTRIES 65536
#define FRAMES_ENTRY_SIZE 32768
#define VIRTUAL_MEM_SIZE (128*1024*1024)
#define PAGE_SIZE 32768
#define PAGE_COUNT (VIRTUAL_MEM_SIZE/PAGE_SIZE)

// funciones generales
void os_mount(char* memory_path);
void os_ls_processes();
int os_exists(int process_id, char* file_name);
void os_ls_files(int process_id);
void os_frame_bitmap();

// funciones procesos
int os_start_process(int process_id, char* process_name);
int os_finish_process(int process_id);
int os_rename_process(int process_id, char* new_name);

