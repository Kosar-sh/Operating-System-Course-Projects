all : mapreduce storage product
mapreduce: main.o logger.o
	g++ -std=c++17 main.o logger.o -o mapreduce

storage: storage.o
	g++ -std=c++17 storage.o logger.o -o storage

product: product.o
	g++ -std=c++17 product.o logger.o -o product

storage.o: storage.cpp logger.hpp
	g++ -std=c++17 -c storage.cpp

main.o: main.cpp logger.hpp
	g++ -std=c++17 -c main.cpp

product.o: product.cpp logger.hpp
	g++ -std=c++17 -c product.cpp

logger.o: logger.cpp logger.hpp colorprint.hpp
	g++ -std=c++17 -c logger.cpp

clean:
	rm *.o
