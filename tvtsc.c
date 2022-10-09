#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tvtsc.h"
#include "cmpltrtok.h"

char* RESERVED_KEYS[] = {
	"batch",
	"datetime",
	"duration_in_sec",
	"epoch",
	"from_datetime",
	"global_batch",
	"is_temp",
	"memo",
	"name",
	"parent_epoch",
	"parent_id",
	"save_dir",
	"save_freq",
	"to_datetime",
	"train_id",
};

int params_dict_callback_append_bson(PARAMS_DICT *pdict, char* key, PARAMS_DICT_VAL *pval, void* ref_data) {
	bson_t *pbson = (bson_t *)ref_data;
	int type = pval->type;
	PARAMS_DICT_VAL_UNION data = pval->data;
	switch (type) {
	case PARAMS_DICT_TYPE_I:
		if(!bson_append_int32(pbson, key, -1, data.int_val)) {
			return PARAMS_DICT_ERROR_CANNOT_APPEND_BSON;
		}
		break;
	case PARAMS_DICT_TYPE_F:
		if(!bson_append_double(pbson, key, -1, data.float_val)) {
			return PARAMS_DICT_ERROR_CANNOT_APPEND_BSON;
		}
		break;
	case PARAMS_DICT_TYPE_S:
		if(!bson_append_utf8(pbson, key, -1, data.str_val, -1)) {
			return PARAMS_DICT_ERROR_CANNOT_APPEND_BSON;
		}
		break;
	case PARAMS_DICT_TYPE_L:
		if(!bson_append_date_time(pbson, key, -1, data.long_val)) {
			return PARAMS_DICT_ERROR_CANNOT_APPEND_BSON;
		}
		break;
	case PARAMS_DICT_TYPE_N:
		if(!bson_append_null(pbson, key, -1)) {
			return PARAMS_DICT_ERROR_CANNOT_APPEND_BSON;
		}
		break;
	default:
		return PARAMS_DICT_ERROR_IVALID_TYPE;
		break;
	}
	return PARAMS_DICT_ERROR_NONE;
}

int tvts_error(char* title, int code, char* fmt, ...) {
	char buf[MAXLINE];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, MAXLINE, fmt, ap);
	va_end(ap);

	fprintf(stderr, "%s(%d): %s\n", title, code, buf);
	return code;
}

int tvts_ap_into_params_dict(tvts *self, PARAMS_DICT *pdict, va_list ap) {
	if (!pdict) {
		pdict = self->params;
	}

	int ret = 0;
	while (1) {
		char* key = va_arg(ap, char*);
		if (!key) {
			break;
		}

		PARAMS_DICT_VAL *pval = params_dict_get(self->reserved_keys, key);
		if (pval) {
			return tvts_error("tvts_ap_into_params_dict", TVTS_ERROR_USING_RESERVED_KEYWORD, "Key '%s' is reserved and cannot be used by the user.", key);
		}

		char type = va_arg(ap, int);
		int int_val;
		double float_val;
		char* str_val;
		long long long_val;
		switch (type) {
		case PARAMS_DICT_TYPE_I:
			int_val = va_arg(ap, int);
			ret = params_dict_upsert(pdict, key, type, &int_val);
			if (ret) {
				return tvts_error("ap_into_params_dict", ret, "Cannot set key '%s', type '%c', value '%d'!", key, type, int_val);
			}
			break;
		case PARAMS_DICT_TYPE_F:
			float_val = va_arg(ap, double);
			ret = params_dict_upsert(pdict, key, type, &float_val);
			if (ret) {
				return tvts_error("ap_into_params_dict", ret, "Cannot set key '%s', type '%c', value '%lf'!", key, type, float_val);
			}
			break;
		case PARAMS_DICT_TYPE_S:
			str_val = va_arg(ap, char*);
			ret = params_dict_upsert(pdict, key, type, &str_val);
			if (ret) {
				return tvts_error("ap_into_params_dict", ret, "Cannot set key '%s', type '%c', value '%s'!", key, type, str_val);
			}
			break;
		case PARAMS_DICT_TYPE_L:
			long_val = va_arg(ap, long long);
			ret = params_dict_upsert(pdict, key, type, &long_val);
			if (ret) {
				return tvts_error("ap_into_params_dict", ret, "Cannot set key '%s', type '%c', value '%lld'!", key, type, long_val);
			}
			break;
		case PARAMS_DICT_TYPE_N:
			// no corresponding value
			ret = params_dict_upsert(pdict, key, type, 0);
			if (ret) {
				return tvts_error("ap_into_params_dict", ret, "Cannot set key '%s', type '%c'!", key, type);
			}
			break;
		default:
			return tvts_error("ap_into_params_dict", PARAMS_DICT_ERROR_IVALID_TYPE, "Invalid type '%c'!", type);
			break;
		}
	}
	return TVTS_ERROR_NONE;
}

