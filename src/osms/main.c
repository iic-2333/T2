#include "../osms_API/osms_API.h"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <ruta_a_memoria>\n", argv[0]);
        return 1;
    }

    //MEMFORMAT.BIN

    os_mount((char *)argv[1]);
    os_ls_processes();
    os_start_process(10, "ventas");
    os_start_process(20, "reportes");
    os_start_process(20, "finanzas");
    os_start_process(30, "finanzas");
    os_ls_processes();
    //osrmsFile* f = os_open(10, "log.txt", 'w');
    //os_write_file(f, "log_local.txt");
    //os_close(f);
    //f = os_open(10, "log.txt", 'r');
    //os_read_file(f, "log_copia.txt");
    //os_close(f);
    os_ls_files(10);
    os_exists(10, "log.txt");
    os_rename_process(20, "Recursos humanos");
    os_rename_process(20, "RRHH");
    //os_cp(10, "log.txt", 20, "reporte.txt");
    //os_frame_bitmap();
    //os_delete_file(10, "log.txt");
    os_finish_process(10);
    os_ls_processes();
    //os_frame_bitmap();
    //os_finish_process(20);
    

    //MEMFILLED.BIN
    /*/
    os_mount((char *)argv[1]);
    os_ls_processes();
    os_ls_files(6);
    os_ls_files(7);
    os_exists(6, "log.txt");
    os_exists(6, "cat.jpg");
    os_frame_bitmap();
    */

    free_all();

    return 0;
}