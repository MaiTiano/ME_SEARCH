#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#define INT_MAX       2147483647    /* maximum (signed) int value */

#define MVPRED_MEDIAN  0
#define MVPRED_UPLAYER 1



typedef unsigned char byte;    //!< byte type definition
typedef struct block
{
	int mbid; // the mb_nr from left to right, from top to bottom!
	int available;
	int blkx;  // block position
	int blky;
	int pmvx;  // block's predicted MV
	int pmvy;
	int psad;  // block's predicted SAD
	int Best_mvx;// Final best MV
	int Best_mvy;
	int Min_sad; //Final best SAD
} Block_jl;


int get_mem2D(byte ***array2D, int rows, int columns);
int get_mem3D(byte ****array3D, int frames, int rows, int columns);  
int get_mem2Dint(int ***array2D, int rows, int columns);
int check_MB_availability(int abcd_address, int current_mb_num, int mb_number_a_row, int mb_number_a_col);
void Record_MV_SAD(Block_jl *current_block, int curr_MB_addr);

void Stat_calculation(int offset_x, int offset_y, int SAD_center, int best_SAD, int SAD_AVG_pred, int SAD_MIN_pred, int SAD_MED_pred);

void free_mem2D(byte **array2D);
void free_mem2Dint(int **array2D);
void free_mem3D(byte ***array2D, int frames);




int pred_SAD_avg_flag;// for SAD prediction info    
int pred_SAD_avg; 
int pred_SAD_min_flag;// for SAD prediction info    
int pred_SAD_min; 
int pred_SAD_med_flag;// for SAD prediction info    
int pred_SAD_med; 
