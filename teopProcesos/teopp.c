#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

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
          perror("Error al abrir archivo de instancia");
          exit(EXIT_FAILURE);
     }

     int N, M;
     // Leer N y M (tamaño de la cuadrícula: N filas x M columnas).
     if (fscanf(f, "%d %d", &N, &M) != 2)
     {
          fprintf(stderr, "Error al leer N y M.\n");
          fclose(f);
          exit(EXIT_FAILURE);
     }

     // Crear la memoria compartida para la cuadrícula (grid).
     size_t grid_size = N * M * sizeof(Cell);
     Cell *grid = mmap(NULL, grid_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
     if (grid == MAP_FAILED)
     {
          perror("Error al crear memoria compartida para la cuadrícula");
          fclose(f);
          exit(EXIT_FAILURE);
     }

     // Inicializar toda la cuadrícula como TB (tierra baldía: type = 0, resistencia = 0).
     for (int i = 0; i < N * M; i++)
     {
          grid[i].type = TB;
          grid[i].resistance = 0;
          grid[i].initial_resistance = 0;
     }

     int K;
     // Leer el número de objetos OM/IC.
     if (fscanf(f, "%d", &K) != 1)
     {
          fprintf(stderr, "Error al leer K (número de objetos).\n");
          fclose(f);
          munmap(grid, grid_size);
          exit(EXIT_FAILURE);
     }

     // Leer los K objetos; cada línea contiene: X Y resistencia.
     // Se supone que si la resistencia es negativa → OM; si es positiva → IC.
     for (int i = 0; i < K; i++)
     {
          int x, y, res;
          if (fscanf(f, "%d %d %d", &x, &y, &res) != 3)
          {
               fprintf(stderr, "Error al leer objeto %d.\n", i);
               fclose(f);
               munmap(grid, grid_size);
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
          if (res < 0)
          {
               grid[index].type = OM;
          }
          else if (res > 0)
          {
               grid[index].type = IC;
          }
          grid[index].resistance = res;
          grid[index].initial_resistance = res;
     }

     int L;
     // Leer el número de drones.
     if (fscanf(f, "%d", &L) != 1)
     {
          fprintf(stderr, "Error al leer L (número de drones).\n");
          fclose(f);
          munmap(grid, grid_size);
          exit(EXIT_FAILURE);
     }

     // Reservar memoria para el arreglo de drones (este arreglo no necesita ser compartido).
     Drone *drones = malloc(L * sizeof(Drone));
     if (drones == NULL)
     {
          perror("Error al asignar memoria para drones");
          fclose(f);
          munmap(grid, grid_size);
          exit(EXIT_FAILURE);
     }

     // Leer la información de cada dron: X, Y, RD, PE.
     for (int i = 0; i < L; i++)
     {
          int X, Y, RD, PE;
          if (fscanf(f, "%d %d %d %d", &X, &Y, &RD, &PE) != 4)
          {
               fprintf(stderr, "Error al leer dron %d.\n", i);
               fclose(f);
               free(drones);
               munmap(grid, grid_size);
               exit(EXIT_FAILURE);
          }
          drones[i].X = X;
          drones[i].Y = Y;
          drones[i].RD = RD;
          drones[i].PE = PE;
     }
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

     // Particionar el arreglo de drones entre los n procesos.
     int chunk = L / n;
     int rem = L % n;
     int start = 0;
     pid_t pid;

     for (int i = 0; i < n; i++)
     {
          int count = chunk + (i < rem ? 1 : 0);
          int end = start + count;
          // Crear un proceso hijo para procesar los drones en el rango [start, end).
          pid = fork();
          if (pid < 0)
          {
               perror("Error en fork");
               exit(EXIT_FAILURE);
          }
          else if (pid == 0)
          { // Proceso hijo
               // Cada hijo procesa los drones en el rango [start, end).
               for (int j = start; j < end; j++)
               {
                    int dX = drones[j].X;
                    int dY = drones[j].Y;
                    int RD = drones[j].RD;
                    int PE = drones[j].PE;
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
                              // Solo se actualiza si hay un objeto OM o IC.
                              if (grid[idx].type == OM)
                              {
                                   // Para OM se suma el PE (recordar que res es negativo).
                                   __sync_fetch_and_add(&grid[idx].resistance, PE);
                              }
                              else if (grid[idx].type == IC)
                              {
                                   // Para IC se resta el PE.
                                   __sync_fetch_and_add(&grid[idx].resistance, -PE);
                              }
                         }
                    }
               }
               // Liberar recursos locales y salir.
               free(drones);
               munmap(grid, grid_size);
               exit(EXIT_SUCCESS);
          }
          // El padre actualiza el índice inicial para el siguiente proceso.
          start = end;
     }

     // El proceso padre espera a que terminen todos los procesos hijos.
     for (int i = 0; i < n; i++)
     {
          wait(NULL);
     }

     // Clasificar los objetos OM e IC según su estado final.
     int OM_intact = 0, OM_partial = 0, OM_destroyed = 0;
     int IC_intact = 0, IC_partial = 0, IC_destroyed = 0;

     for (int i = 0; i < N * M; i++)
     {
          if (grid[i].type == OM)
          {
               if (grid[i].resistance == grid[i].initial_resistance)
                    OM_intact++;
               // Si la resistencia de OM es 0 o positiva, está destruido.
               else if (grid[i].resistance == 0 || grid[i].resistance > 0)
                    OM_destroyed++;
               else
                    OM_partial++;
          }
          else if (grid[i].type == IC)
          {
               if (grid[i].resistance == grid[i].initial_resistance)
                    IC_intact++;
               // Si la resistencia de IC es 0 o negativa, está destruido.
               else if (grid[i].resistance == 0 || grid[i].resistance < 0)
                    IC_destroyed++;
               else
                    IC_partial++;
          }
     }

     // Imprimir resultados.
     printf("OM sin destruir: %d\n", OM_intact);
     printf("OM parcialmente destruidos: %d\n", OM_partial);
     printf("OM totalmente destruido: %d\n", OM_destroyed);
     printf("IC sin destruir: %d\n", IC_intact);
     printf("IC parcialmente destruidos: %d\n", IC_partial);
     printf("IC totalmente destruido: %d\n", IC_destroyed);

     // Liberar memoria.
     free(drones);
     munmap(grid, grid_size);
     return EXIT_SUCCESS;
}