int tvts_init_worker(tvts *self, ...) {
	int ret = 0;
	char buf[MAXLINE];

	// reserved key-words
	int key_word_n = sizeof(RESERVED_KEYS)/sizeof(char*);
	self->reserved_keys = malloc(sizeof(PARAMS_DICT));
	params_dict_init(self->reserved_keys, key_word_n);
	if (1) {
		int i;
		int INT_FLAG = 1;
		for (i = 0; i < key_word_n; ++i) {
			char *key = RESERVED_KEYS[i];
			char type = PARAMS_DICT_TYPE_I;
			ret = params_dict_upsert(self->reserved_keys, key, type, &INT_FLAG);
			if (ret) {
				return tvts_error("tvts_init_worker", ret, "Making self->reserved_keys: Cannot set key '%s', type '%c', value '%d'!", key, type, INT_FLAG);
			}
		}
	}

	//default args
	self->memo = 0;
	self->is_temp = 0;
	self->host = cmp_cp_str(DEFAULT_HOST);
	self->port = DEFAULT_PORT;
	self->db_name = cmp_cp_str(DEFAULT_DB_NAME);
	self->table_prefix = cmp_cp_str(DEFAULT_TABLE_PREFIX);
	self->save_freq = DEFAULT_SAVE_FREQ;
	self->save_dir = 0;
	self->init_weights = 0;
	self->params = malloc(sizeof(PARAMS_DICT));
	params_dict_init(self->params, MAX_DICT_SIZE);

	//parse args
	va_list ap;
	va_start(ap, self);
	int flag_name_set = 0;
	int save_dir_set = 0;
	int params_def_started = 0;
	while (1) {
		char *arg_name = va_arg(ap, char*);
		if (!arg_name) {
			break;
		}

		if (0 == strcmp(arg_name, "--name")) {
			char* name = va_arg(ap, char*);
			if (!name || !name[0]) {
				return tvts_error("tvts_init", TVTS_ERROR_BAD_ARG, "'name' must be set!");
			}
			self->name = cmp_cp_str(name);
			flag_name_set = 1;
		} else if (0 == strcmp(arg_name, "--memo")) {
			char* memo = va_arg(ap, char*);
			if (!memo || !memo[0]) {
				continue;
			}
			free((char*)self->memo);
			self->memo = cmp_cp_str(memo);
		} else if (0 == strcmp(arg_name, "--is_temp")) {
			int is_temp = !!va_arg(ap, int);
			self->is_temp = is_temp;
		} else if (0 == strcmp(arg_name, "--host")) {
			char* host = va_arg(ap, char*);
			if (!host || !host[0]) {
				continue;
			}
			free((char*)self->host);
			self->host = cmp_cp_str(host);
		} else if (0 == strcmp(arg_name, "--port")) {
			int port = va_arg(ap, int);
			if (port <= 0) {
				return tvts_error("tvts_init", TVTS_ERROR_BAD_ARG, "'port' must be positive integer!");
			}
			self->port = port;
		} else if (0 == strcmp(arg_name, "--db_name")) {
			char* db_name = va_arg(ap, char*);
			if (!db_name || !db_name[0]) {
				continue;
			}
			free((char*)self->db_name);
			self->db_name = cmp_cp_str(db_name);
		} else if (0 == strcmp(arg_name, "--table_prefix")) {
			char* table_prefix = va_arg(ap, char*);
			if (!table_prefix) {
				continue;
			}
			free((char*)self->table_prefix);
			self->table_prefix = cmp_cp_str(table_prefix);
		} else if (0 == strcmp(arg_name, "--save_freq")) {
			int sf = va_arg(ap, int);
			if (sf <= 0) {
				return tvts_error("tvts_init", TVTS_ERROR_BAD_ARG, "'save_freq' must be positive integer!");
			}
			self->save_freq = sf;
		} else if (0 == strcmp(arg_name, "--save_dir")) {
			char* save_dir = va_arg(ap, char*);
			if (!save_dir) {
				continue;
			}
			free((char*)self->save_dir);
			self->save_dir = cmp_cp_str(save_dir);
			save_dir_set = 1;
		} else if (0 == strcmp(arg_name, "--init_weights")) {
			char* init_weights = va_arg(ap, char*);
			if (!init_weights || !init_weights[0]) {
				continue;
			}
			free((char*)self->init_weights);
			self->init_weights = cmp_cp_str(init_weights);
		} else if (0 == strcmp(arg_name, "--params")) {
			params_def_started = 1;
			break;
		} else {
			return tvts_error("tvts_init", TVTS_ERROR_BAD_ARG, "Unsupported argument name '%s'!", arg_name);
		}
	}

	if (params_def_started) {
		int ret = tvts_ap_into_params_dict(self, 0, ap);
		if (ret) {
			return tvts_error("tvts_init", ret, "Error occurred when convert arguments into params dict!");
		}
	}
	va_end(ap);

	if (!flag_name_set) {
		return tvts_error("tvts_init", TVTS_ERROR_BAD_ARG, "'name' must be set!");
	}
	if (!save_dir_set) {
		return tvts_error("tvts_init", TVTS_ERROR_BAD_ARG, "Please specify the save_dir at where weights are saved!");
	}
	if (!self->memo) {
		self->memo = cmp_cp_str("(No memo)");
	}
	char *new_memo = cmp_strncat("(libtvtsc) ", MAXLINE, self->memo);
	free((char*)self->memo);
	self->memo = new_memo;

	//table names
	snprintf(buf, MAXLINE, "%s_%s", self->table_prefix, self->name);
	self->table_name = cmp_cp_str(buf);
	snprintf(buf, MAXLINE, "%s_%s_4batch", self->table_prefix, self->name);
	self->table_name_4batch = cmp_cp_str(buf);

	//conn
	mongoc_init();
	snprintf(buf, MAXLINE, "mongodb://%s:%d/?appname=tvts_c", self->host, self->port);
	if(!(self->client = mongoc_client_new(buf))) {
		return tvts_error("tvts_init", TVTS_ERROR_CANNOT_CONN, "Cannot connect to %s!", buf);
	}
	self->db = mongoc_client_get_database(self->client, self->db_name);
	self->table = mongoc_client_get_collection(self->client, self->db_name, self->table_name);
	self->table_4batch = mongoc_client_get_collection(self->client, self->db_name, self->table_name_4batch);

	// get train id
	ret = tvts_get_next_train_id(self, &self->train_id);
	if (ret) {
		return tvts_error("tvts_init", TVTS_ERROR_CANNOT_GET_TRAIN_ID, "Cannot get train_id! (%d)", ret);
	} else {
		fprintf(stderr, "> TVTS: id of this training is %d\n", self->train_id);
	}

	// set important params
	params_dict_upsert_macro(self->params, "name", PARAMS_DICT_TYPE_S, &self->name);
	params_dict_upsert_macro(self->params, "memo", PARAMS_DICT_TYPE_S, &self->memo);
	params_dict_upsert_macro(self->params, "is_temp", PARAMS_DICT_TYPE_I, &self->is_temp);
	params_dict_upsert_macro(self->params, FIELD_NAME_TRAIN_ID, PARAMS_DICT_TYPE_I, &self->train_id);
	params_dict_upsert_macro(self->params, "save_freq", PARAMS_DICT_TYPE_I, &self->save_freq);
	params_dict_upsert_macro(self->params, "save_dir", PARAMS_DICT_TYPE_S, &self->save_dir);
	if (self->init_weights && self->init_weights[0]) {
		params_dict_upsert_macro(self->params, "init_weights", PARAMS_DICT_TYPE_S, &self->init_weights);
	}

	// datetime recorder
	long long now = cmp_current_timestamp();
	params_dict_upsert_macro(self->params, "dt", PARAMS_DICT_TYPE_L, &now);

	return TVTS_ERROR_NONE;
}

