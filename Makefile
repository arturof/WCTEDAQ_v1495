.PHONY: all clean

CXXFLAGS ?= -O2 -pipe

all: v1495-registers v1495-counters cfd-registers

# write v1495 registers
v1495-registers: v1495-registers.o common.o
	$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs jsoncpp` -lcaen++ -lCAENComm
v1495-registers.o: v1495-registers.cpp common.hpp
	$(CXX) -c $< $(CXXFLAGS) `pkg-config --cflags jsoncpp`

# read v1495 counters
v1495-counters: v1495-counters.o common.o
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcaen++ -lCAENComm
v1495-counters.o: v1495-counters.cpp common.hpp counters.hpp
	$(CXX) -c $< $(CXXFLAGS)

# write cfd registers
cfd-registers: cfd-registers.o common.o
	$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs jsoncpp` -lcaen++ -lCAENComm
cfd-registers.o: cfd-registers.cpp common.hpp
	$(CXX) -c $< $(CXXFLAGS) `pkg-config --cflags jsoncpp`

# common functions
common.o: common.cpp common.hpp
	$(CXX) -c $< $(CXXFLAGS)

# generate array with counters names
counters.hpp: make-counters.pl config.json VME1495_counters.txt
	./$< config.json VME1495_counters.txt > $@

clean:
	rm -f v1495-registers v1495-counters cfd-registers *.o counters.hpp
