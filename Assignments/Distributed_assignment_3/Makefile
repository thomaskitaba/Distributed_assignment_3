CXX      = mpicxx
CXXFLAGS = -O3 -march=native -fopenmp -std=c++11

all: cardiacsim serial

cardiacsim: cardiac.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

serial: serial.cpp
	g++ -O3 -march=native -std=c++11 -o $@ $<

clean:
	rm -f cardiacsim serial *.o
