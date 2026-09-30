cxx = g++
cxxflags = -std=c++17 -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2) -Iexternal/imgui -Iexternal/imgui/backends -MMD -MP
target = chip8

sources = src/main.cpp src/chip8.cpp \
          external/imgui/imgui.cpp \
          external/imgui/imgui_demo.cpp \
          external/imgui/imgui_draw.cpp \
          external/imgui/imgui_tables.cpp \
          external/imgui/imgui_widgets.cpp \
          external/imgui/backends/imgui_impl_sdl2.cpp \
          external/imgui/backends/imgui_impl_sdlrenderer2.cpp

objects = $(sources:.cpp=.o)
deps = $(objects:.o=.d)

all: $(target)

$(target): $(objects)
	$(cxx) $(cxxflags) -o $(target) $(objects) $(shell pkg-config --libs sdl2)

%.o: %.cpp
	$(cxx) $(cxxflags) -c $< -o $@

-include $(deps)

clean:
	rm -f $(objects) $(deps) $(target)

.phony: all clean