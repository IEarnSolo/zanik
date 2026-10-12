CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror

.PHONY: test clean

test: compost_test
	./compost_test

compost_test: compost.cpp compost.h tests/compost_test.cpp
	$(CXX) $(CXXFLAGS) compost.cpp tests/compost_test.cpp -o $@

clean:
	rm -f compost_test
