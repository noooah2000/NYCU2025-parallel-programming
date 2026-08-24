CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3

pi.out: pi.o
	$(CXX) $(CXXFLAGS) pi.o -o pi.out

pi.o: pi.cpp
	$(CXX) $(CXXFLAGS) -c pi.cpp

clean:
	rm -f *.o pi.out