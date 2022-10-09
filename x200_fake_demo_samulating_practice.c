// Note: Please refer to the equivalent python code in tvts/examples/x200_fake_demo_samulating_practice.py
//       to understand this piece of code if you feel it is complex.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include "tvtsc.h"
#include "cmpltrtok.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// model and data 1 of 2 (start)
// Actually there is no model and data, we just assume that there are N_SAMPLE_AMOUNT training data and the
// loss will decrease and the top1 metric will increase by each mini-batch.
// To translate the python code intuitively, I have to place these variables and functions here as global.
static int N_SAMPLE_AMOUNT = 50000;
static const double LOSS_INIT = 5.0, LOSS_VAL_INIT = 6.5, EPS = 1e-8;

static double xtop1();
static double xtop1_val();
static double LOSS = LOSS_INIT, LOSS_VAL = LOSS_VAL_INIT,
	TOP1 = 0., TOP1_VAL = 0.;

static void xload(const char *xpath) {
	FILE *pf = fopen(xpath, "r");
	if (!pf) {
		fprintf(stderr, "Cannot open file %s for xload!\n", xpath);
		exit(1);
	}
	fscanf(pf, "%lf %lf %lf %lf", &LOSS, &TOP1, &LOSS_VAL, &TOP1_VAL);
	fclose(pf);
}

static void xsave(const char *xpath) {
	FILE *pf = fopen(xpath, "w");
	if (!pf) {
		fprintf(stderr, "Cannot open file %s for xsave!\n", xpath);
		exit(1);
	}
	fprintf(pf, "%lf %lf %lf %lf\n", LOSS, TOP1, LOSS_VAL, TOP1_VAL);
	fclose(pf);
}

static double xtop1() {
	return (LOSS_INIT - LOSS + EPS) / LOSS_INIT;
}

static double xtop1_val(){
	return (LOSS_VAL_INIT - LOSS_VAL + EPS) / LOSS_VAL_INIT;
}

static void print_loss_and_top1() {
	printf("loss=%lf, loss_val=%lf, top1=%lf, top1_val=%lf\n", LOSS, LOSS_VAL, TOP1, TOP1_VAL);
}
// model and data 1 of 2 (end)
///////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	cmp_title("A fake demo of libtvtsc simulating real practice");
	int res = 0; // A flag of successful result.

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Hyper params and switches (start)
	cmp_title("Decide hyper params");
	const char *VER = "v1.0"; // version info of this code file
	const char *MEMO = cmp_cp_str(VER); // default memo is the version info
	double LR = 0.001; // default init learning rate
	double GAMMA = 0.95; // default multiplicative factor of learning rate decay per epoch
	printf("Default LR=%lf, GAMMA=%lf\n", LR, GAMMA);
	int IS_SPEC_LR = 0; // is manually specified the LR
	int IS_SPEC_GAMMA = 0; // is manually specified the GAMMA

	// default dir for saving weights
	const char *SAVE_DIR = 0;
	if (1) {
		char *exec_path = argv[0];
		char base_dir[MAXLINE];
		char *file_name = cmp_path_split(exec_path, base_dir, sizeof(base_dir)/sizeof(char));
		if (is_path_fully_provided(base_dir)) {
			SAVE_DIR = cmp_path_join_many(base_dir, "_save", file_name, VER);
		} else {
			char cwd[MAXLINE];
			if (!getcwd(cwd, sizeof(cwd)/sizeof(char))) {
				fprintf(stderr, "Cannot get CWD!\n");
				exit(1);
			}
			SAVE_DIR = cmp_path_join_many(cwd, base_dir, "_save", file_name, VER);
		}
		free(file_name);
	}

	// specify or override params from CLI
	// help
	static int is_show_help = 0;
	static int TEMP = 0;
	const char *usage_message =