int tvts_bson_get_float_field(const bson_t *doc, const char *key, double *pfloat_val) {
	if (!pfloat_val) {
		return TVTS_ERROR_NONE;
	}

	bson_iter_t iter;
	const bson_value_t *pval;

	if(!bson_iter_init_find(&iter, doc, key)) {
		return TVTS_ERROR_CANNOT_FOUND_KEY_IN_BSON;
	}
	pval = bson_iter_value(&iter);
	if (BSON_TYPE_DOUBLE == pval->value_type) {
		*pfloat_val = pval->value.v_double;
	} else {
		return PARAMS_DICT_ERROR_DATA_CORRUPTED;
	}
	return TVTS_ERROR_NONE;
}

int tvts_bson_get_int_field(const bson_t *doc, const char *key, int *pint_val) {
	if (!pint_val) {
		return TVTS_ERROR_NONE;
	}

	bson_iter_t iter;
	const bson_value_t *pval;

	if(!bson_iter_init_find(&iter, doc, key)) {
		return TVTS_ERROR_CANNOT_FOUND_KEY_IN_BSON;
	}
	pval = bson_iter_value(&iter);
	if (BSON_TYPE_INT32 == pval->value_type) {
		*pint_val = pval->value.v_int32;
	} else if (BSON_TYPE_INT64 == pval->value_type) {
		*pint_val = (int)pval->value.v_int64;
	} else {
		return PARAMS_DICT_ERROR_DATA_CORRUPTED;
	}
	return TVTS_ERROR_NONE;
}

