SRCDIR = .
BIN_TARGET = ../bin
EXECUTABLE = kdsync
CXXFLAGS = -std=c++17 
LDFLAGS = -lrdkafka -lcppkafka -lderecho -pthread -lcrypto -lfmt -I../include


prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin


.PHONY: all debug dir install clean

all: $(BIN_TARGET)/$(EXECUTABLE)

debug: CXXFLAGS += -g
debug: all

dir:
	mkdir -p $(BIN_TARGET)

$(BIN_TARGET)/$(EXECUTABLE): dir
	$(CXX) $(CXXFLAGS) -o $(BIN_TARGET)/$(EXECUTABLE) $(SRCDIR)/kdsync.cpp $(LDFLAGS)

install:
	install -m 755 $(BIN_TARGET)/$(EXECUTABLE) $(bindir)

uninstall:
	rm $(bindir)/$(EXECUTABLE)

clean:
	rm -f $(BIN_TARGET)/*