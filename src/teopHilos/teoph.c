#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Definición de tipos de objetos
#define TB 0 // Tierra Baldía
#define OM 1 // Objetivo Militar
#define IC 2 // Infraestructura Civil

typedef struct 
{
   int type;               // 0: TB, 1: OM, 2: IC
   int resistance;         // resistencia actual (se actualizará)
   int initial_resistance; // resistencia inicial (para comparación final)
} Cell;

typedef struct
{
     int X;
     int Y;
     int RD;
     int PE;
} Drone;

typedef struct {
    int start, end;
    Drone *drones;
    Cell *grid;
    int N, M;
} ThreadData;

// Mutex para proteger el acceso a la cuadrícula
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Funcion que retorna el maximo de 2 numeros
static inline int max_int(int a, int b)
{
     return (a > b) ? a : b;
}

// Funcion que retorna el minimo de 2 numeros
static inline int min_int(int a, int b)
{
     return (a < b) ? a : b;
}

// Funcion que lee el tamaño de la cuadricula
void read_grid_size(FILE *f, int *N, int *M) 
{
    if (fscanf(f, "%d %d", N, M) != 2) 
    {
        fprintf(stderr, "Error al leer N y M.\n");
        exit(EXIT_FAILURE);
    }
}

// Funcion que inicializa la cuadricula
Cell *initialize_grid(int N, int M) 
{
    // Crear la cuadrícula (matriz de Cell) en el heap.
    int total_cells = N * M;
    Cell *grid = malloc(total_cells * sizeof(Cell));
    if (!grid) 
    {
        perror("Error al asignar memoria para la cuadrícula");
        exit(EXIT_FAILURE);
    }

    // Inicializar toda la cuadrícula como TB (tierra baldía)
    for (int i = 0; i < total_cells; i++) 
    {
        grid[i].type = TB;
        grid[i].resistance = 0;
        grid[i].initial_resistance = 0;
    }
    return grid;
}

// Funcion que lee los valores que iran en la cuadricula
void read_objects(FILE *f, Cell *grid, int N, int M, int *K) 
{
    // Leer el número de objetos OM/IC.
    if (fscanf(f, "%d", K) != 1) 
    {
        fprintf(stderr, "Error al leer K (número de objetos).\n");
        fclose(f);
        free(grid);
        exit(EXIT_FAILURE);
    }

    // Leer los K objetos; cada línea contiene: X Y resistencia.
    for (int i = 0; i < *K; i++) 
    {
        int x, y, res;
        if (fscanf(f, "%d %d %d", &x, &y, &res) != 3) 
        {
            fprintf(stderr, "Error al leer objeto %d.\n", i);
            fclose(f);
            free(grid);
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

// Funcion que lee el numero de drones que iran en la cuadricula
Drone *read_drones(FILE *f, int *L) 
{
    // Leer el número de drones.
    if (fscanf(f, "%d", L) != 1) 
    {
        fprintf(stderr, "Error al leer L (número de drones).\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    Drone *drones = malloc(*L * sizeof(Drone));
    if (!drones) 
    {
        perror("Error al asignar memoria para drones");
        fclose(f);
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

// Funcion que se encarga de manipular los hilos a usar en el programa
void *thread_worker(void *arg) 
{
    ThreadData *td = (ThreadData *)arg;
    for (int j = td->start; j < td->end; j++) 
    {
        int dX = td->drones[j].X;
        int dY = td->drones[j].Y;
        int RD = td->drones[j].RD;
        int PE = td->drones[j].PE;

        // Calcular límites del área afectada asegurando que queden dentro de la cuadrícula
        int x_start = max_int(0, dX - RD);
        int x_end = min_int(td->N - 1, dX + RD);
        int y_start = max_int(0, dY - RD);
        int y_end = min_int(td->M - 1, dY + RD);

        for (int x = x_start; x <= x_end; x++) 
        {
            for (int y = y_start; y <= y_end; y++) 
            {
                int idx = x * td->M + y;
               
                // Bloquear el mutex antes de actualizar la resistencia
                pthread_mutex_lock(&mutex);
               
                // Actualizar únicamente si hay un objeto OM o IC.
                if (td->grid[idx].type == OM) 
                {
                    // Para OM se suma PE; dado que la resistencia es negativa,
                    // si llega a 0 o se vuelve positiva, el objeto se considera destruido.
                    td->grid[idx].resistance += PE;
                } 
                else if (td->grid[idx].type == IC) 
                {
                    // Para IC se resta PE.
                    td->grid[idx].resistance -= PE;
                }
                // Desbloquear el mutex después de actualizar la resistencia
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    return NULL;
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
    
    // Convertir el argumento <n> a entero y validarlo.
    int n = atoi(argv[1]);
    
    if (n <= 0) 
    {
        fprintf(stderr, "Error: <n> debe ser un entero positivo.\n");
        exit(EXIT_FAILURE);
    }
    
    // Abrir el archivo de instancia.
    FILE *f = fopen(argv[2], "r");
    if (!f) 
    {
        perror("Error al abrir el archivo de instancia");
        exit(EXIT_FAILURE);
    }
    
    int N, M, K, L;

    read_grid_size(f, &N, &M);
    Cell *grid = initialize_grid(N, M);
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
    
    pthread_t threads[n];
    ThreadData threadData[n];

    int chunk = L / n; 
    int rem = L % n;
    int start = 0;
    for (int i = 0; i < n; i++) 
    {
        int count = chunk + (i < rem ? 1 : 0);
        threadData[i] = (ThreadData){start, start + count, drones, grid, N, M};
        pthread_create(&threads[i], NULL, thread_worker, &threadData[i]);
        start += count;
    }

    // Esperar a que terminen todos los hilos.
    for (int i = 0; i < n; i++)
    {
          pthread_join(threads[i], NULL);
    } 
    
    printf("Valores leídos:\n");
    printf("N = %d, M = %d\n", N, M);
    printf("K = %d (objetos OM/IC)\n", K);
    printf("L = %d (drones)\n", L);
    printf("n final (hilos) = %d\n\n\n", n);

    // Imprimir los resultados.
    print_results(grid, N * M);
    
    // Liberar recursos.
    free(drones);
    free(grid);

    // Destruir el mutex
    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}