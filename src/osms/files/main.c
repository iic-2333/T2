#include "../../osms_API/osms_API.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <ruta_a_memoria>\n", argv[0]);
        return 1;
    }

    printf("===== PROBANDO FUNCIONES DE ARCHIVOS CON MEMFORMAT.BIN =====\n\n");

    // Montar la memoria
    printf("----- Montando memoria -----\n");
    os_mount((char *)argv[1]);

    // Crear procesos para las pruebas
    printf("\n----- Creando procesos para pruebas de archivos -----\n");
    int res1 = os_start_process(10, "docs");
    printf("Proceso 10 (docs) creado: %s\n", res1 ? "Éxito" : "Falló");
    int res2 = os_start_process(20, "media");
    printf("Proceso 20 (media) creado: %s\n", res2 ? "Éxito" : "Falló");

    // Crear y escribir en un nuevo archivo (os_open y os_write_file)
    printf("\n----- Probando os_open (modo escritura) y os_write_file -----\n");
    osrmsFile* f1 = os_open(10, "document.txt", 'w');
    if (f1 != NULL) {
        printf("Archivo 'document.txt' abierto en modo escritura\n");
        int write_result = os_write_file(f1, "Archivos/symbol.txt");
        printf("Resultado de escritura: %s (bytes escritos: %d)\n",
               write_result > 0 ? "Éxito" : "Falló", write_result);
        os_close(f1);
        printf("Archivo cerrado\n");
    } else {
        printf("Error al abrir el archivo 'document.txt' en modo escritura\n");
    }

    // Abrir el mismo archivo en modo lectura (os_open y os_read_file)
    printf("\n----- Probando os_open (modo lectura) y os_read_file -----\n");
    osrmsFile* f2 = os_open(10, "document.txt", 'r');
    if (f2 != NULL) {
        printf("Archivo 'document.txt' abierto en modo lectura\n");
        int read_result = os_read_file(f2, "document_leido.txt");
        printf("Resultado de lectura: %s (bytes leídos: %d)\n",
               read_result > 0 ? "Éxito" : "Falló", read_result);
        os_close(f2);
        printf("Archivo cerrado\n");
    } else {
        printf("Error al abrir el archivo 'document.txt' en modo lectura\n");
    }

    // Crear otro archivo para probar borrado
    printf("\n----- Creando archivo para prueba de borrado -----\n");
    osrmsFile* f3 = os_open(20, "borrar.txt", 'w');
    if (f3 != NULL) {
        os_write_file(f3, "Archivos/darth.txt");
        os_close(f3);
        printf("Archivo 'borrar.txt' creado en proceso 20\n");
    }

    // Listar archivos antes de borrar
    printf("\n----- Archivos del proceso 20 antes de borrar -----\n");
    os_ls_files(20);

    // Borrar archivo (os_delete_file)
    printf("\n----- Probando os_delete_file -----\n");
    os_delete_file(20, "borrar.txt");
    printf("Archivo 'borrar.txt' eliminado del proceso 20\n");

    // Listar archivos después de borrar
    printf("\n----- Archivos del proceso 20 después de borrar -----\n");
    os_ls_files(20);

    // Verificar funciones os_open con distintos modos
    printf("\n----- Probando os_open con diferentes modos -----\n");
    osrmsFile* f4 = os_open(10, "modo_r.txt", 'r');
    printf("Abrir archivo inexistente en modo lectura: %s\n", f4 != NULL ? "Éxito (no debería)" : "Falló (esperado)");

    osrmsFile* f5 = os_open(10, "modo_w.txt", 'w');
    printf("Abrir archivo inexistente en modo escritura: %s\n", f5 != NULL ? "Éxito" : "Falló");
    if (f5 != NULL) {
        os_write_file(f5, "Archivos/symbol.txt");
        os_close(f5);
    }

    printf("\n----- Pruebas de funciones de archivos completadas -----\n");

    return 0;
}
