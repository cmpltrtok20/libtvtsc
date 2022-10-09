# A brief intro to libtvtsc

[libtvtsc](https://github.com/cmpltrtok20/libtvtsc) is a C translation of the python code for the scalar recording of [TVTS (Trackable and Visible Training System)](https://github.com/cmpltrtok20/tvts) in order to let the user uses TVTS in C.

[TVTS](https://github.com/cmpltrtok20/tvts) (**Trackable and Visible Training System**) is an auxiliary system for machine learning or deep learning model training written in Python3, with [MongoDB](https://www.mongodb.com/) as its storage backend and [Matplotlib](https://matplotlib.org/) as its visualization backend.

Currently, **only Linux platforms** are considered. Maybe the user could run it on other Unix-like platforms, but it is not tested.

# Prerequisites
 - [libmongoc-dev](https://mongoc.org/libmongoc/current/installing.html)
 - [libbson-dev](https://mongoc.org/libmongoc/current/installing.html)
 - [libcmpltrtok](https://github.com/cmpltrtok20/libcmpltrtok) ( Also a project of mine. )

# Clone the code

```bash
$ mkdir libtvtsc
$ git clone https://github.com/cmpltrtok20/libtvtsc.git libtvtsc
```

# Modify the Makefile for customization

```bash
$ cd libtvtsc
$ nano Makefile
```

At the beginning lines of the Makefile of this project, the user can edit it to modify:

- Will the build be for Debug (DEBUG=1) or Release (DEBUG=0)?
- The installation destinations for the header and the shared library.
- The directory of the header and the shared library of [libcmpltrtok](https://github.com/cmpltrtok20/libcmpltrtok)
- The directory of the header and the shared library of [libmongoc-dev](https://mongoc.org/libmongoc/current/installing.html)
- The directory of the header and the shared library of [libbson-dev](https://mongoc.org/libmongoc/current/installing.html)

As below:

```makefile
DEBUG=1
DIR_OF_OUTPUT_H=/usr/local/include
DIR_OF_OUTPUT_SO=/usr/local/lib
DIR_OF_LIBCMPLTRTOK_H=/usr/local/include
DIR_OF_LIBCMPLTRTOK_SO=/usr/local/lib
DIR_OF_LIBMONGOC_H=/usr/include/libmongoc-1.0
DIR_OF_LIBMONGOC_SO=/usr/lib/x86_64-linux-gnu
DIR_OF_LIBBSON_H=/usr/include/libbson-1.0
DIR_OF_LIBBSON_SO=/usr/lib/x86_64-linux-gnu
```

The user could run "**make check**" to check these values after saving the modification to Makefile, as below:

```bash
$ cd libtvtsc
$ make check 
Is for Debug or Releas: Debug
Path to install the header: /usr/local/include/tvtsc.h
Path to install the so: /usr/local/lib/libtvtsc.so
The dir to seek the header of libcmpltrtok: /usr/local/include
The dir to seek the so of libcmpltrtok: /usr/local/lib
The dir to seek the header of libmongoc-dev: /usr/include/libmongoc-1.0
The dir to seek the so of libmongoc-dev: /usr/lib/x86_64-linux-gnu
The dir to seek the header of libbson-dev: /usr/include/libbson-1.0
The dir to seek so of libbson-dev: /usr/lib/x86_64-linux-gnu
$
```

# Build and install

```bash
$ cd libtvtc
$ make
$ sudo make install
```

Then the shared library **libtvtsc.so** will be built and installed with header file **tvtsc.h**.

Or the user could just simply copy the tvtsc.h and the libtvtsc.so to where the user wishes to install them and give them the appropriate ownership and file mode after executing "make".

# Uninstall

The user could uninstall them like below:

```bash
$ cd libtvtsc
$ sudo make uninstall
```

Or the user could just simply delete the tvtsc.h and the libtvtsc.so at where the user installed them.

# Invoke and compile with libcmpltrtok

The user could use these routines declared in tvtsc.h in the user's C code such as below (using "tvts_init"):


```c
#include <stdio.h>
#include <stdlib.h>
#include "tvtsc.h"

// The user could invoke these routines declared in tvtsc.h
tvts *ptvts = malloc(sizeof(tvts));
int ret = tvts_init(
	ptvts
	, "--name", NAME
	, "--memo", "The memo about this time of training"
	, "--is_temp", 1
	, "--host", "localhost"
	, "--port", 27017

	, "--db_name", "tvts"
	, "--table_prefix", "train_log"
	, "--save_freq", 2
	, "--save_dir", "/path/to/dir/for/saving/weights",

	"--params"
	, "batch_size", PARAMS_DICT_TYPE_I, N_BATCH_SIZE
	, "lr", PARAMS_DICT_TYPE_F, LR
	, "gamma", PARAMS_DICT_TYPE_F, GAMMA
	, "n_epoch", PARAMS_DICT_TYPE_I, N_EPOCHS
);
if (ret) {
	fprintf(stderr, "Cannot init tvts struct! Error number is %d.", ret);
	exit(ret);
}
```

Then the user could compile her code like the below:

```bash
$ gcc \
 -I/dir/path/of/libbson-dev_header \
 -I/dir/path/of/libmongoc-dev_header \
 -I/dir/path/of/libtvtsc_header \
 -I/dir/path/of/libcmpltrtok_header \
 -c user_code.c -o user_code.o \
 -L/dir/path/of/libtvtsc_so -ltvtsc \
 -L/dir/path/of/libbson-dev_so -lbson-1.0 \
 -L/dir/path/of/libmongoc-dev_so -lmongoc-1.0 \
 -L/dir/path/of/libcmpltrtok_so -lcmpltrtok
```

# Run with libtvtsc

If the user installed the libtvtsc.so into a system library location, the user probably could directly run her program that depends on the libtvtsc.so. If the system complains it cannot find the libtvtsc.so, please add the path of the directory of the libtvtsc.so to the environment variable LD\_LIBRARY\_PATH like below ( The user could also add the path of the directories of the shared libraries of libcmpltrtok, libmongoc-dev, and libbson-dev if the system cannot find them. ):

```bash
$ export LD_LIBRARY_PATH="/dir/path/of/libmongoc-dev_so:\
/dir/path/of/libbson-dev_so:\
/dir/path/of/libcmpltrtok_so:\
/dir/path/of/libtvtsc_so${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
$ ./the_exec
```

# Example codes

 - x100\_fake\_demo\_simple.c

It is the equivalent code of tvts/examples/x100\_fake\_demo\_simple.py.

 - x200\_fake\_demo\_samulating\_practice.c

It is the equivalent code of tvts/examples/x200\_fake\_demo\_samulating\_practice.py.

Note: The user could use x100\_fake\_demo\_simple.c and tvts/examples/x100\_fake\_demo\_simple.py in [TVTS](https://github.com/cmpltrtok20/tvts) interchangeably with the same TVTS name, because they both do not save/load weights, but she could not do it for x200\_fake\_demo\_samulating\_practice.c and tvts/examples/x200\_fake\_demo\_samulating\_practice.py in [TVTS](https://github.com/cmpltrtok20/tvts), i.e. the user should use 2 different TVTS name for running x200\_fake_demo\_samulating\_practice.c and tvts/examples/x200\_fake\_demo\_samulating\_practice.py.

# About the author

Hi, my name is [**Yunpeng Pei**](https://github.com/cmpltrtok20), an AI engineer working in the City of Beijing, China. I developed [these projects](https://github.com/cmpltrtok20?tab=repositories) in my spare time just for interest. I will be very happy if you find them to be very little help to the community. I must say sorry that they are far away from perfect and their document is not finished yet, because of my limited ability and limited resources on these projects; although I have tried my best. You could contact me freely by email to my personal email **belarc@163.com**.

