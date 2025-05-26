# PONTIFICIA UNIVERSIDAD CATÓLICA DE CHILE

ESCUELA DE INGENIERÍA
DEPARTAMENTO DE CIENCIA DE LA COMPUTACIÓN

## IIC2333 — Sistemas Operativos y Redes — 1/2025

### Tarea 2

**Miércoles 07 de Mayo**
**Fecha de Entrega:** Lunes 26 de Mayo a las 21:00
**Composición:** Grupos de 2 personas

---

### Objetivo

Implementar una API para manejar el contenido de una memoria principal.

### Contextualización

El proyecto consiste en desarrollar una API para escribir archivos en una memoria principal organizada en frames. La API deberá gestionar procesos simulados (iniciarlos, asignar memoria física, liberar memoria y eliminarlos), cada uno con su propia memoria virtual paginada. Estos procesos incluirán archivos asociados que representarán sus segmentos. Dichos archivos tendrán una dirección virtual en la memoria virtual del proceso, la cual se convertirá a direcciones físicas utilizando una tabla de páginas invertida.

### Introducción

La paginación es un mecanismo que nos permite eliminar la fragmentación externa de la memoria y consiste en dividir la memoria virtual de un proceso en porciones de igual tamaño llamadas páginas. Mientras que la memoria física es separada en porciones de igual tamaño llamados frames.

En este proyecto tendrán la posibilidad de experimentar con una implementación de un mecanismo de paginación simplificada sobre una memoria principal la cual será simulada por un archivo real. Deberán leer y modificar el contenido de esta memoria mediante una API desarrollada por ustedes. Se recomienda para este proyecto que vean los videos de segmentación, paginación y tabla de páginas invertida (enlaces más abajo).

---

## Estructura de memoria **osrms**

La memoria física está representada por un archivo real del sistema que está organizado de la siguiente manera:

* Se divide en segmentos de tamaños fijos en el siguiente orden:

  * 8 KB
  * 192 KB
  * 8 KB
  * 2 GB
* El tamaño total de la memoria es la suma de todos estos segmentos.

1. Los primeros 8 KB de la memoria corresponden a espacio reservado exclusivamente para la **Tabla de PCBs**.
2. Los siguientes 192 KB están destinados a la **Tabla Invertida de Páginas**.
3. Los siguientes 8 KB corresponden a espacio reservado exclusivamente para el **Frame Bitmap**.
4. Los últimos 2 GB de memoria están divididos en conjuntos de Bytes denominados **frames**:

   * **Tamaño de frame:** 32 KB. La memoria contiene un total de 2¹⁶ frames.
   * Cada frame posee un número secuencial (PFN) que puede almacenarse en 2 bytes (unsigned int).

Además, cada proceso cuenta con un espacio virtual de memoria igual a 128 MB. Esta memoria virtual está dividida en páginas de 32 KB.

### Tabla de PCBs

Se encuentra al inicio de la memoria y contiene información sobre los procesos. Está separada en 32 entradas, con la siguiente estructura:

* **Tamaño de entrada:** 256 Bytes
* **Cantidad de entradas:** 32
* **1 Byte** de estado: `0x01` si el proceso existe, `0x00` en caso contrario
* **14 Bytes** para indicar el nombre del proceso
* **1 Byte** para indicar el ID del proceso
* **240 Bytes** para una **Tabla de Archivos**

#### Tabla de Archivos

Contiene la información de los archivos presentes en la memoria virtual de un proceso. Está separada en 10 entradas, donde cada entrada cumple con el siguiente formato:

* **1 Byte** de validez: `0x01` si la entrada es válida, `0x00` en caso contrario
* **14 Bytes** para nombre del archivo
* **5 Bytes** para tamaño del archivo
* **4 Bytes** para la dirección virtual, estructurados como:

  * 5 bits no significativos (0b00000)
  * 12 bits VPN
  * 15 bits offset

### Tabla Invertida de Páginas

Contiene la información para traducir direcciones virtuales a físicas dentro de la memoria. Las direcciones físicas resultantes serán relativas al último segmento de 2 GB.

* **Tamaño de entrada:** 24 bits
* **Cantidad de entradas:** 65 536
* Cada entrada tiene:

  * 1 bit de validez (1 = válida)
  * 10 bits para ID de proceso (primeros 2 bits no significativos)
  * 13 bits para VPN (primer bit no significativo)

Para calcular una dirección física a partir de una dirección virtual:

