#include "../../osms_API/osms_API.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <ruta_a_memoria>\n", argv[0]);
        return 1;
    }

    printf("===== PROBANDO FUNCIONES GENERALES CON MEMFILLED.BIN =====\n\n");

    // Montar la memoria
    printf("----- Montando memoria -----\n");
    os_mount((char *)argv[1]);

    // Mostrar procesos existentes
    printf("----- Procesos existentes -----\n");
    os_ls_processes();

    // Verificar existencia de archivos en procesos
    printf("\n----- Verificando existencia de archivos -----\n");
    int exists1 = os_exists(6, "log.txt");
    printf("Archivo 'log.txt' en proceso 6: %s\n", exists1 ? "Existe" : "No existe");
    int exists2 = os_exists(7, "dolphin.md");
    printf("Archivo 'dolphin.md' en proceso 7: %s\n", exists2 ? "Existe" : "No existe");
    int exists3 = os_exists(6, "archivo_inexistente.txt");
    printf("Archivo 'archivo_inexistente.txt' en proceso 6: %s\n", exists3 ? "Existe" : "No existe");

    // Listar archivos de procesos existentes
    printf("\n----- Archivos del proceso 6 -----\n");
    os_ls_files(6);
    printf("\n----- Archivos del proceso 7 -----\n");
    os_ls_files(7);

    // Estado del bitmap de frames
    printf("\n----- Estado del bitmap de frames -----\n");
    os_frame_bitmap();

    printf("\n----- Pruebas de funciones generales completadas -----\n");

    return 0;
}
