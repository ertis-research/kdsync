SRCDIR = .
BUILDDIR = ../build
EXECUTABLE = kdsync

CXX = g++
FLAGS = -g
LDLIBS = -lrdkafka -lcppkafka


all: dir $(BUILDDIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(EXECUTABLE): $(SRCDIR)/kdsync.cpp
	$(CXX) $(FLAGS) -o $(BUILDDIR)/$(EXECUTABLE) $(SRCDIR)/kdsync.cpp $(LDLIBS)


clean:
	rm -f $(BUILDDIR)/*