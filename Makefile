
# ========================
# Project settings
# ========================

TARGET := particle_life
SRC    := main.cpp

CXX    := g++
STD    := -std=c++20	-fopenmp

SFML   := -lsfml-graphics -lsfml-window -lsfml-system

# ========================
# Common flags
# ========================

WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
           -Wfloat-equal -Wno-unused-parameter

COMMON := $(STD) $(WARNINGS)  


# ========================
# Debug / test build
# ========================

DEBUG_FLAGS := \
	-O0 \
	-g3 \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer \
	-fstack-protector-strong \
	-D_GLIBCXX_ASSERTIONS \
	-DDEBUG

DEBUG := $(COMMON) $(DEBUG_FLAGS)

# ========================
# Release / max performance
# ========================

RELEASE_FLAGS := \
	-O3 \
	-march=native \
	-fomit-frame-pointer \
	-flto \
	-fno-exceptions \
	-fno-rtti \
	-funroll-loops \
	-ffast-math \
	-DNDEBUG

RELEASE := $(STD) $(RELEASE_FLAGS)

# ========================
# Targets
# ========================

all: release

debug:
	$(CXX) $(SRC) $(DEBUG) -o $(TARGET)_debug $(SFML)

release:
	$(CXX) $(SRC) $(RELEASE) -o $(TARGET) $(SFML)

clean:
	rm -f $(TARGET) $(TARGET)_debug
