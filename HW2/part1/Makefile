CXX = g++

CXXFLAGS = -std=c++17 -Wall -O3 -ffast-math -mavx2 -mfma

pi.out: pi.o
	$(CXX) $(CXXFLAGS) pi.o -o pi.out -lpthread

pi.o: pi.cpp
	$(CXX) $(CXXFLAGS) -c pi.cpp

clean:
	rm -f *.o pi.out
