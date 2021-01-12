3rdParty libs used:
 - SQLite 

to build... you'll need these packages: make, cmake, llvm, ninja

to ease the pain of installing packages in windows use package management tool ^_^
i.e. https://chocolatey.org/install
 - choco install make
 - choco install cmake
 - choco install llvm
 - choco install ninja

build project via command line (also there are "build" and "rebuild" tasks for vscode)
```
    mkdir build
    cd build
    cmake -GNinja .. 
    ninja
```
