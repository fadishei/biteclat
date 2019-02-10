BITSET ?= ROARING
MEMPROF ?= 0

OBJXXS :=
OBJS := stats.o bitset.o itemset.o itemtree.o eclat.o main.o
CFLAGS := -O2 -Wno-unused-result -DBITSET=$(BITSET)
CXXFLAGS := -O2 -std=c++11 -DBITSET=$(BITSET)
LDFLAGS := -no-pie -pthread -lm
OUT := eclat

ifeq ($(BITSET),ROARING)
	OBJS += roaring/roaring.o wrapper_roaring.o
	CFLAGS += -Iroaring
else ifeq ($(BITSET),EWAH)
	OBJXXS += wrapper_ewah.opp
	CXXFLAGS += -Iewah
else ifeq ($(BITSET),BM)
	OBJXXS += wrapper_bm.opp
	CXXFLAGS += -Ibm
else ifeq ($(BITSET),CONCISE)
	OBJXXS += wrapper_concise.opp
	CXXFLAGS += -Iconcise
endif

ifeq ($(MEMPROF),1)
	CFLAGS += -DMEMPROF
	LDFLAGS += -ltcmalloc
endif

default: $(OUT)

all: default

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.opp: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT): $(OBJS) $(OBJXXS)
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o *.opp $(OUT)