"usage: x200_fake_demo_samulating_practice.py [-h] [--name NAME] [--memo MEMO]\n\
                                             [--temp] [-n EPOCHS]\n\
                                             [--batch BATCH] [--lr LR]\n\
                                             [--gamma GAMMA] [--pi PI]\n\
                                             [--pe PE] [--save_freq SAVE_FREQ]\n\
                                             [--save_dir SAVE_DIR]\n\
                                             [--init_weights INIT_WEIGHTS]\n\
                                             [--host HOST] [--port PORT]\n\
\n\
optional arguments:\n\
  --help            show this help message and exit\n\
  --name NAME           The name of this training, VERY important to TVTS.\n\
                        (default: tvts_py_example_x200_01)\n\
  --memo MEMO           The memo. (default: (no memo))\n\
  --temp                Run as temporary code (default: False)\n\
  -n EPOCHS, --epochs EPOCHS\n\
                        How many epoches to train. (default: 10)\n\
  --batch BATCH         Batch size. (default: 256)\n\
  --lr LR               Learning rate. (default: None)\n\
  --gamma GAMMA         Multiplicative factor of learning rate decay per\n\
                        epoch. (default: None)\n\
  --pi PI               id of the parent training (default: 0)\n\
  --pe PE               parent epoch of the parent training (default: 0)\n\
  --save_freq SAVE_FREQ\n\
                        How many epochs save weights once. (default: 2)\n\
  --save_dir SAVE_DIR   The dir where weights saved. (default: %s\n\
  --init_weights INIT_WEIGHTS\n\
                        The path to the stored weights to init the model.\n\
                        (default: None)\n\
  --host HOST           Host of the mongodb for tvts. (default: localhost)\n\
  --port PORT           Port of the mongodb for tvts. (default: 27017)\n\
\n";
	// defaults
	char *NAME = "tvts_fake_demo_practice_c";
	char *specified_memo = "(no memo)";
	int N_EPOCHS = 10, N_BATCH_SIZE = 256;
	int PARENT_TRAIN_ID = 0, PARENT_EPOCH = 0;
	int SAVE_FREQ = 2;
	char *INIT_WEIGHTS = 0;
	char *MONGODB_HOST = DEFAULT_HOST;
	int MONGODB_PORT = DEFAULT_PORT;
	// CLI options
	static struct option long_options[] = {
		// group #0
		{"help", no_argument, &is_show_help, 1},
		{"temp", no_argument, &TEMP, 1},

		// group #1
		{"name", required_argument, 0, '\x1'},
		{"memo", required_argument, 0, '\x2'},
		// group #2
		{"epochs", required_argument, 0, 'n'},
		{"batch", required_argument, 0, '\x3'},
		{"lr", required_argument, 0, '\x4'},
		{"gamma", required_argument, 0, '\x5'},
		// group #3
		{"pi", required_argument, 0, '\x6'},
		{"pe", required_argument, 0, '\x7'},
		{"save_freq", required_argument, 0, '\x8'},
		{"save_dir", required_argument, 0, '\x9'},
		// group #4
		{"init_weights", required_argument, 0, '\xA'},
		{"host", required_argument, 0, '\xB'},
		{"port", required_argument, 0, '\xC'},
		{0, 0, 0, 0}
	};
	// parse CLI options
	while(1) {
		int c;
		int option_index = 0;
		c = getopt_long(argc, argv,
			"\x1:" "\x2:"
			"n:" "\x3:" "\x4:" "\x5:"
			"\x6:" "\x7:" "x8:" "x9:"
			"\xA:" "\xB:" "\xC:",  long_options, &option_index);
		if (-1 == c) {
			break;
		}
		switch (c){
		case 0:
			break;
		case '\x1':
			if (optarg && optarg[0]) {
				NAME = cmp_cp_str(optarg);
			} else {
				fprintf(stderr, "The name of TVTS must not be empty!\n");
				exit(1);
			}
			break;
		case '\x2':
			if (optarg && optarg[0]) {
				specified_memo = cmp_cp_str(optarg);
				char *memo_old = (char*)MEMO;
				MEMO = cmp_strncat(MEMO, MAXLINE, "; ", specified_memo);
				free(memo_old);
			}
			break;
		case 'n':
			N_EPOCHS = atoi(optarg);
			if (N_EPOCHS <= 0) {
				fprintf(stderr, "The epoch number must be a positive integer!\n");
				exit(1);
			}
			break;
		case '\x3':
			N_BATCH_SIZE = atoi(optarg);
			if (N_BATCH_SIZE <= 0) {
				fprintf(stderr, "The batch size must be a positive integer!\n");
				exit(1);
			}
			break;
		case '\x4':
			LR = strtod(optarg, 0);
			if (LR <= 0) {
				fprintf(stderr, "The LR should be positive!\n");
				exit(1);
			}
			IS_SPEC_LR = 1;
			break;
		case '\x5':
			GAMMA = strtod(optarg, 0);
			if (!(GAMMA > 0 && GAMMA < 1)) {
				fprintf(stderr, "The GAMMA should be between 0 and 1 (excluding 0 and 1)!\n");
				exit(1);
			}
			IS_SPEC_GAMMA = 1;
			break;
		case '\x6':
			PARENT_TRAIN_ID = atoi(optarg);
			if (PARENT_TRAIN_ID < 0) {
				fprintf(stderr, "The parent train id must be a positive integer or zero!\n");
				exit(1);
			}
			break;
		case '\x7':
			PARENT_EPOCH = atoi(optarg);
			if (PARENT_EPOCH < 0) {
				fprintf(stderr, "The parent epoch must be a positive integer or zero!\n");
				exit(1);
			}
			break;
		case '\x8':
			SAVE_FREQ = atoi(optarg);
			if (SAVE_FREQ <= 0) {
				fprintf(stderr, "The frequency of weight-saving must be a positive integer!\n");
				exit(1);
			}
			break;
		case '\x9':
			if (optarg && optarg[0]) {
				SAVE_DIR = cmp_cp_str(optarg);
			} else {
				fprintf(stderr, "The dir to save weights must not be empty!\n");
				exit(1);
			}
			break;
		case '\xA':
			if (optarg && optarg[0]) {
				INIT_WEIGHTS = cmp_cp_str(optarg);
			} else {
				fprintf(stderr, "The path to initial weights must not be empty!\n");
				exit(1);
			}
			break;
		case '\xB':
			if (optarg && optarg[0]) {
				MONGODB_HOST = cmp_cp_str(optarg);
			} else {
				fprintf(stderr, "The host name of mongodb must not be empty!\n");
				exit(1);
			}
			break;
		case '\xC':
			MONGODB_PORT = atoi(optarg);
			if (MONGODB_PORT <= 0) {
				fprintf(stderr, "The port of mongodb must be a positive integer!\n");
				exit(1);
			}
			break;
		case '?':
			// getopt_long already printed an error message.
			printf(usage_message, SAVE_DIR);
			exit(1);
		default:
			fprintf(stderr, "Option parsing enters the default branch!\n");
			printf(usage_message, SAVE_DIR);
			exit(1);
		}
	}
	// post processing of CLI options
	// show help
	if (is_show_help) {
		printf(usage_message, SAVE_DIR);
		exit(0);
	}

	// the dir to save weights
	res = cmp_mkdir_p(SAVE_DIR, S_IRWXU);
	if (!res) {
		// cmp_mkdir_if_not_existed will tell the error.
		exit(1);
	} else {
		printf("Weights will be saved under this dir: %s\n", SAVE_DIR);
	}
	// init weights or parent training is mutual exclusion with each other
	if (PARENT_TRAIN_ID && INIT_WEIGHTS && INIT_WEIGHTS[0]) {
		fprintf(stderr, "You cannot specify parent_id=%d and init_weights=%s at the same time!\n", PARENT_TRAIN_ID, INIT_WEIGHTS);
		exit(1);
	}
	// Hyper params and switches (end)
	///////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// tvts init and resume (start)
	// tvts init
	cmp_title("TVTS init");
	tvts *ptvts = malloc(sizeof(tvts));
	int ret = tvts_init(
		ptvts,
		"--name", NAME,
		"--memo", MEMO,
		"--is_temp", TEMP,
		"--host", MONGODB_HOST,
		"--port", MONGODB_PORT,
		"--save_freq", SAVE_FREQ,
		"--save_dir", SAVE_DIR,
		"--init_weights", INIT_WEIGHTS,

		"--params",
		"ver", PARAMS_DICT_TYPE_S, VER,
		"batch_size", PARAMS_DICT_TYPE_I, N_BATCH_SIZE,
		"lr", PARAMS_DICT_TYPE_F, LR,
		"gamma", PARAMS_DICT_TYPE_F, GAMMA,
		"n_epoch", PARAMS_DICT_TYPE_I, N_EPOCHS
	);
	if (ret) {
		fprintf(stderr, "Cannot establish the TVTS object!\n");
		exit(ret);
	}
	printf("TVTS initialized. TRAIN ID = %d\n", ptvts->train_id);
	printf(
		"MONGODB_HOST=%s, "
		"MONGODB_PORT=%d, "
		"NAME=%s, "
		"MEMO=%s, "
		"TEMP=%d\n",
		MONGODB_HOST,
		MONGODB_PORT,
		NAME,
		MEMO,
		TEMP
	);
	params_dict_print(ptvts->params);
	printf(
		"SAVE_FREQ=%d, "
		"SAVE_DIR=%s, "
		"INIT_WEIGHTS=%s\n",
		SAVE_FREQ,
		SAVE_DIR,
		INIT_WEIGHTS
	);
	// tvts resume
	char *CKPT_PATH = 0;
	if(INIT_WEIGHTS && INIT_WEIGHTS[0]) {
		// Note: Implement the semantic of INIT_WEIGHTS is your burden.
		CKPT_PATH = INIT_WEIGHTS;
	} else {
		if (PARENT_TRAIN_ID) {
			cmp_title("TVTS resume");
			printf("PARENT_TRAIN_ID=%d, PARENT_EPOCH=%d\n", PARENT_TRAIN_ID, PARENT_EPOCH);
			char *rel_path;
			ret = tvts_resume(
				ptvts
				, PARENT_TRAIN_ID, PARENT_EPOCH
				, &rel_path, 0
				, 2 // There are 2 items to restore, i.e. lr and gamma as below.
				// Uncomment the followin g line to trigger this error: Key 'parent_epoch' is reserved and cannot be used by the user.
//				, "parent_epoch", PARAMS_DICT_TYPE_I
				, "lr", PARAMS_DICT_TYPE_F
				, "gamma", PARAMS_DICT_TYPE_F
			);
			if (ret) {
				fprintf(stderr, "Cannot resume parent training!\n");
				exit(ret);
			}
			CKPT_PATH = cmp_path_join(SAVE_DIR, rel_path);
			free(rel_path);
			if (!IS_SPEC_LR) {
				LR = params_dict_get(ptvts->params, "lr")->data.float_val;
			}
			if (!IS_SPEC_GAMMA) {
				GAMMA = params_dict_get(ptvts->params, "gamma")->data.float_val;
			}
		}
	}
	printf("CKPT_PATH=%s\n", CKPT_PATH);
	printf("Use below CLI command to visualize TVTS:\n");
	printf("python3 /path/to/tvts/tvts.py --host \"%s\" --port %d -m \"loss|loss_val,top1|top1_val\" --batch_metrics \"loss,top1\" -k \"top1_val\" --save_dir \"%s\" \"%s\"\n", MONGODB_HOST, MONGODB_PORT, SAVE_DIR, NAME);
	// tvts init and resume (end)
	///////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// model and data 2 of 2 (start)
	if (TEMP) {
		N_SAMPLE_AMOUNT = 500;
	}
	if (!CKPT_PATH) {
		cmp_title("From scratch");
		TOP1 = xtop1(), TOP1_VAL = xtop1_val();
	} else {
		cmp_title("Restore check point");
		printf("Loading weight from %s ...\n", CKPT_PATH);
		xload(CKPT_PATH);
		printf("Loaded.\n");
	}
	print_loss_and_top1();
	// model and data 2 of 2 (end)
	///////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// train (start)
	cmp_title("Train");
	if (TEMP) {
		cmp_title("(Temporary)");
	}
	int n_batches = (int)ceil((double)N_SAMPLE_AMOUNT / N_BATCH_SIZE);
	fprintf(stderr, "N_SAMPLE_AMOUNT: %d\n", N_SAMPLE_AMOUNT);
	fprintf(stderr, "N_BATCH_SIZE: %d\n", N_BATCH_SIZE);
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
			TOP1 = xtop1();
			TOP1_VAL = xtop1_val();

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
		print_loss_and_top1();
		// save the last epoch or by ts.save_freq
		// Note: Implement the semantic of save_freq and save_rel_path is your burden such as below.
		if (epoch == N_EPOCHS || !(epoch % ptvts->save_freq)) {
			char *save_prefix = tvts_get_save_name(ptvts, epoch);
			char save_rel_path[MAXLINE];
			snprintf(save_rel_path, MAXLINE, "%s.txt", save_prefix);
			char *save_path = cmp_path_join(ptvts->save_dir, save_rel_path);
			free(save_prefix);
			// actually do the saving task
			xsave(save_path);
			printf("Saved to : %s\n", save_path);
			free(save_path);
			// Use tvts to save info of an epoch
			ret = tvts_save_epoch(
				ptvts
				, "--epoch", epoch
				, "--save_rel_path", save_rel_path
				, "--save_dir", ptvts->save_dir
				, "--params"
				, "lr", PARAMS_DICT_TYPE_F, LR
				, "gamma", PARAMS_DICT_TYPE_F, GAMMA
				, "loss", PARAMS_DICT_TYPE_F, LOSS
				, "top1", PARAMS_DICT_TYPE_F, TOP1
				, "loss_val", PARAMS_DICT_TYPE_F, LOSS_VAL
				, "top1_val", PARAMS_DICT_TYPE_F, TOP1_VAL
			);
			if (ret) {
				fprintf(stderr, "tvts(%d): %s\n", ret, "Cannot insert into db when save epoch data!");
				exit(ret);
			}
		} else {
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
			);
			if (ret) {
				fprintf(stderr, "tvts(%d): %s\n", ret, "Cannot insert into db when save epoch data!");
				exit(ret);
			}
		}

		// decay LR
        LR *= GAMMA;
	}
	// train (end)
	///////////////////////////////////////////////////////////////////////////////////////////////////

	cmp_title("All over");
}
