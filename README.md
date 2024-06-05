### qsortomp.cpp:
1. compile:
- mkdir build
- g++ ./qsortomp.cpp -o ./build/qsortomp -fopenmp
2. run:
- ./build/qsortomp num_data num_threads

### psrsmpi.cpp
1. compile:
- mpic++ -o ./build/psrsmpi psrsmpi.cpp
2. run:
- mpiexec -n num_proc ./build/psrsmpi num_data
