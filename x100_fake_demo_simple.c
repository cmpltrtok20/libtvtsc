#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tvtsc.h"
#include "cmpltrtok.h"

int main() {
	cmp_title("A fake simple demo of libtvtsc");

	// Hyper parameters
	cmp_title("Decide hyper params");
	const char* NAME = "tvts_fake_demo_simple";
	printf("Use below commnad to check the visualized curves:\n"
        "python3 tvts.py --host localhost --port 27017 -m \"loss|loss_val,top1|top1_val\" --batch_metrics \"loss,top1\" -k \"top1_val\" \"%s\"\n\n", NAME);
	double LR = 0.001, GAMMA = 0.95;
	int N_BATCH_SIZE = 256, N_EPOCHS = 10, N_SAMPLE_AMOUNT = 50000;

	// tvts init
	cmp_title("TVTS init");
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
		, "--save_dir", "/path/to/dir/for/saving/weights", // In this fake demo, we do not actually save weights; but in practice it is a must.

		"--params"
		, "batch_size", PARAMS_DICT_TYPE_I, N_BATCH_SIZE
		, "lr", PARAMS_DICT_TYPE_F, LR
		, "gamma", PARAMS_DICT_TYPE_F, GAMMA
		, "n_epoch", PARAMS_DICT_TYPE_I, N_EPOCHS
		// Example: Uncomment the following line will trigger error TVTS_ERROR_USING_RESERVED_KEYWORD, because of that "train_id" is a reserved key word.
//		, "train_id", PARAMS_DICT_TYPE_I, 1
	);
	if (ret) {
		fprintf(stderr, "Cannot init tvts struct! Error number is %d.", ret);
		exit(ret);
	}
	printf("TVTS initialized. TRAIN ID = %d\n", ptvts->train_id);

	// model and data
	// Actually no model and data, we just assume that there are N_SAMPLE_AMOUNT training data and the loss will decrease and the top1 metric will increase by each mini-batch.
	double LOSS_INIT = 5.0, LOSS_VAL_INIT = 6.5; // loss value in training of the training set and the val set
	double LOSS = LOSS_INIT, LOSS_VAL = LOSS_VAL_INIT;
	double TOP1 = 0, TOP1_VAL = 0;

	// train
	cmp_title("Train");
	int n_batches = (int)ceil((double)N_SAMPLE_AMOUNT / N_BATCH_SIZE);
	fprintf(stderr, "n_batches (per epoch): %d\n", n_batches);
	int epoch;
	char buf[MAXLINE];
	for (epoch = 1; epoch <= N_EPOCHS; ++epoch) {
		snprintf(buf, MAXLINE, "%d", epoch);
		cmp_title(buf);
		int batch;
		for (batch = 1; batch <= n_batches; ++batch) {
			// We assume below is the effect of the training in one mini-batch
			LOSS *= 0.9988;
			LOSS_VAL *= 0.9997;
			TOP1 = (LOSS_INIT - LOSS) / LOSS_INIT;
			TOP1_VAL = (LOSS_VAL_INIT - LOSS_VAL) / LOSS_VAL_INIT;

			// Use tvts to save info of an mini-batch
			ret = tvts_save_batch(
				ptvts
				, "--epoch", epoch
				, "--batch", batch
				, "--params"
				, "lr", PARAMS_DICT_TYPE_F, LR
				, "gamma", PARAMS_DICT_TYPE_F, GAMMA
				, "loss", PARAMS_DICT_TYPE_F, LOSS
				, "top1", PARAMS_DICT_TYPE_F, TOP1
				, "loss_val", PARAMS_DICT_TYPE_F, LOSS_VAL
				, "top1_val", PARAMS_DICT_TYPE_F, TOP1_VAL
				// Example: Uncomment his line will trigger error TVTS_ERROR_USING_RESERVED_KEYWORD, because of that "batch" is a reserved key word.
				//, "batch", PARAMS_DICT_TYPE_I, batch
			);
			if (ret) {
				fprintf(stderr, "tvts(%d): %s\n", ret, "Cannot insert into db when save batch data!");
				exit(ret);
			}

			// the progress bar
			printf(">");
			fflush(stdout);
			if (!(batch % 50)) {
				printf("\n");
			}
		}

		// one epoch is over
		printf("\n");
		printf("loss=%f, loss_val=%f, top1=%f, top1_val=%f\n", LOSS, LOSS_VAL, TOP1, TOP1_VAL);

		// Use tvts to save info of an epoch
		ret = tvts_save_epoch(
			ptvts
			, "--epoch", epoch
			, "--params"
			, "lr", PARAMS_DICT_TYPE_F, LR
			, "gamma", PARAMS_DICT_TYPE_F, GAMMA
			, "loss", PARAMS_DICT_TYPE_F, LOSS
			, "top1", PARAMS_DICT_TYPE_F, TOP1
			, "loss_val", PARAMS_DICT_TYPE_F, LOSS_VAL
			, "top1_val", PARAMS_DICT_TYPE_F, TOP1_VAL
			// Example: Uncomment his line will trigger error TVTS_ERROR_USING_RESERVED_KEYWORD, because of that "epoch" is a reserved key word.
			//, "epoch", PARAMS_DICT_TYPE_I, epoch
		);
		if (ret) {
			fprintf(stderr, "tvts(%d): %s\n", ret, "Cannot insert into db when save epoch data!");
			exit(ret);
		}

		// decay LR
        LR *= GAMMA;
	}

	cmp_title("All over");
}