int tvts_bson_get_utf8_field(const bson_t *doc, const char *key, char **pstr_val) {
	if (!pstr_val) {
		return TVTS_ERROR_NONE;
	}

	bson_iter_t iter;
	const bson_value_t *pval;

	if(!bson_iter_init_find(&iter, doc, key)) {
		return TVTS_ERROR_CANNOT_FOUND_KEY_IN_BSON;
	}
	pval = bson_iter_value(&iter);
	if (BSON_TYPE_UTF8 == pval->value_type) {
		*pstr_val = (char*)pval->value.v_utf8.str;
	} else {
		return PARAMS_DICT_ERROR_DATA_CORRUPTED;
	}
	return TVTS_ERROR_NONE;
}

int tvts_get_next_train_id(tvts *self, int *ptrain_id) {
	int ret = 0;
	bson_t *filter;
	bson_t *opts;
	mongoc_cursor_t *cursor;
	bson_error_t error;
	const bson_t *doc;

	filter = bson_new();
	opts = BCON_NEW(
		"limit", BCON_INT64 (1),
		"sort",
			"{",
				FIELD_NAME_TRAIN_ID, BCON_INT32 (-1),
			"}",
		"projection",
			"{",
				FIELD_NAME_TRAIN_ID, BCON_BOOL (true),
			"}"
	);

	// get max train_id from self->table
	int id = 1;
	cursor = mongoc_collection_find_with_opts (self->table, filter, opts, NULL);
	while (mongoc_cursor_next (cursor, &doc)) {
		ret = tvts_bson_get_int_field(doc, FIELD_NAME_TRAIN_ID, &id);
		id += 1;
		if (ret) {
			mongoc_cursor_destroy (cursor);
			bson_destroy (filter);
			bson_destroy (opts);
			return ret;
		}
		break;
	}
	if (mongoc_cursor_error (cursor, &error)) {
		fprintf (stderr, "tvts_get_train_id: An error occurred: %s when fetch max %s from %s\n", error.message, FIELD_NAME_TRAIN_ID, self->table_name);
		return TVTS_ERROR_CANNOT_FETCH;
	}
	mongoc_cursor_destroy (cursor);

	// get max train_id from self->table_4batch
	int id4batch = 1;
	cursor = mongoc_collection_find_with_opts (self->table_4batch, filter, opts, NULL);
	while (mongoc_cursor_next (cursor, &doc)) {
		ret = tvts_bson_get_int_field(doc, FIELD_NAME_TRAIN_ID, &id4batch);
		id4batch += 1;
		if (ret) {
			mongoc_cursor_destroy (cursor);
			bson_destroy (filter);
			bson_destroy (opts);
			return ret;
		}
		break;
	}
	if (mongoc_cursor_error (cursor, &error)) {
		fprintf (stderr, "tvts_get_train_id: An error occurred: %s when fetch max %s from %s\n", error.message, FIELD_NAME_TRAIN_ID, self->table_name_4batch);
		return TVTS_ERROR_CANNOT_FETCH;
	}
	mongoc_cursor_destroy (cursor);

	// return the bigger one
	*ptrain_id = (id > id4batch ? id : id4batch);

	// cleaning
	bson_destroy (filter);
	bson_destroy (opts);

	return TVTS_ERROR_NONE;
}

void tvts_free(tvts *self) {
	free((char*)self->host);
	free((char*)self->db_name);
	free((char*)self->table_prefix);
	free((char*)self->name);

	params_dict_destroy(self->params);
	free(self->params);

	free((char*)self->table_name);
	free((char*)self->table_name_4batch);

	mongoc_collection_destroy(self->table);
	mongoc_collection_destroy(self->table_4batch);
	mongoc_database_destroy(self->db);
	mongoc_client_destroy(self->client);
	mongoc_cleanup();
}

void tvts_print(tvts *self) {
	printf("host: \t%s\n", self->host);
	printf("db_name: \t%s\n", self->db_name);
	printf("table_prefix: \t%s\n", self->table_prefix);
	printf("name: \t%s\n", self->name);

	printf("table_name: \t%s\n", self->table_name);
	printf("table_name_4batch: \t%s\n", self->table_name_4batch);

	printf("libmongoc table: \t%p\n", self->table);
	printf("libmongoc table_4batch: \t%p\n", self->table_4batch);
	printf("libmongoc client: \t%p\n", self->client);

	printf("Init params:\n");
	params_dict_print(self->params);
}

