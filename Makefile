ifeq (Darwin,$(shell uname -s))
ARCH=-arch x86_64
LIBS += -framework OpenGL
else
ARCH=-m64
LIBS += -lGL
endif

LIBS += -lsfml-graphics -lsfml-window -lsfml-system
INCLUDES=

# compile source.cpp dest.o
define compile
	$(CXX) $(ARCH) $(CXXFLAGS) $(3) -c $(1) -o $(2)
endef
# link sources dest
define link
	$(CXX) $(ARCH) $(LIBS) $(1) -o $(2)
endef

define autolink
	$(call link, $^, $@)
endef


%.o: %.cpp Makefile
	$(call compile, $<, $@)

main: main.o
	$(call autolink)

PHONY: clean

clean:
	-rm *.o
