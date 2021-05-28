SRCDIR = .
BINDIR = ../bin
EXECUTABLE = kdsync

CXXFLAGS = -std=c++17 
LDFLAGS = -lrdkafka -lcppkafka -lderecho -pthread -lcrypto -lfmt -I../include


all: $(BINDIR)/$(EXECUTABLE)

debug: CXXFLAGS += -g
debug: all

dir:
	mkdir -p $(BINDIR)

$(BINDIR)/$(EXECUTABLE): dir
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/$(EXECUTABLE) $(SRCDIR)/kdsync.cpp $(LDFLAGS)


clean:
	rm -f $(BINDIR)/*