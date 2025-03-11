#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>

// Definición de tipos de objetos:
#define TB 0 // Tierra Baldía
#define OM 1 // Objetivo Militar (resistencia negativa)
#define IC 2 // Infraestructura Civil (resistencia positiva)

typedef struct
{
    int type;               // 0: TB, 1: OM, 2: IC
    int resistance;         // resistencia actual (esta variable es la que se actualizará)
    int initial_resistance; // resistencia inicial (para clasificación final)
} Cell;

typedef struct
{
    int X;
    int Y;
    int RD;
    int PE;
} Drone;

// Función auxiliar para calcular el máximo entre dos enteros.
static inline int max_int(int a, int b)
{
    return (a > b) ? a : b;
}

// Función auxiliar para calcular el mínimo entre dos enteros.
static inline int min_int(int a, int b)
{
    return (a < b) ? a : b;
}

// Funcion para leer el tamaño de la grilla
void read_grid_size(FILE *f, int *N, int *M)
{
    if (fscanf(f, "%d %d", N, M) != 2)
    {
        fprintf(stderr, "Error al leer N y M.\n");
        exit(EXIT_FAILURE);
    }
}

// Funcion para inicializar la grilla
Cell *initialize_grid(int N, int M, pthread_mutex_t **mutex)
{
    size_t grid_size = N * M * sizeof(Cell);
    Cell *grid = mmap(NULL, grid_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (grid == MAP_FAILED)
    {
        perror("Error al crear memoria compartida para la cuadrícula");
        exit(EXIT_FAILURE);
    }

    // Crear un mutex en memoria compartida
    *mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (*mutex == MAP_FAILED)
    {
        perror("Error al crear memoria compartida para el mutex");
        munmap(grid, grid_size);
        exit(EXIT_FAILURE);
    }

    // Inicializar el mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(*mutex, &attr);

    // Inicializar la cuadrícula
    for (int i = 0; i < N * M; i++)
    {
        grid[i].type = TB;
        grid[i].resistance = 0;
        grid[i].initial_resistance = 0;
    }
    return grid;
}

// Funcion para leer los objeto de la entrada
void read_objects(FILE *f, Cell *grid, int N, int M, int *K)
{
    if (fscanf(f, "%d", K) != 1)
    {
        fprintf(stderr, "Error al leer K (número de objetos).\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < *K; i++)
    {
        int x, y, res;
        if (fscanf(f, "%d %d %d", &x, &y, &res) != 3)
        {
            fprintf(stderr, "Error al leer objeto %d.\n", i);
            exit(EXIT_FAILURE);
        }
        if (x < 0 || x >= N || y < 0 || y >= M)
        {
            fprintf(stderr, "Advertencia: Coordenadas fuera de rango para objeto %d.\n", i);
            continue;
        }
        int index = x * M + y;
        if (grid[index].type != TB)
        {
            fprintf(stderr, "Error: La celda (%d, %d) ya contiene un objeto.\n", x, y);
            continue;
        }
        grid[index].type = (res < 0) ? OM : IC;
        grid[index].resistance = res;
        grid[index].initial_resistance = res;
    }
}

// Funcion para leer el numero de drones
Drone *read_drones(FILE *f, int *L)
{
    if (fscanf(f, "%d", L) != 1)
    {
        fprintf(stderr, "Error al leer L (número de drones).\n");
        exit(EXIT_FAILURE);
    }

    // Reservar memoria para el arreglo de drones (este arreglo no necesita ser compartido).
    Drone *drones = malloc(*L * sizeof(Drone));
    if (!drones)
    {
        perror("Error al asignar memoria para drones");
        exit(EXIT_FAILURE);
    }

    // Leer la información de cada dron: X, Y, RD, PE.
    for (int i = 0; i < *L; i++)
    {
        if (fscanf(f, "%d %d %d %d", &drones[i].X, &drones[i].Y, &drones[i].RD, &drones[i].PE) != 4)
        {
            fprintf(stderr, "Error al leer dron %d.\n", i);
            exit(EXIT_FAILURE);
        }
    }
    return drones;
}

// Funcion para procesar los drones
void process_drones(int start, int end, Drone *drones, Cell *grid, int N, int M, pthread_mutex_t *mutex)
{
    for (int j = start; j < end; j++)
    {
        int dX = drones[j].X, dY = drones[j].Y;
        int RD = drones[j].RD, PE = drones[j].PE;

        // Calcular límites del área afectada por el dron.
        int x_start = max_int(0, dX - RD);
        int x_end = min_int(N - 1, dX + RD);
        int y_start = max_int(0, dY - RD);
        int y_end = min_int(M - 1, dY + RD);

        for (int x = x_start; x <= x_end; x++)
        {
            for (int y = y_start; y <= y_end; y++)
            {
                int idx = x * M + y;
                // Bloquear el mutex antes de actualizar la resistencia
                pthread_mutex_lock(mutex);
                // Solo se actualiza si hay un objeto OM o IC.
                if (grid[idx].type == OM)
                {
                    grid[idx].resistance += PE;
                }
                else if (grid[idx].type == IC)
                {
                    grid[idx].resistance -= PE;
                }
                // Desbloquear el mutex después de actualizar la resistencia
                pthread_mutex_unlock(mutex);
            }
        }
    }
}

// Funcion para imprimir los ressultados del programa
void print_results(Cell *grid, int total_cells)
{
    // Clasificar los objetos OM e IC según su estado.
    int OM_intact = 0, OM_partial = 0, OM_destroyed = 0;
    int IC_intact = 0, IC_partial = 0, IC_destroyed = 0;

    // En este enfoque:
    // Para un OM (resistencia inicialmente negativa):
    //   - intacto si resistencia == initial
    //   - destruido si resistencia es 0 o se volvió positiva.
    //   - parcialmente destruido si sigue siendo negativa pero diferente del valor inicial.
    // Para un IC (resistencia inicialmente positiva):
    //   - intacto si resistencia == initial
    //   - destruido si resistencia es 0 o se volvió negativa.
    //   - parcialmente destruido si sigue siendo positiva pero diferente del valor inicial.

    for (int i = 0; i < total_cells; i++)
    {
        if (grid[i].type == OM)
        {
            if (grid[i].resistance == grid[i].initial_resistance)
                OM_intact++;
            else if (grid[i].resistance == 0 || grid[i].resistance > 0)
                OM_destroyed++;
            else
                OM_partial++;
        }
        else if (grid[i].type == IC)
        {
            if (grid[i].resistance == grid[i].initial_resistance)
                IC_intact++;
            else if (grid[i].resistance == 0 || grid[i].resistance < 0)
                IC_destroyed++;
            else
                IC_partial++;
        }
    }

    // Imprimir los resultados.
    printf("OM sin destruir: %d\n", OM_intact);
    printf("OM parcialmente destruidos: %d\n", OM_partial);
    printf("OM totalmente destruido: %d\n", OM_destroyed);
    printf("IC sin destruir: %d\n", IC_intact);
    printf("IC parcialmente destruidos: %d\n", IC_partial);
    printf("IC totalmente destruido: %d\n", IC_destroyed);
}

// Funcion principal del programa
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <n> <archivo_instancia>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);

    if (n <= 0)
    {
        fprintf(stderr, "Error: <n> debe ser un entero positivo.\n");
        exit(EXIT_FAILURE);
    }

    FILE *f = fopen(argv[2], "r");
    if (!f)
    {
        perror("Error al abrir el archivo de instancia");
        exit(EXIT_FAILURE);
    }

    int N, M, K, L;
    read_grid_size(f, &N, &M);

    // Inicializar la cuadrícula y el mutex en memoria compartida
    pthread_mutex_t *mutex;
    Cell *grid = initialize_grid(N, M, &mutex);

    read_objects(f, grid, N, M, &K);
    Drone *drones = read_drones(f, &L);
    fclose(f);

    // Ajustar n: n debe ser <= min(N*M, L)
    int nm = N * M;
    int limit = (nm < L) ? nm : L;
    if (n > limit)
    {
        n = limit;
    }

    printf("Valores leídos:\n");
    printf("N = %d, M = %d\n", N, M);
    printf("K = %d (objetos OM/IC)\n", K);
    printf("L = %d (drones)\n", L);
    printf("n final (procesos) = %d\n\n\n", n);

    int chunk = L / n, rem = L % n, start = 0;
    for (int i = 0; i < n; i++)
    {
        int count = chunk + (i < rem ? 1 : 0);
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            process_drones(start, start + count, drones, grid, N, M, mutex);
            exit(EXIT_SUCCESS);
        }
        start += count;
    }

    for (int i = 0; i < n; i++)
        wait(NULL);

    // Imprimir los resultados
    print_results(grid, N * M);

    // Liberar memoria.
    munmap(grid, N * M * sizeof(Cell));
    munmap(mutex, sizeof(pthread_mutex_t));
    free(drones);
    return EXIT_SUCCESS;
}