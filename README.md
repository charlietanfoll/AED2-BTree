# AED2-BTree

## Como executar

Com o compilador C++23 instalado, compile e execute o projeto com:

```bash
g++ -std=c++23 -Wall -Wextra -fpermissive main.cpp -o arvoreb && ./arvoreb
```

O programa sera executado no terminal e exibira o menu da Arvore B.

Para usar CMake, configure, compile e execute com:

```bash
cmake -S . -B build
cmake --build build
./build/AED2
```