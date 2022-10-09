#include <stdio.h>
#include <stdlib.h>
#include "tvtsc.h"
#include "cmpltrtok.h"

int main(int argc, char *argv[]) {
	tvts *ptvts = malloc(sizeof(tvts));
	int ret = tvts_init(
		ptvts,
		"--host", "192.168.31.20",
		"--port", 27017,
		"--name", "tvtsc_quick_demo",
		"--db_name", "tvts",
		"--table_prefix", "train_log",
		"--save_freq", 2,
		"--save_dir", save_dir,

		"--params",
		"ver", PARAMS_DICT_TYPE_S, "v1.0",
		"batch_size", PARAMS_DICT_TYPE_I, 256,
		"lr", PARAMS_DICT_TYPE_F, 0.001,
		"n_epoch", PARAMS_DICT_TYPE_I, 100,

		/* add more */
		"adjust", PARAMS_DICT_TYPE_S, "auto",
		"null_value", PARAMS_DICT_TYPE_N, // None value (Null value)
		"lr", PARAMS_DICT_TYPE_S, "auto", // override, change type
		"ver", PARAMS_DICT_TYPE_S, "alpha_version", // override
		"adjust", PARAMS_DICT_TYPE_S, "performence first", // override

		/* add more */
//		"train_id", PARAMS_DICT_TYPE_I, 1, // this will trigger error TVTS_ERROR_USING_RESERVED_KEYWORD 11
		"lr", PARAMS_DICT_TYPE_F, 0.001 // override, change type again
	);
	if (ret) {
		exit(ret);
	}
}
