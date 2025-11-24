# Simulación de ecosistema con zorros y conejos (secuencial y paralela)

Este repositorio implementa una simulación de ecosistemas tipo **depredador–presa** en una grilla 2D con:

- **Rocas** (`ROCK`)
- **Conejos** (`RABBIT`)
- **Zorros** (`FOX`)

En cada generación, los conejos y zorros se mueven, se reproducen y (en el caso de los zorros) pueden morir de hambre según reglas dadas.

---

## Archivos

- `seq.cpp`  
  Implementación **secuencial** básica.  
  Lee el estado inicial por `stdin`, simula `N_GEN` generaciones y escribe el estado final en el formato solicitado.

- `seqTab.cpp`  
  Versión **secuencial con visualización**:
  - Muestra la grilla en ASCII por generación.
  - Muestra conteo de conejos, zorros y rocas.
  - Puede mostrar estadísticas de población por generación.
  - Control con pausa, slow down y speed up

- `parallel.cpp`  
  Implementación **paralela** con **OpenMP** de la simulación básica (sin menú).  
  Paraleliza la actualización de la grilla utilizando bloques o locks por celda para resolver conflictos de escritura y preservar las reglas del modelo.  

- `parallelTab.cpp`  
  Implementación **paralela con OpenMP** y **visualización/estadísticas**:
  - Misma lógica base que `parallel.cpp`.
  - Misma idea de visualización/tabla de poblaciones que `seqTab.cpp`.
  - Control con pausa, slow down y speed up

---

## Formato de entrada

Todos los programas comparten el mismo formato de entrada por `stdin`:

1. Primera línea (parámetros globales):

    GEN_PROC_RABBITS GEN_PROC_FOXES GEN_FOOD_FOXES N_GEN R C N

    Donde:

    - GEN_PROC_RABBITS: generaciones mínimas para que un conejo pueda reproducirse.
    - GEN_PROC_FOXES: generaciones mínimas para que un zorro pueda reproducirse.
    - GEN_FOOD_FOXES: generaciones máximas que un zorro sobrevive sin comer antes de morir.
    - N_GEN: número total de generaciones a simular.
    - R, C: número de filas (R) y columnas (C) de la grilla.
    - N: número de objetos iniciales en el mundo.

2. Siguientes N líneas: descripción de cada objeto inicial:

    TYPE x y

    Donde:

    - TYPE ∈ {ROCK, RABBIT, FOX}
    - x: fila (0-based)
    - y: columna (0-based)

    Ejemplo de archivo de entrada:

        3 4 5 10 5 5 4
        ROCK 0 0
        RABBIT 2 2
        FOX 3 4
        RABBIT 4 1

---

## Formato de salida

Las versiones sin interfaz escriben al final en `stdout` el estado de la ultima generacion con el mismo formato de la entreda:

    GEN_PROC_RABBITS GEN_PROC_FOXES GEN_FOOD_FOXES 0 R C N_final
    TYPE x y
    TYPE x y
    ...

Donde:

- N_final: número de objetos que quedan tras N_GEN generaciones.
- Cada línea siguiente describe una roca, conejo o zorro en su posición final.

Las versiones con interfaz muestran:

- La evolución de la grilla en pantalla por generación.
- Estadísticas de población por generación.
- Estado final en el mismo formato de las otras versiones.

---

## Visualización y estadísticas (versiones *Tab*)

En las versiones con visualización (`seqTab.cpp` y `parallelTab.cpp`) la grilla representa el estado con los siguientes simbolos ASCII:

- .  → celda vacía
- *  → roca
- R  → conejo
- F  → zorro

Por generacion muestra:

- Número de generación.
- Conteo de conejos, zorros y rocas.
- La grilla ASCII actualizada.

---

## Requisitos

- Compilador C++ con soporte para **C++17** (por ejemplo `g++`).
- Para las versiones paralelas (`parallel.cpp`, `parallelTab.cpp`):
  - Soporte para **OpenMP** 

---

## Compilación

Ejemplos usando `g++`:

- Versión secuencial básica:

        g++ -O2 seq.cpp -o seq

- Versión secuencial con visualización/estadísticas:

        g++ -O2 seqTab.cpp -o seqTab

- Versión paralela básica (OpenMP):

        g++ -O2 -fopenmp parallel.cpp -o parallel

- Versión paralela con visualización/estadísticas (OpenMP):

        g++ -O2 -fopenmp parallelTab.cpp -o parallelTab

---

## Ejemplos de uso

Suponiendo que tienes archivos de entrada en una carpeta `tests/`:

- Simulación secuencial básica:

    - Linux / macOS:

            ./seq < tests/input1.txt > output_seq.txt

    - Windows (PowerShell):

            .\seq.exe < .\tests\input1.txt > .\output_seq.txt

- Simulación paralela básica:

    - Linux / macOS:

            ./parallel < tests/input1.txt > output_parallel.txt

    - Windows (PowerShell):

            .\parallel.exe < .\tests\input1.txt > .\output_parallel.txt

- Versión con visualización secuencial:

    - Linux / macOS:

            ./seqTab < tests/input1.txt

    - Windows (PowerShell):

            .\seqTab.exe < .\tests\input1.txt

- Versión con visualización paralela:

    - Linux / macOS:

            ./parallelTab < tests/input1.txt

    - Windows (PowerShell):

            .\parallelTab.exe < .\tests\input1.txt

---

