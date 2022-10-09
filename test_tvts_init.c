#include <stdio.h>
#include <stdlib.h>
#include "tvtsc.h"
#include "cmpltrtok.h"

int main() {
	tvts *ptvts = malloc(sizeof(tvts));
	int ret = tvts_init(
		ptvts,
		"--host", "localhost",
		"--port", 27017,
		"--name", "test_tvtsc_x",
		"--db_name", "tvts",
		"--table_prefix", "train_log",
		"--save_freq", 5,
		"--save_dir", "/path/to/weights_dir",

		"--params",
		"ver", PARAMS_DICT_TYPE_S, "v1.0",
		"batch_size", PARAMS_DICT_TYPE_I, 256,
		"lr", PARAMS_DICT_TYPE_F, 0.1,
		"n_epoch", PARAMS_DICT_TYPE_I, 100,

		/* add more */
		"adjust", PARAMS_DICT_TYPE_S, "auto",
		"null_value", PARAMS_DICT_TYPE_N, // None value (Null value)
		"lr", PARAMS_DICT_TYPE_S, "auto", // override, change type
		"ver", PARAMS_DICT_TYPE_S, "alpha_version", // override
		"adjust", PARAMS_DICT_TYPE_S, "performence first", // override

		/* add more */
//		"save_dir", PARAMS_DICT_TYPE_S, "/the/dir/to/save/model", // this will trigger error TVTS_ERROR_USING_RESERVED_KEYWORD 11
		"lr", PARAMS_DICT_TYPE_F, 0.001 // override, change type again
	);
	if (ret) {
		exit(ret);
	}
	params_dict_print(ptvts->params);

	cmp_title("tvtsc.train_id");
	printf("Train ID: %d\n", ptvts->train_id);

	cmp_title("tvts");
	tvts_print(ptvts);

	cmp_title("dict");
	PARAMS_DICT *pdict = malloc(sizeof(PARAMS_DICT));
	params_dict_clone(ptvts->params, pdict);
	params_dict_print(pdict);

	cmp_title("cleaning");
	tvts_free(ptvts);
	free(ptvts);
	params_dict_destroy(pdict);
	free(pdict);

	cmp_title("over");
}
