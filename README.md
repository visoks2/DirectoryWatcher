watch windows directory and do some stuff if something is changed 

to ease the pain of installing packages in windows use package management tool ^_^
i.e. https://chocolatey.org/install
 - choco install make
 - choco install cmake
 - choco install llvm
 - choco install ninja

```
    mkdir build
    cd build
    cmake -GNinja .. 
    ninja
```
