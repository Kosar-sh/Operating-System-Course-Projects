all : VoiceFilters.out 
VoiceFilters.out: main.o 
	g++ -std=c++11 main.o -o VoiceFilters.out -lsndfile

main.o: main.cpp  
	g++ -std=c++11 -c main.cpp

clean:
	rm *.o
