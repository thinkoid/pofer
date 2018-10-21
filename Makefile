# -*- mode: Makefile; -*-

CXX = g++

CXXSTD = -std=c++17

CPPFLAGS = -I.
CXXFLAGS = -pthread -g -O $(CXXSTD) -W -Wall -Wno-multichar -pedantic

LDFLAGS = -pthread
LIBS = -lboost_log -lboost_log_setup -lboost_system -lboost_thread -lstdc++fs

DEPENDDIR = ./.deps
DEPENDFLAGS = -M

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc,%.o,$(SRCS))

TARGETS = pof

all: $(TARGETS)

DEPS = $(patsubst %.o,$(DEPENDDIR)/%.d,$(OBJS))
-include $(DEPS)

$(DEPENDDIR)/%.d: %.cc $(DEPENDDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPENDFLAGS) $< >$@

$(DEPENDDIR):
	@[ ! -d $(DEPENDDIR) ] && mkdir -p $(DEPENDDIR)

%: %.cc

pof: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) $(TARGETS)

