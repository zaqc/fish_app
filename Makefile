PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = fish_test.o

PKT = pkg-config gtkmm-3.0 gtk+-3.0 epoxy

INC = $(shell $(PKT) --cflags)
INC += "-I/usr/include/"
LIB = $(shell $(PKT) --libs)

CFLAGS += -g -Wall

all:	fish_test

fish_test:	$(OBJS)
	$(CXX) -o $@ $^ $(LIB)

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) -o $@ $<

clean:
	rm -fr fish_test $(OBJS)
