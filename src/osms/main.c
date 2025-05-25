#include "../osms_API/osms_API.h"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <ruta_a_memoria>\n", argv[0]);
        return 1;
    }
    os_mount((char *)argv[1]);
    os_start_process(10, "ventas");
    os_start_process(20, "reportes");
    os_ls_processes();
    //osrmsFile* f = os_open(10, "log.txt", 'w');
    //os_write_file(f, "log_local.txt");
    //os_close(f);
    //f = os_open(10, "log.txt", 'r');
    //os_read_file(f, "log_copia.txt");
    //os_close(f);
    os_ls_files(10);
    os_exists(10, "log.txt");
    //os_cp(10, "log.txt", 20, "reporte.txt");
    //os_frame_bitmap();
    //os_delete_file(10, "log.txt");
    //os_finish_process(10);
    //os_finish_process(20);
    return 0;
}