SRCDIR = .
BINDIR = ../bin
EXECUTABLE = kdsync

CXX = g++
FLAGS = -g
LDLIBS = -lrdkafka -lcppkafka


all: dir $(BINDIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BINDIR)

$(BINDIR)/$(EXECUTABLE): $(SRCDIR)/kdsync.cpp
	$(CXX) $(FLAGS) -o $(BINDIR)/$(EXECUTABLE) $(SRCDIR)/kdsync.cpp $(LDLIBS)


clean:
	rm -f $(BINDIR)/*