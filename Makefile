DEBUG=1
DIR_OF_OUTPUT_H=/usr/local/include
DIR_OF_OUTPUT_SO=/usr/local/lib
DIR_OF_LIBCMPLTRTOK_H=/usr/local/include
DIR_OF_LIBCMPLTRTOK_SO=/usr/local/lib
DIR_OF_LIBMONGOC_H=/usr/include/libmongoc-1.0
DIR_OF_LIBMONGOC_SO=/usr/lib/x86_64-linux-gnu
DIR_OF_LIBBSON_H=/usr/include/libbson-1.0
DIR_OF_LIBBSON_SO=/usr/lib/x86_64-linux-gnu


###############################################################################
# CAUTION! Modify below only if you are famaliar with Make

current_dir := $(shell pwd)
SHELL := /bin/bash 


ifeq ($(DEBUG), 1) 
CFLAGS=-O0 -g
config_label='Debug'
else
CFLAGS=-Ofast
config_label='Release'
endif

common=-D_GNU_SOURCE
common+=-I$(DIR_OF_LIBBSON_H) -I$(DIR_OF_LIBMONGOC_H) -I$(DIR_OF_LIBCMPLTRTOK_H)
LDFLAGS=-L$(DIR_OF_LIBBSON_SO) -L$(DIR_OF_LIBMONGOC_SO) -L$(DIR_OF_LIBCMPLTRTOK_SO)
LDLIBS=-lbson-1.0 -lmongoc-1.0 -lcmpltrtok -lm
ldflags_this=-L$(current_dir)
ldlibs_this=-ltvtsc
libs_spec=$(ldflags_this) $(ldlibs_this) $(LDFLAGS) $(LDLIBS)

common_prerequisites=Makefile
OUTPUT_H=tvtsc.h
OUTPUT_SO=libtvtsc.so

all: $(OUTPUT_SO) \
test_tvts_init.out \
x100_fake_demo_simple.out \
x200_fake_demo_samulating_practice.out

$(OUTPUT_SO): tvtsc.o $(common_prerequisites)
	gcc $(common) $(CFLAGS) -shared tvtsc.o -o $(OUTPUT_SO) $(LDFLAGS) $(LDLIBS)

tvtsc.o: tvtsc.c tvtsc.h $(common_prerequisites)
	gcc $(common) $(CFLAGS) -fpic -c tvtsc.c -o tvtsc.o

test_tvts_init.out: test_tvts_init.c tvtsc.h $(OUTPUT_SO) $(common_prerequisites)
	gcc $(common) $(CFLAGS) test_tvts_init.c -o test_tvts_init.out $(libs_spec)

x100_fake_demo_simple.out: x100_fake_demo_simple.c tvtsc.h $(OUTPUT_SO) $(common_prerequisites)
	gcc $(common) $(CFLAGS) x100_fake_demo_simple.c -o x100_fake_demo_simple.out $(libs_spec)

x200_fake_demo_samulating_practice.out: x200_fake_demo_samulating_practice.c tvtsc.h $(OUTPUT_SO) $(common_prerequisites)
	gcc $(common) $(CFLAGS) x200_fake_demo_samulating_practice.c -o x200_fake_demo_samulating_practice.out $(libs_spec)

.PHONY : check
check : 
	@echo "Is for Debug or Releas:" $(config_label)
	@echo "Path to install the header:" $(DIR_OF_OUTPUT_H)/$(OUTPUT_H)
	@echo "Path to install the so:" $(DIR_OF_OUTPUT_SO)/$(OUTPUT_SO)
	@echo "The dir to seek the header of libcmpltrtok:" $(DIR_OF_LIBCMPLTRTOK_H)
	@echo "The dir to seek the so of libcmpltrtok:" $(DIR_OF_LIBCMPLTRTOK_SO)
	@echo "The dir to seek the header of libmongoc-dev:" $(DIR_OF_LIBMONGOC_H)
	@echo "The dir to seek the so of libmongoc-dev:" $(DIR_OF_LIBMONGOC_SO)
	@echo "The dir to seek the header of libbson-dev:" $(DIR_OF_LIBBSON_H)
	@echo "The dir to seek so of libbson-dev:" $(DIR_OF_LIBBSON_SO)

.PHONY : install
install : $(OUTPUT_SO) $(OUTPUT_H)
	cp -iv $(OUTPUT_H) $(DIR_OF_OUTPUT_H)
	chmod -v 0444 $(DIR_OF_OUTPUT_H)/$(OUTPUT_H)
	cp -iv $(OUTPUT_SO) $(DIR_OF_OUTPUT_SO)
	chown -v root:root $(DIR_OF_OUTPUT_SO)/$(OUTPUT_SO)
	chmod -v 0755 $(DIR_OF_OUTPUT_SO)/$(OUTPUT_SO)

.PHONY : uninstall
uninstall :
	-rm -v $(DIR_OF_OUTPUT_H)/$(OUTPUT_H)
	-rm -v $(DIR_OF_OUTPUT_SO)/$(OUTPUT_SO)

.PHONY : clean
clean : 
	-rm -v *.o
	-rm -v *.out
	-rm -v *.so
