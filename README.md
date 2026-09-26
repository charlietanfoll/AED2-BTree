# AED2-BTree

## Como executar

Desempacote os arquivos em uma pasta, logo em seguida, verifique se você está em um ambiente linux atualizado com a versão mais recente do seu compilador de C++.
Ou crie um devcontainer com as seguintes depências:


```
{
  "name": "C++23 e CMake (Ubuntu)",
  "image": "mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04",
  "onCreateCommand": "sudo apt-get update && sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends clang clang-format cmake gdb git lldb ninja-build && sudo rm -rf /var/lib/apt/lists/*",
  "customizations": {
    "vscode": {
      "settings": {
        "C_Cpp.default.cppStandard": "c++23",
        "C_Cpp.default.cStandard": "c17",
        "C_Cpp.default.compilerPath": "/usr/bin/g++",
        "cmake.generator": "Ninja",
        "cmake.buildDirectory": "${workspaceFolder}/build",
        "cmake.configureSettings": {
          "CMAKE_CXX_STANDARD": "23",
          "CMAKE_CXX_STANDARD_REQUIRED": "ON",
          "CMAKE_CXX_EXTENSIONS": "OFF"
        },
        "editor.formatOnSave": true
      },
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.cpptools-extension-pack",
        "ms-vscode.cmake-tools"
      ]
    }
  },
  "postCreateCommand": "g++ --version && cmake --version && ninja --version && gdb --version | head -n 1"
}
```

Usando o Cmake,  configure, compile e execute com o seguinte comando na raíz do diretório:

```bash
cmake -S . -B build 
cmake --build build
./build/AED2
```
