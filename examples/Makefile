SHELL := /bin/bash

ifeq ($(CPP_VERSION),)
CPP_VERSION := c++23
endif

ifeq ($(INCLUDE_DIR),)
INCLUDE_DIR := ..
endif

HEADER_FILES := cfsm.hpp
HEADER_FILES := $(foreach header, $(HEADER_FILES), $(INCLUDE_DIR)/$(header))

INCLUDE_FLAGS := $(foreach include_dir, $(INCLUDE_DIR), -I$(include_dir))

LIBS := pthread
LD_FLAGS := $(foreach lib, $(LIBS), -l$(lib))

SRCS := simple.cc trafficlights.cc benchmark.cc

OBJECTS := $(SRCS:.cc=.o)

TARGETS := $(SRCS:.cc=)

all: $(TARGETS)

$(OBJECTS): $(HEADER_FILES)

$(TARGETS): $(OBJECTS)

./%: ./%.o
	g++ $(INCLUDE_FLAGS) -o $@ $< $(LD_FLAGS)

./%.o: ./%.cc $(HEADER_FILES)
	g++ -std=$(CPP_VERSION) -ggdb3 $(INCLUDE_FLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS) $(TARGETS) 

.PHONY: all clean
