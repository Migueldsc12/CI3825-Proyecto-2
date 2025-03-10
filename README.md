# Teatro de Operaciones para Drones - CI3825

## 📖 Descripción
El objetivo del proyecto es el diseño de una solución algorítmica para un problema, haciendo uso de hilos o procesos de UNIX, para obtener un aceleramiento de la solución, con respecto a una solución de un solo proceso.

Este proyecto es parte de la materia **CI3825 - Sistemas de Operación I** en la **Universidad Simón Bolívar**.

---

## 📂 Estructura del Proyecto

```
CI3825-Proyecto-1/
│── src/               # Código fuente
|   |── teopHilos/
|   |   |── Makefile
|   |   |── teoph.c
|   |── teopProcesos/
|   |   |── teopp.c
|   |   |── Makefile
│── test/
|   |── test.txt
|   |── test2.txt
|   |── test3.txt
│── README.md          
```

---

## 🛠️ **Compilación**

Para compilar el proyecto, usa `make` en la terminal en la carpeta del programa que se quiere ejecutar:

```sh
make
```

Para compilar en **modo depuración**, usa:

```sh
make DEBUG=1
```

Para limpiar archivos compilados:

```sh
make clean
```

---

## 🚀 **Ejecución**

Después de compilar, ejecuta el programa con:

```sh
./teopX n test.txt
```
Siendo X la letra h o p segun el programa que se quiera ejecutar y n el numero de procesos/hilos

---


---

## ✍️ **Autores**
- **Eliezer Cario - 18-10605**
- **Miguel Salomon - 19-10274**

---
