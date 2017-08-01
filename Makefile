CPP=g++ -std=c++11 -Wall -O3
INCDIR=$(HOME)/include

INCFILES=mcpp_arrays/core.hpp mcpp_arrays/rs_array.hpp

all: test-mcpp-arrays

test-mcpp-arrays: test-mcpp-arrays.cpp mcpp_arrays.hpp $(INCFILES)
	$(CPP) -I. -o $@ $<

install:
	mkdir -p $(INCDIR)/mcpp_arrays
	cp -f mcpp_arrays.hpp $(INCDIR)/
	cp -f $(INCFILES) $(INCDIR)/mcpp_arrays

uninstall:
	rm -rf $(INCDIR)/mcpp_arrays.hpp $(INCDIR)/mcpp_arrays

clean:
	rm -f *~ mcpp_arrays/*~
