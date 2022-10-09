#ifndef TVTSC_H_
#define TVTSC_H_

#include <bson/bson.h>
#include <mongoc/mongoc.h>
#include "cmpltrtok.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 27017
#define DEFAULT_DB_NAME "tvts"
#define DEFAULT_TABLE_PREFIX "train_log" /* default table name prefix */
#define DEFAULT_SAVE_FREQ 1 /* default frequency of weight saving, i.e. how many epochs to save once */
#define INDEX_NAME "id_epoch"
#define INDEX_NAME_4BATCH "id_epoch_batch"
#define INDEX_NAME2_4BATCH "id_global_batch"
#define FIELD_NAME_TRAIN_ID "train_id"
#define FIELD_NAME_EPOCH "epoch"
#define FIELD_NAME_BATCH "batch"
#define FIELD_NAME_GLOBAL_BATCH "global_batch"

extern char* RESERVED_KEYS[];

#define TVTS_ERROR_NONE 0
#define TVTS_ERROR_BAD_ARG 1
#define TVTS_ERROR_CANNOT_CONN 2
#define TVTS_ERROR_DATA_CORRUPTED 3
#define TVTS_ERROR_CANNOT_INSERT 4
#define TVTS_ERROR_CANNOT_GET_TRAIN_ID 5
#define TVTS_ERROR_CANNOT_FETCH 6
#define TVTS_ERROR_CANNOT_GET_INDEXES 7
#define TVTS_ERROR_CANNOT_FOUND_KEY_IN_BSON 8
#define TVTS_ERROR_CANNOT_CREATE_INDEX 9
#define TVTS_ERROR_NO_RECORD 10
#define TVTS_ERROR_USING_RESERVED_KEYWORD 11
#define TVTS_ERROR_NOT_SUPPORTED_YET 12
#define TVTS_ERROR_CANNOT_RESUME_TEMP_FOR_FORMAL 13

#define MAX_DICT_SIZE 2000

typedef struct tvts {
    const char *name;
    const char *memo;
    int is_temp;
    const char *host;
    int port;
    const char *db_name;
    const char *table_prefix;
    const char *table_name; // table name for the epoch data
    const char *table_name_4batch; // table name for the epoch data
    int save_freq;
    const char *save_dir;
    const char *init_weights;

    mongoc_client_t *client;
    mongoc_database_t *db;
    mongoc_collection_t *table; // table for the epoch data
    mongoc_collection_t *table_4batch; // table for the batch data

    int train_id;

    PARAMS_DICT *params, *reserved_keys;

    long long dt;
} tvts;

int params_dict_callback_append_bson(PARAMS_DICT *pdict, char* key, PARAMS_DICT_VAL *pval, void *ref_data);
int tvts_error(char *title, int code, char *fmt, ...);
int tvts_ap_into_params_dict(tvts *self, PARAMS_DICT *pdict, va_list ap);
int tvts_init_worker(tvts *self, ...);
#define tvts_init(self, ...) tvts_init_worker(self, __VA_ARGS__, (void *) 0)
int tvts_bson_get_float_field(const bson_t *doc, const char *key, double *pfloat_val);
int tvts_bson_get_int_field(const bson_t *doc, const char *key, int *pint_val);
int tvts_bson_get_utf8_field(const bson_t *doc, const char *key, char **pstr_val);
int tvts_get_next_train_id(tvts *self, int *ptrain_id);
void tvts_free(tvts *self);
void tvts_print(tvts *self);
int tvts_save_epoch_worker(tvts *self, ...);
#define tvts_save_epoch(self, ...) tvts_save_epoch_worker(self, __VA_ARGS__, (void *) 0)
int tvts_create_index(mongoc_database_t *db, const char* table_name, const char* index_name, bson_t *pkeys, int is_unique);
int tvts_get_index_names_dict(mongoc_collection_t *collection, const char* collection_name, PARAMS_DICT *pdict);
int tvts_params_dict_to_bson(PARAMS_DICT *pdict, bson_t *pbson);
int tvts_mark_start_dt(tvts *self);
char *tvts_get_save_name(tvts *self, int epoch);
int tvts_save_batch_worker(tvts *self, ...);
#define tvts_save_batch(self, ...) tvts_save_batch_worker(self, __VA_ARGS__, (void *) 0)
int tvts_resume(tvts *self, int parent_train_id, int parent_epoch, char **psave_rel_path, char **psave_dir, int n_keys_of_data_to_restore, ...);

#ifdef __cplusplus
}
#endif

#endif /* TVTSC_H_ */
