MCL_USE_OMP=1
MCL_USE_GMP_LIB=0
XBYAK_DIR?=./ext/xbyak
XBYAK_AARCH64_DIR?=./ext/xbyak_aarch64
include common.mk
INC_DIR= -I ./ -I $(XBYAK_DIR) -I include
ifneq ($(findstring $(ARCH),arm64/aarch64),)
INC_DIR+= -I $(XBYAK_AARCH64_DIR)
LDFLAGS+= -L $(XBYAK_AARCH64_DIR)/lib -lxbyak_aarch64
endif

CFLAGS += $(INC_DIR)
HEADER=include/constdiv.hpp
#VPATH=test

BASE=div7_test
SRC=$(BASE).cpp
ALL_SRC=$(SRC) div7.cpp
EXE=bin/$(BASE).exe

TARGET=$(EXE)
all:$(TARGET)

.SUFFIXES: .cpp

obj/%.o: test/%.cpp $(HEADER)
	$(CXX) -c -o $@ $< $(CFLAGS) -MMD -MP -MF $(@:.o=.d)

$(EXE): obj/$(BASE).o obj/div7.o
	$(CXX) -o $@ $< $(LDFLAGS) obj/div7.o

clean:
	$(RM) obj/*.o obj/*.d $(TARGET) bin/*.exe

test: $(EXE)
	./$(EXE)

test_alld: $(EXE)
	./$(EXE) -alld

.PHONY: test clean

# don't remove these files automatically
.SECONDARY: $(addprefix bin/, $(ALL_SRC:.cpp=.o))
