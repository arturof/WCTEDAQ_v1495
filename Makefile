.PHONY: all clean

CXXFLAGS ?= -O2 -pipe

# libDAQInterface location
DAQInterface= /home/mpmt/Monitoring/libDAQInterface
Dependencies= $(DAQInterface)/Dependencies

ZMQLib= -L$(Dependencies)/zeromq-4.0.7/lib -lzmq
ZMQInclude= -I$(Dependencies)/zeromq-4.0.7/include/

BoostLib= -L$(Dependencies)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I$(Dependencies)/boost_1_66_0/install/include

ToolDAQLib= -L$(Dependencies)/ToolDAQFramework/lib -lToolDAQChain -lServiceDiscovery -lDAQDataModelBase -lDAQStore -lTempDAQDataModel -lTempDAQTools
ToolDAQInclude= -I$(Dependencies)/ToolDAQFramework/include

ToolFrameworkLib= -L$(Dependencies)/ToolFrameworkCore/lib -lDataModelBase -lStore
ToolFrameworkInclude= -I$(Dependencies)/ToolFrameworkCore/include

all: v1495-registers v1495-counters cfd-registers v1495-counters-database

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

# read v1495 counters and send to database
v1495-counters-database: v1495-counters-database.cpp common.o $(DAQInterface)/lib/libDAQInterface.so
	$(CXX) -o $@ $^ -O3 -fPIC -std=c++20 -Wpedantic -lcaen++ -lCAENComm -I$(DAQInterface)/include -L$(DAQInterface)/lib -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolFrameworkInclude) $(ZMQInclude) $(BoostInclude) $(ToolDAQLib) $(ToolFrameworkLib) $(ZMQLib) $(BoostLib) -ljsoncpp
v1495-counters-database.o: v1495-counters-database.cpp common.hpp counters.hpp
	$(CXX) -c $< $(CXXFLAGS) -I$(DAQInterface)/include -L$(DAQInterface)/lib -lDAQInterface $(ToolDAQInclude) $(ToolFrameworkInclude) $(ZMQInclude) $(BoostInclude)

# common functions
common.o: common.cpp common.hpp
	$(CXX) -c $< $(CXXFLAGS)

# generate array with counters names
counters.hpp: make-counters.pl config.json VME1495_counters.txt
	./$< config.json VME1495_counters.txt > $@

clean:
	rm -f v1495-registers v1495-counters cfd-registers v1495-counters-database *.o counters.hpp