int tvts_save_epoch_worker(tvts *self, ...) {
	char buf[MAXLINE];
	int ret = 0;

	//default args
	int flag_epoch_set = 0, flag_save_rel_path_set = 0, flag_save_dir_set = 0;
	int epoch = 0;
	char *save_rel_path = 0, *save_dir = 0;

	//parse args
	va_list ap;
	va_start(ap, self);
	int params_def_started = 0;
	while (1) {
		char* arg_name = va_arg(ap, char*);
		if (!arg_name) {
			break;
		}

		if (0 == strcmp(arg_name, "--epoch")) {
			epoch = va_arg(ap, int);
			if (epoch <= 0) {
				return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "'epoch' must be positive integer!");
			}
			flag_epoch_set = 1;
		} else if (0 == strcmp(arg_name, "--save_rel_path")) {
			char* sp = va_arg(ap, char*);
			if (!sp || !sp[0]) {
				continue;
			}
			save_rel_path = cmp_cp_str(sp);
			flag_save_rel_path_set = 1;
		} else if (0 == strcmp(arg_name, "--save_dir")) {
			char* sd = va_arg(ap, char*);
			if (!sd || !sd[0]) {
				continue;
			}
			save_dir = cmp_cp_str(sd);
			flag_save_dir_set = 1;
		} else if (0 == strcmp(arg_name, "--params")) {
			params_def_started = 1;
			break;
		} else {
			return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "Unsupported argument name '%s'!", arg_name);
		}
	}

	// clone params
	PARAMS_DICT dict = {0};
	params_dict_init(&dict, MAX_DICT_SIZE);
	params_dict_clone(self->params, &dict);

	if (params_def_started) {
		ret = tvts_ap_into_params_dict(self, &dict, ap);
		if (ret) {
			return tvts_error("tvts_save_epoch", ret, "Error occurred when convert arguments into params dict!");
		}
	}

	va_end(ap);

	// epoch and save path
	if (!flag_epoch_set || !epoch) {
		return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "'epoch' must be set!");
	} else {
		params_dict_upsert_macro(&dict, FIELD_NAME_EPOCH, PARAMS_DICT_TYPE_I, &epoch);
	}
	if (flag_save_rel_path_set) {
		params_dict_upsert_macro(&dict, "save_rel_path", PARAMS_DICT_TYPE_S, &save_rel_path);
		free(save_rel_path);
	}
	if (flag_save_dir_set) {
		params_dict_upsert_macro(&dict, "save_dir", PARAMS_DICT_TYPE_S, &save_dir);
		free(save_dir);
	}

	// datetime of record
	long long now = cmp_current_timestamp();
	PARAMS_DICT_VAL *pdict_val = params_dict_get(self->params, "dt");
	if (!pdict_val || pdict_val->type != PARAMS_DICT_TYPE_L) {
		return TVTS_ERROR_DATA_CORRUPTED;
	}
	long long then = pdict_val->data.long_val;
	params_dict_upsert_macro(&dict, "datetime", PARAMS_DICT_TYPE_L, &now);
	params_dict_upsert_macro(&dict, "from_datetime", PARAMS_DICT_TYPE_L, &then);
	params_dict_upsert_macro(&dict, "to_datetime", PARAMS_DICT_TYPE_L, &now);
	double duration = (double)(now - then) / 1000.0;
	params_dict_upsert_macro(&dict, "duration_in_sec", PARAMS_DICT_TYPE_F, &duration);
	params_dict_upsert_macro(self->params, "dt", PARAMS_DICT_TYPE_L, &now);

	// insert data into db
	bson_t *pbson = bson_new();
	ret = tvts_params_dict_to_bson(&dict, pbson);
	bson_error_t error;
	if (!mongoc_collection_insert_one(self->table, pbson, NULL, NULL, &error)) {
		fprintf (stderr, "tvts_save_epoch_worker: %s\n", error.message);
		return TVTS_ERROR_CANNOT_INSERT;
	}

	// cleaning
	params_dict_destroy(&dict);
	bson_destroy(pbson);

	// add index if  it is not there yet
	// get all existing indexes
	PARAMS_DICT dict4indexes;
	params_dict_init(&dict4indexes, MAX_DICT_SIZE);
	ret = tvts_get_index_names_dict(self->table, self->table_name, &dict4indexes);
	PARAMS_DICT_VAL *pval = params_dict_get(&dict4indexes, INDEX_NAME);
	params_dict_destroy(&dict4indexes);
	if (pval) {
		return TVTS_ERROR_NONE;
	}
	// create index
	bson_t keys;
	bson_init(&keys);
	BSON_APPEND_INT32(&keys, FIELD_NAME_TRAIN_ID, 1);
	BSON_APPEND_INT32(&keys, FIELD_NAME_EPOCH, 1);
	ret = tvts_create_index(self->db, self->table_name, INDEX_NAME, &keys, 1);
	if(ret) {
		return ret;
	}

	return TVTS_ERROR_NONE;
}

