#CXX = g++
#CXXFLAGS = -std=c++11 -Wall

linker: Linker.cpp
	 g++ --std=c++11 Linker.cpp -o linker

clean:
	rm -f linker *~