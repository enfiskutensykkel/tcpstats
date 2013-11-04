### Makefile for tcpstats ###
PROJECT=tcpstats
DEFINES=ETHERNET_FRAME_SIZE=14
OBJ_DIR=build
SRC_DIR=src

### Compiler and linker settings ###
CC=$(if $(shell which colorgcc),colorgcc,gcc)
LD := gcc
CFLAGS := -Wall -Wextra -pedantic
LDLIBS := pthread stdc++ pcap

### Generic make variables ###
DEF := $(filter-out %DEBUG,$(DEFINES)) $(if $(filter DEBUG,$(DEFINES)),DEBUG,NDEBUG)
SRC := $(shell find $(SRC_DIR) -type f -regextype posix-extended -regex ".+\.(c|cpp)")
HDR := $(shell find $(SRC_DIR) -type f -regextype posix-extended -regex ".+\.h")
ALL := $(SRC) $(HDR) Makefile LICENSE README.md


### Make targets ###
.PHONY: $(PROJECT) all clean realclean todo
all: $(PROJECT)

define cpp_compile_target
$(OBJ_DIR)/$(2): $(1) $(HDR)
	-@mkdir -p $$(@D)
	$$(CC) -x c++ -std=gnu++98 $$(CFLAGS) $(if $(filter DEBUG,$(DEF)),-g,-O3) $(addprefix -D,$(DEF:-D%=D)) -o $$@ -c $$<
OBJ += $(OBJ_DIR)/$(2)
endef

define c_compile_target
$(OBJ_DIR)/$(2): $(1)
	-@mkdir -p $$(@D)
	$$(CC) -x c -std=gnu99 $$(CFLAGS) $(if $(filter DEBUG,$(DEF)),-g,-O3) $(addprefix -D,$(DEF:-D%=D)) -o $$@ -c $$<
OBJ += $(OBJ_DIR)/$(2)
endef

$(foreach file,$(filter-out %.c,$(SRC)),$(eval $(call cpp_compile_target,$(file),$(notdir $(file:%.cpp=%.o)))))
$(foreach file,$(filter-out %.cpp,$(SRC)),$(eval $(call c_compile_target,$(file),$(notdir $(file:%.c=%.o)))))


$(PROJECT): $(OBJ)
	$(LD) -o $@ $^ $(addprefix -l,$(LDLIBS:-l%=%))

clean:
	-$(RM) $(OBJ)

realclean: clean
	-$(RM) $(PROJECT)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true