int tvts_create_index(mongoc_database_t *db, const char* table_name, const char* index_name, bson_t *pkeys, int is_unique) {
	bson_error_t error;
	bson_t *create_indexes;

	fprintf(stderr, "> TVTS: Creating index %s on %s ...\n", index_name, table_name);

	create_indexes = BCON_NEW (
		"createIndexes",
		BCON_UTF8 (table_name),
		"indexes",
			"[",
				"{",
					"key",
					BCON_DOCUMENT (pkeys),
					"name",
					BCON_UTF8 (index_name),
					"unique",
					BCON_BOOL (is_unique),
				"}",
			"]"
	);
	int r = mongoc_database_write_command_with_opts (db, create_indexes, 0, 0, &error);
	if (!r) {
		fprintf (stderr, "Error in createIndexes: %s\n", error.message);
		return TVTS_ERROR_CANNOT_CREATE_INDEX;
	}
	fprintf(stderr, "> TVTS: Created index %s on %s.\n", index_name, table_name);
	bson_destroy(create_indexes);
	return TVTS_ERROR_NONE;
}

int tvts_get_index_names_dict(mongoc_collection_t *collection, const char* collection_name, PARAMS_DICT *pdict) {
	int ret = 0;
	bson_error_t error;
	mongoc_cursor_t *cursor = mongoc_collection_find_indexes_with_opts (collection, NULL);
	if (mongoc_cursor_error (cursor, &error)) {
		fprintf (stderr, "tvts_get_index_names_dict: An error occurred: %s when get indexes from %s\n", error.message, collection_name);
		return TVTS_ERROR_CANNOT_GET_INDEXES;
	}
	const bson_t *doc;
	char *pname;
	while (mongoc_cursor_next (cursor, &doc)) {
		ret = tvts_bson_get_utf8_field(doc, "name", &pname);
		if (ret) {
			return ret;
		}
		ret = params_dict_upsert(pdict, pname, PARAMS_DICT_TYPE_N, 0);
		if (ret) {
			return ret;
		}
	}
	mongoc_cursor_destroy (cursor);
	return TVTS_ERROR_NONE;
}

int tvts_params_dict_to_bson(PARAMS_DICT *pdict, bson_t *pbson) {
	int ret = params_dict_walk_through(pdict, params_dict_callback_append_bson, pbson);
	return ret;
}

int tvts_mark_start_dt(tvts *self) {
	long long now = cmp_current_timestamp();
	int ret = params_dict_upsert(self->params, "dt", PARAMS_DICT_TYPE_L, &now);
	return ret;
}

char *tvts_get_save_name(tvts *self, int epoch) {
	char buf[MAXLINE];
	snprintf(buf, MAXLINE, "%s-%d-%d", self->name, self->train_id, epoch);
	return cmp_cp_str(buf);
}

