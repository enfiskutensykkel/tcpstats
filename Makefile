NAME=losscalc

CC=$(if $(shell which colorgcc),colorgcc,gcc)
LD := gcc
CFLAGS := -Wall -Wextra -pedantic
LDLIBS := pthread stdc++ pcap

SRC := $(shell find src/ -type f -regextype posix-extended -regex ".+\.(c|cpp)")
HDR := $(shell find src/ -type f -regextype posix-extended -regex ".+\.h")
ALL := $(SRC) $(HDR) README.md LICENSE Makefile
DEF := ETHERNET_FRAME_SIZE=14 
OBJ := $(filter %.o,$(SRC:src/%.c=build/c_%.o)) $(filter %.o,$(SRC:src/%.cpp=build/cpp_%.o))

.PHONY: $(NAME) all clean realclean todo
all: $(NAME)

$(NAME): $(OBJ)
	$(LD) -o $@ $^ $(addprefix -l,$(LDLIBS:-l%=%))

clean:
	-$(RM) $(OBJ)

realclean: clean
	$(RM) $(NAME)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true

build/c_%.o: src/%.c
	-@mkdir -p build
	$(CC) -std=gnu99 -x c $(CFLAGS) -o $@ -c $< $(addprefix -D,$(DEF))

build/cpp_%.o: src/%.cpp
	-@mkdir -p build
	$(CC) -std=gnu++98 -x c++ $(CFLAGS) $(if $(filter NDEBUG,$(DEF)),-O3,-g) -o $@ -c $< $(addprefix -D,$(DEF))
