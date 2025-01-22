## Сборка контейнера: 

* docker run -u root -it cpp_image
* apt-get install git
* потом git clone репозитория
* cd lab№

## Запуск лаб:
* 1 - g++ -std=c++20 -fopenmp main.cpp -o main
* 2 - g++ -std=c++20 -mavx main.cpp -o main 
* 3 - g++ -std=c++20 -mfma -mavx main.cpp -o main
* 4 - g++ -std=c++20 -fopenmp  main.cpp vector_mod.cpp randomize.cpp test.cpp num_threads.cpp mod_ops.cpp performance.cpp -o main
* 5 - g++ -std=c++20 -fopenmp main.cpp -o main
* 
P.s. если процессор поддерживает, вместо -mavx указываем флаг -mavx512f