1. Extraer 12 bits de VPN y 15 bits de offset de la dirección virtual.
2. Buscar en la tabla invertida la entrada que coincida con el par (ID de proceso, VPN). El índice de la entrada corresponderá al PFN.
3. La dirección física es `PFN * tamaño_frame + offset`.

> **Nota:** El número de frame (PFN) no está almacenado en la memoria física.

### Frame Bitmap

Cuenta con un tamaño de 8 KB (65 536 bits). Cada bit indica si un frame está libre (`0`) o usado (`1`).

* El Frame bitmap contiene un bit por cada frame de la memoria.
* Debe reflejar el estado actual de la memoria principal.

### Frames

Zona donde se almacenan los datos de los archivos (últimos 2 GB).

> **Endianness:** Lectura y escritura deben realizarse en orden **little endian** para todos los valores mayores a un byte.

---

## API de **osrms**

Implementar en C una biblioteca con interfaz en `osrms_API.h` y definición en `osrms_API.c`. Definir también un `struct osrmsFile` que represente un archivo abierto.

### Funciones generales

```c
void os_mount(char* memory_path);
void os_ls_processes();
int os_exists(int process_id, char* file_name);
void os_ls_files(int process_id);
void os_frame_bitmap();
```

### Funciones para procesos

```c
int os_start_process(int process_id, char* process_name);
int os_finish_process(int process_id);
int os_rename_process(int process_id, char* new_name);
```

### Funciones para archivos

```c
osrmsFile* os_open(int process_id, char* file_name, char mode);
int os_read_file(osrmsFile* file_desc, char* dest);
int os_write_file(osrmsFile* file_desc, char* src);
void os_delete_file(int process_id, char* file_name);
void os_close(osrmsFile* file_desc);
```

### Funciones bonus

```c
int os_cp(int pid_src, char* fname_src, int pid_dst, char* fname_dst);
```

---

## Ejecución

Compilar con `make` para generar ejecutable `osrms`. Ejecutar:

```
./osrms mem.bin
```

Ejemplo de secuencia en `main.c`:

```c
os_mount(argv[1]);
// ... otras llamadas a OSRMS API
```

Se entregarán dos archivos de memoria en Canvas:

* `memformat.bin`: memoria formateada
* `memfilled.bin`: memoria con procesos y archivos

---

## Formalidades

* La entrega se hace en el buzón de Canvas, incluyendo código fuente y `Makefile`.
* Solo en parejas. Lenguaje: C. Sin binarios ni repositorio Git.
* El programa debe compilar con `make` y generar `osrms`.
* Descuentos por incumplimientos de formato, compilación, fugas de memoria (valgrind), etc.

### Política de atraso

Se puede entregar con hasta 2 días hábiles de atraso. La nota máxima se ajusta según la fórmula:

```
NAtraso = min(NT1, 7.0 - 0.75 * d)
```

---

## Evaluación

* **6.0 pts**: Funciones de biblioteca:

  * 0.2 pts: `os_mount`
  * 0.6 pts: `os_ls_processes`
  * 0.6 pts: `os_exists`
  * 0.6 pts: `os_ls_files`
  * 0.6 pts: `os_frame_bitmap`
  * 0.3 pts: `os_start_process`
  * 0.3 pts: `os_finish_process`
  * 0.2 pts: `os_rename_process`
  * 0.3 pts: `os_open`
  * 0.3 pts: `os_close`
  * 0.6 pts: `os_write_file`
  * 0.6 pts: `os_read_file`
  * 0.8 pts: `os_delete_file`
* **1.0 pt (Bonus):** `os_cp`
* Descuento de hasta 1.0 pts si Valgrind reporta leaks o errores.

---

## Corrección

1. Preparar uno o más `main.c` que ejerzan todas las funciones implementadas.
2. Subir en Canvas `main.c` y archivos necesarios.
3. Se revisará en Google Meets demostrando el funcionamiento.
4. Se pueden usar archivos de evidencias (gif, audio, etc.).
5. Se recomienda más de un `main.c` para distintas funcionalidades.

---

## Observaciones

* La primera función a usar siempre será `os_mount`.
* No es necesario compaction ni defragmentación de memoria.
* Al eliminar, basta con marcar bits de validez en 0.
* Si un archivo no cabe completamente, detener la escritura (no eliminarlo).
* Detalles no especificados pueden asumirse en el README.

---

## Dudas

Cualquier duda a través del foro oficial: [https://github.com/IIC2333/Foro-2025-1](https://github.com/IIC2333/Foro-2025-1)

> **Nota:** 48 horas antes de la hora de entrega se dejarán de responder dudas.