int tvts_save_batch_worker(tvts *self, ...) {
	char buf[MAXLINE];
	int ret = 0;

	//default args
	int flag_epoch_set = 0, flag_batch_set = 0;
	int epoch = 0, batch = 0, is_batch_global = 0;

	//parse args
	va_list ap;
	va_start(ap, self);
	int params_def_started = 0;
	while (1) {
		char* arg_name = va_arg(ap, char*);
		if (!arg_name) {
			break;
		}

		if (0 == strcmp(arg_name, "--is_batch_global")) {
			is_batch_global = va_arg(ap, int);
		} else if (0 == strcmp(arg_name, "--epoch")) {
			epoch = va_arg(ap, int);
			if (epoch <= 0) {
				return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "'epoch' must be positive integer!");
			}
			flag_epoch_set = 1;
		} else if (0 == strcmp(arg_name, "--batch")) {
			batch = va_arg(ap, int);
			if (batch <= 0) {
				return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "'batch' must be positive integer!");
			}
			flag_batch_set = 1;
		} else if (0 == strcmp(arg_name, "--params")) {
			params_def_started = 1;
			break;
		} else {
			return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "Unsupported argument name '%s'!", arg_name);
		}
	}

	// clone params
	PARAMS_DICT dict = {0};
	params_dict_init(&dict, MAX_DICT_SIZE);
	params_dict_clone(self->params, &dict);

	if (params_def_started) {
		ret = tvts_ap_into_params_dict(self, &dict, ap);
		if (ret) {
			return tvts_error("tvts_save_epoch", ret, "Error occurred when convert arguments into params dict!");
		}
	}

	va_end(ap);

	// epoch and save path
	if (!flag_epoch_set || !epoch) {
		return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "'epoch' must be set!");
	} else if (!flag_batch_set || !batch) {
		return tvts_error("tvts_save_epoch", TVTS_ERROR_BAD_ARG, "'batch' must be set!");
	} else {
		params_dict_upsert_macro(&dict, FIELD_NAME_EPOCH, PARAMS_DICT_TYPE_I, &epoch);
		if (is_batch_global) {
			params_dict_upsert_macro(&dict, FIELD_NAME_GLOBAL_BATCH, PARAMS_DICT_TYPE_I, &batch);
		} else {
			params_dict_upsert_macro(&dict, FIELD_NAME_BATCH, PARAMS_DICT_TYPE_I, &batch);
		}
	}

	// datetime of record
	long long now = cmp_current_timestamp();
	params_dict_upsert_macro(&dict, "datetime", PARAMS_DICT_TYPE_L, &now);

	// insert data into db
	bson_t *pbson = bson_new();
	ret = tvts_params_dict_to_bson(&dict, pbson);
	bson_error_t error;
	if (!mongoc_collection_insert_one(self->table_4batch, pbson, NULL, NULL, &error)) {
		fprintf (stderr, "tvts_save_batch_worker: %s\n", error.message);
		return TVTS_ERROR_CANNOT_INSERT;
	}

	// cleaning
	params_dict_destroy(&dict);
	bson_destroy(pbson);

	// add index if  it is not there yet
	// get all existing indexes
	PARAMS_DICT dict4indexes;
	params_dict_init(&dict4indexes, MAX_DICT_SIZE);
	ret = tvts_get_index_names_dict(self->table_4batch, self->table_name_4batch, &dict4indexes);
	PARAMS_DICT_VAL *pval = params_dict_get(&dict4indexes, INDEX_NAME_4BATCH);
	PARAMS_DICT_VAL *pval2 = params_dict_get(&dict4indexes, INDEX_NAME2_4BATCH);
	params_dict_destroy(&dict4indexes);
	if (!pval) {
		// create index
		bson_t keys;
		bson_init(&keys);
		BSON_APPEND_INT32(&keys, FIELD_NAME_TRAIN_ID, 1);
		BSON_APPEND_INT32(&keys, FIELD_NAME_EPOCH, 1);
		BSON_APPEND_INT32(&keys, FIELD_NAME_BATCH, 1);
		ret = tvts_create_index(self->db, self->table_name_4batch, INDEX_NAME_4BATCH, &keys, 0);
		if(ret) {
			return ret;
		}
	}
	if (!pval2) {
		// create index
		bson_t keys;
		bson_init(&keys);
		BSON_APPEND_INT32(&keys, FIELD_NAME_TRAIN_ID, 1);
		BSON_APPEND_INT32(&keys, FIELD_NAME_GLOBAL_BATCH, 1);
		ret = tvts_create_index(self->db, self->table_name_4batch, INDEX_NAME2_4BATCH, &keys, 0);
		if(ret) {
			return ret;
		}
	}

	return TVTS_ERROR_NONE;
}

int tvts_resume(tvts *self, int parent_train_id, int parent_epoch, char **psave_rel_path, char **psave_dir, int n_keys_of_data_to_restore, ...) {
	if (!parent_train_id) {
		return TVTS_ERROR_NONE;
	} else if (parent_train_id < 0) {
		return tvts_error("tvts_resume", TVTS_ERROR_BAD_ARG, "train_id cannot be less than 0!");
	}

	bson_t *filter;
	bson_t *opts = 0;
	mongoc_cursor_t *cursor;
	bson_error_t error;
	const bson_t *doc;
	int ret = 0;

	if (!psave_rel_path) {
		return tvts_error("tvts_resume", TVTS_ERROR_BAD_ARG, "psave_rel_path (char**) as a ptr to save_rel_path must be provided!");
	} else {
		*psave_rel_path = 0;

		// cursor
		if (!parent_epoch) {
			filter = BCON_NEW(
				FIELD_NAME_TRAIN_ID, BCON_INT32(parent_train_id),
				"save_rel_path",
					"{",
						"$ne", BCON_NULL,
					"}"
			);
			opts = BCON_NEW(
				"sort",
					"{",
						"epoch", BCON_INT32(-1),
					"}",
				"limit", BCON_INT64(1)
			);
			cursor = mongoc_collection_find_with_opts(self->table, filter, opts, 0);
		} else {
			if (parent_epoch < 0) {
				return tvts_error("tvts_resume", TVTS_ERROR_BAD_ARG, "epoch cannot be less than 0!");
			}
			filter = BCON_NEW(
				FIELD_NAME_TRAIN_ID, BCON_INT32(parent_train_id),
				FIELD_NAME_EPOCH, BCON_INT32(parent_epoch)
			);
			cursor = mongoc_collection_find_with_opts(self->table, filter, 0, 0);
		}

		// fetch
		int found = 0;
		int is_save_rel_path_good = 0, is_save_dir_good = 0;
		while (mongoc_cursor_next (cursor, &doc)) {
			found = 1;
			ret = tvts_bson_get_int_field(doc, FIELD_NAME_EPOCH, &parent_epoch);
			if (ret) {
				return tvts_error("tvts_resume", ret, "Cannot get epoch from record!");
			}
			ret = tvts_bson_get_utf8_field(doc, "save_rel_path", psave_rel_path);
			if (!ret) {
				is_save_rel_path_good = 1;
				*psave_rel_path = cmp_cp_str(*psave_rel_path);
			} else {
				*psave_rel_path = 0;
			}
			if (psave_dir) {
				ret = tvts_bson_get_utf8_field(doc, "save_dir", psave_dir);
				if (!ret) {
					is_save_dir_good = 1;
					*psave_dir = cmp_cp_str(*psave_dir);
				} else {
					*psave_dir = 0;
				}
			}
			break;
		}
		if (mongoc_cursor_error (cursor, &error)) {
			return tvts_error("tvts_resume", TVTS_ERROR_CANNOT_FETCH, "An error occurred: %s\n", error.message);
		}

		// cleaning
		mongoc_cursor_destroy (cursor);
		bson_destroy (filter);
		if (opts) bson_destroy (opts);

		// validate
		if (!found) {
			return tvts_error("tvts_resume", TVTS_ERROR_NO_RECORD, "No record!");
		}
		int is_temp = 0;
		ret = tvts_bson_get_int_field(doc, "is_temp", &is_temp);
		if (ret) {
			return tvts_error("tvts_resume", ret, "Cannot get data of type int for key '%s'. Error %d occurred.", "is_temp", ret);
		}
		if (is_temp && !self->is_temp) {
			return tvts_error("tvts_resume", TVTS_ERROR_CANNOT_RESUME_TEMP_FOR_FORMAL, "You cannot run a formal training with a temporary parent!");
		}

		if (!psave_rel_path || !psave_rel_path[0] || !is_save_rel_path_good) {
			return tvts_error("tvts_resume", TVTS_ERROR_NO_RECORD, "No record with saved path!");
		}
	}

	// restore the data from parent record
	if (1) {
		va_list ap;
		va_start(ap, n_keys_of_data_to_restore);
		int i;
		for (i = 0; i < n_keys_of_data_to_restore; ++i) {
			char* key = va_arg(ap, char*);

			PARAMS_DICT_VAL *pval = params_dict_get(self->reserved_keys, key);
			if (pval) {
				return tvts_error("tvts_ap_into_params_dict", TVTS_ERROR_USING_RESERVED_KEYWORD, "Key '%s' is reserved and cannot be used by the user.", key);
			}

			int type = va_arg(ap, int);
			PARAMS_DICT_VAL val;
			val.type = type;
			switch (type) {
			case PARAMS_DICT_TYPE_I:
				ret = tvts_bson_get_int_field(doc, key, (int*)&val.data);
				if (ret) {
					return tvts_error("tvts_resume", ret, "Cannot get data of type int for key '%s'. Error %d occurred.", key, ret);
				}
				break;
			case PARAMS_DICT_TYPE_F:
				ret = tvts_bson_get_float_field(doc, key, (double*)&val.data);
				if (ret) {
					return tvts_error("tvts_resume", ret, "Cannot get data of type double for key '%s'. Error %d occurred.", key, ret);
				}
				break;
			case PARAMS_DICT_TYPE_S:
				ret = tvts_bson_get_utf8_field(doc, key, (char**)&val.data);
				if (ret) {
					return tvts_error("tvts_resume", ret, "Cannot get data of type utf8 string for key '%s'. Error %d occurred.", key, ret);
				}
				break;
			case PARAMS_DICT_TYPE_L:
				return tvts_error("tvts_resume", ret, "Restored type of long long has not been supported yet for key '%s'. Error %d occurred.", key, TVTS_ERROR_NOT_SUPPORTED_YET);
				break;
			default:
				return tvts_error("tvts_resume", ret, "Restored type of unrecognized has not been supported yet for key '%s'. Error %d occurred.", key, TVTS_ERROR_NOT_SUPPORTED_YET);
				break;
			}
			ret = params_dict_upsert(self->params, key, type, &val.data);
			if (ret) {
				return tvts_error("tvts_resume", ret, "When restore value of key '%s' (type '%c'), error %d occurred.", key, type, ret);
			}
		}
		va_end(ap);
	}

	// set parent_id and parent_epoch
	params_dict_upsert(self->params, "parent_id", PARAMS_DICT_TYPE_I, &parent_train_id);
	params_dict_upsert(self->params, "parent_epoch", PARAMS_DICT_TYPE_I, &parent_epoch);

	return TVTS_ERROR_NONE;
}
