#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "me_search.h"
#define MIN(a,b)  (((a) < (b)) ? (a) : (b)) 
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
/*Used in ME_SEARCH(), to determin the initial search position*/
//#define START_SEARCH_00
// OR
#define START_SEARCH_PRED


int blocksize = 4;
char seqName[20];	//argv[1]
int width, height;   //argv[2]£¬ argv[3]
int numFrames = 200;	//argv[4]
//int search_range /*= 16*/; 	//argv[5]
int refnum /*= 1*/;	//argv[6]


//typedef unsigned char byte;    //!< byte type definition

//For initSpiral() fuction.
int* sx;
int* sy;
int maxSp;

unsigned char** curFrameY;//current frame
unsigned char** curFrameU;
unsigned char** curFrameV;
unsigned char*** prevFrameY;//previous frame
unsigned char*** prevFrameU;
unsigned char*** prevFrameV;
unsigned char** searchwindow;//Search window for motion estimation

unsigned char** mpFrameY;
unsigned char** mpFrameU;
unsigned char** mpFrameV;

int * SAD_cost;                    //store SAD information needed for forward ref-frame prediction  SAD_cost[mb_address] = min_mcost
int **all_mv;  // all_mv[mb_address][0,1] = mv


unsigned char **in;
unsigned char **out;

FILE *seqFile;
FILE *fmv, *fmse; // two files to record the MV and SAD info of searched blocks.
FILE *p_stat_SAD_MV;// to be used in the function of Stat_calculation() 
/*
const int DIST[65]=
{
       0,    2,    8,   18,   32,   50,   72,   98, 
     128,  162,  200,  242,  288,  338,  392,  450, 
     512,  578,  648,  722,  800,  882,  968, 1058,
    1152, 1250, 1352, 1458, 1568, 1682, 1800, 1922, 
    2048, 2178, 2312, 2450, 2592, 2738, 2888, 3042, 
    3200, 3362, 3528, 3698, 3872, 4050, 4232, 4418, 
    4608, 4802, 5000, 5202, 5408, 5618, 5832, 6050, 
    6272, 6498, 6728, 6962, 7200, 7442, 7688, 7938, 
    8192
}; 
*/
/***********************************************************************
 *
 * \Input:
 *        framewidth: padded frame with one blocksize, 352+blocksize
 *        frameheight: padded frame with one blocksize, 288+blocksize
 * \Usage: to initialize some 2D or 3D memory.
 *
 * *********************************************************************/


int 
GetMemory(int framewidth, 
	  int frameheight, 
	  int SR/*, int blocksize*//*,int ALL_REF_NUM*/)
{
    //int i,j;
    int memory_size = 0;
    //int refnum = ALL_REF_NUM;
    int input_search_range = SR;

    memory_size += get_mem2D(&curFrameY, frameheight, framewidth); 
    memory_size += get_mem2D(&curFrameU, frameheight>>1, framewidth>>1); 
    memory_size += get_mem2D(&curFrameV, frameheight>>1, framewidth>>1); 

    memory_size += get_mem3D(&prevFrameY, refnum, frameheight, framewidth);// to allocate reference frame buffer accoding to the reference frame number 
    memory_size += get_mem3D(&prevFrameU, refnum, frameheight>>1, framewidth>>1); 
    memory_size += get_mem3D(&prevFrameV, refnum, frameheight>>1, framewidth>>1); 

    memory_size += get_mem2D(&mpFrameY, frameheight, framewidth); 
    memory_size += get_mem2D(&mpFrameU, frameheight>>1, framewidth>>1); 
    memory_size += get_mem2D(&mpFrameV, frameheight>>1, framewidth>>1); 

    memory_size += get_mem2D(&searchwindow, input_search_range*2 + blocksize, input_search_range*2 + blocksize);// to allocate search window according to the searchrange
   // memory_size += get_mem2D(&searchwindow, 80, 80);// if searchrange is 32, then only 32+1+32+15 pixels is needed in each row and col, therefore the range of 
    // search window is enough to be set to 80 !

    //memory_size += get_mem2Dint(&all_mv, frameheight/blocksize, framewidth/blocksize);

    return 0;
	
}


void 
FreeMemory(int refno)
{
    free_mem2D(curFrameY);
    free_mem2D(curFrameU);
    free_mem2D(curFrameV);

    free_mem3D(prevFrameY,refno);
    free_mem3D(prevFrameU,refno);
    free_mem3D(prevFrameV,refno);

    free_mem2D(mpFrameY);
    free_mem2D(mpFrameU);
    free_mem2D(mpFrameV);

    free_mem2D(searchwindow);

    //	free_mem2Dint(SAD_cost);
}




/********************************
 *
 *Release  2D memory 
 *
 ***************************** */
void 
free_mem2D(byte **array2D)
{ 
    if (array2D)
    {
	if (array2D[0])
	    free (array2D[0]);
	else 
	{
	    //error ("free_mem2D: trying to free unused memory",100);
	    fprintf(stderr,"free_mem2D: trying to free unused memory!");
	    free (array2D);
	}
    } 
    else
	{
	    //error ("free_mem2D: trying to free unused memory",100);
	    fprintf(stderr,"free_mem2D: trying to free unused memory!");
	}
}


/*!
 *  ************************************************************************
 *   * \brief
 *    *    free 2D memory array
 *     *    which was allocated with get_mem2Dint()
 *      ************************************************************************
 *       */
void 
free_mem2Dint(int **array2D)
{
    if (array2D)
    {
	if (array2D[0]) 
	    free (array2D[0]);
	else 
	{
	    //error ("free_mem2D: trying to free unused memory",100);
	    fprintf(stderr,"free_mem2Dint: trying to free unused memory!");
	    free (array2D);
	}
    } 
    else
    {
	//error ("free_mem2Dint: trying to free unused memory",100);
	fprintf(stderr,"free_mem2Dint: trying to free unused memory!");
    }
}
/********************************
 *
 *Release  3D memory 
 *
 ***************************** */
void 
free_mem3D(byte ***array3D, 
	   int frames)
{
    int i;
    if (array3D)
    {
	for (i=0;i<frames;i++)
	{ 		
	    free_mem2D(array3D[i]);
	}
	free (array3D);
    } 
    else
    {
	fprintf(stderr,"free_mem3D: trying to free unused memory!");
	//error ("free_mem3D: trying to free unused memory",100);
    }
}

/******************************************************************
 *
 * Feed frame into the buffer.
 * \Input: frame width, frame height, total reference frame number
 *
 ******************************************************************/
int 
bufferFrame(int W, 
	    int H/*, int Total_ref_num*/)
{
    int wi,hj;	//iterators witdth_i, height j 
    int fk;
    int width = W;
    int height= H;
    // int refnum = Total_ref_num;

    // --- Buffer previous frame , Only available when reference frame number > 1---
    for(fk=refnum; fk>1; fk--)
    {
	for(hj = 0; hj < height; hj++)
	    for(wi = 0; wi < width; wi++)
		prevFrameY[fk-1][hj][wi] = prevFrameY[fk-2][hj][wi];
	for(hj = 0; hj < height/2; hj++)
	    for(wi = 0; wi < width/2; wi++)
		prevFrameU[fk-1][hj][wi] = prevFrameU[fk-2][hj][wi];
	for(hj = 0; hj < height/2; hj++)
	    for(wi = 0; wi < width/2; wi++)
		prevFrameV[fk-1][hj][wi] = prevFrameV[fk-2][hj][wi];
    }

    // --- Read one frame from sequence file ---
    //  Not only curFrameY but also prevFrameY has been given values!
    for(hj = 0; hj < height; hj++)
	for(wi = 0; wi < width; wi++)
	{
	    prevFrameY[0][hj][wi] = curFrameY[hj][wi];
	    curFrameY[hj][wi] = fgetc(seqFile);
	}

 //[INFO]
 //fprintf(stderr,"[JL-INFO7]--------------------------------Read one frame from seqFile-------------------------------\n");

    for(hj = 0; hj < height/2; hj++)
	for(wi = 0; wi < width/2; wi++)
	{
	    prevFrameU[0][hj][wi] = curFrameU[hj][wi];
	    curFrameU[hj][wi] = fgetc(seqFile);
	}

    for(hj = 0; hj < height/2; hj++)
	for(wi = 0; wi < width/2; wi++)
	{
	    prevFrameV[0][hj][wi] = curFrameV[hj][wi];
	    curFrameV[hj][wi] = fgetc(seqFile);
	}

    return 0;
}


int 
initSpiral(int SR)
{
    int i, k, l;
    int input_search_range = SR;

    maxSp = (input_search_range*2+1)*(input_search_range*2+1);
    sx = (int*) calloc( maxSp, sizeof(int));
    if (sx == NULL)	return 1;
    sy = (int*) calloc( maxSp, sizeof(int));
    if (sy == NULL)	return 1;

    sx[0] = 0;
    sy[0] = 0;

    for (k=1, l=1; l <= input_search_range; l++)
    {
	for (i=-l+1; i< l; i++)
	{
	    sx[k] =  i;  sy[k++] = -l;
	    sx[k] =  i;  sy[k++] =  l;
	}
	    for (i=-l;   i<=l; i++)
	    {
		sx[k] = -l;  sy[k++] =  i;
		sx[k] =  l;  sy[k++] =  i;
	    }
	}
    return 0;
}

/*!**************************************************************************************************
 * 
 * Load search window is the possible search area for the motion estimation.
 * This search area will center at the position defined by the pred_mv_x & pred_mv_y
 * 
 *  
 * Input: current block_x, block_y, pred_mvx, pred_mvy, ref_frame_no, input_search_range, 
 *
 * **************************************************************************************************
*/
int 
LoadSearchWindow(int center_x, 
	         int center_y, 
		 int ref_frame, 
		 int SR_input
		 /*, int pic_width, int pic_height*/)
{
    int i, j;
    //int search_center[5][2]; // reference frame no up to 5. 2 components x and y.

    //search_center[ref_frame-1];

    /*/////////////////////////////////////////////////////////////////// 
    // Method 1. To load search window according to input search range
    *////////////////////////////////////////////////////////////////////

    for(j = center_y - SR_input; j < center_y + SR_input + blocksize; j++)
	for(i = center_x - SR_input; i < center_x + SR_input + blocksize; i++)
	{  
	    if( (i>=0) && (i < width) && (j>=0) && (j<height) )
		searchwindow[j-(center_y - SR_input)][i-(center_x - SR_input)] = prevFrameY[ref_frame][j][i];
	    else 
		searchwindow[j-(center_y - SR_input)][i-(center_x - SR_input)] = 0;
	}

    /*/////////////////////////////////////////////////////////////////////
     * Method 2. To load a fixed size of search window with width=height=80
     *////////////////////////////////////////////////////////////////////

    /* 
       for(j = center_y - 32; j < center_y + 48; j++)
       for(i = center_x - 32; i < center_x + 48; i++)
       {  
       if( (i>=0) && (i < width) && (j>=0) && (j<height) )
       searchwindow[j-(center_y - SR_input)][i-(center_x - SR_input)] = prevFrameY[ref_frame][j][i];
       else 
       searchwindow[j-(center_y - SR_input)][i-(center_x - SR_input)] = 0;

       }*/

    return 0;
}



/*!**************************************************************************************************
 * 
 * In this function surrounding MV and SAD are combined to get the MV prediction and SAD prediction
 *
 * Input: current block_x, block_y, block_size(fixed here), 
 * **************************************************************************************************
 */
int 
Set_MV_SAD_PRED(int bx, 
	        int by, 
		int curr_mb_nr, 
		int mb_number_row, 
		int mb_number_col, 
		short pmv[2]/*, short psad*/)
{
    int SAD_a, SAD_b, SAD_c, SAD_d;
    int mv_a, mv_b, mv_c, mv_d;
    int pred_vector;
    Block_jl block_a, block_b, block_c, block_d;
    int mb_nr/*, row, col*/;
    int SAD_SUM = 0;
    int SAD_count_no = 0;
    int hv;
    int mv_pred_type;

    SAD_a = 0;
    SAD_b = 0;
    SAD_c = 0;
    SAD_d = 0;
    /*
     * mb_num_row = pic_h/blocksize;
     * mb_num_col = pic_w/blocksize;
     *
     * mb_nr = (by/blocksize)*mb_number_row + bx/blocksize; // up-left corner MB is 0, then 1~21 in the first row and so on!
     */
    mb_nr = curr_mb_nr;

    block_a.mbid = mb_nr - 1;
    block_b.mbid = mb_nr - mb_number_row;
    block_c.mbid = mb_nr - mb_number_row + 1;
    block_d.mbid = mb_nr - mb_number_row - 1;

    // --- Function Call ---
    // Input: the neighboring block id, the current block id. Both of them are the MB index number
    block_a.available = check_MB_availability(block_a.mbid, mb_nr, mb_number_row, mb_number_col);
    block_b.available = check_MB_availability(block_b.mbid, mb_nr, mb_number_row, mb_number_col);
    block_c.available = check_MB_availability(block_c.mbid, mb_nr, mb_number_row, mb_number_col);
    block_d.available = check_MB_availability(block_d.mbid, mb_nr, mb_number_row, mb_number_col);

    /* Refine the block availibility test, i.e. block 21 is not the left block of 22!!!!!*/
    /*[1] The most left column*/
    if(mb_nr%22 == 0)
	block_a.available = 0;
    /*[2] The most right column*/
    if((mb_nr + 1)%22 == 0)
	block_c.available = 0;

    /*for print [INFO4]*/
    block_a.mbid = block_a.available?block_a.mbid:-1000;
    block_c.mbid = block_c.available?block_c.mbid:-1000;
    //[INFO]
    //fprintf(stderr, "[JL-INFO4 :358] %d,%d,%d || mb_nr %d\n", block_a.mbid, block_b.mbid, block_c.mbid, mb_nr);





    if(!block_c.available)
    {
	block_c = block_d;
    }

    ///////////////////////////////////////////////////////////
    // ---[1] To get the SAD value of the neighboring blocks ---
    //        then compute the sad_pred
    //        (1.AVERAGE, 2.MEDIAN, 3.UPLAYER)
    ///////////////////////////////////////////////////////////
    SAD_a = block_a.available?(SAD_cost[block_a.mbid]):0;
    SAD_b = block_b.available?(SAD_cost[block_b.mbid]):0;
    SAD_d = block_d.available?(SAD_cost[block_d.mbid]):0;
    SAD_c = block_c.available?(SAD_cost[block_c.mbid]):SAD_d;

    // ---(1.1) Average SAD ---
    if(block_a.available == 1)
    {
	SAD_SUM += SAD_cost[block_a.mbid];
	SAD_count_no +=1;
    }
    if(block_b.available == 1)
    {
	SAD_SUM += SAD_cost[block_b.mbid];
	SAD_count_no +=1;
    }
    if(block_c.available == 1)
    {
	SAD_SUM += SAD_cost[block_c.mbid];
	SAD_count_no +=1;
    }
    else if(block_d.available == 1)
    {
	SAD_SUM += SAD_cost[block_d.mbid];
	SAD_count_no +=1;
    }

    if(SAD_count_no == 0)
    {
	pred_SAD_avg_flag = 0;
	pred_SAD_avg = 1000000;
    }
    else
    {
	pred_SAD_avg_flag = 1;
	pred_SAD_avg = SAD_SUM/SAD_count_no;
    }


    // ---(1.2) Min SAD ---

    if((block_a.available == 0) && (block_b.available == 0) && (block_c.available == 0)) // the upper-left 1st block
    {
	pred_SAD_min = 1000000;
	pred_SAD_min_flag = 0;
    }
    else if((block_a.available == 1) && (block_b.available == 0) && (block_c.available == 0)) // the 1st row, except for 1st block
    {
	pred_SAD_min = SAD_cost[block_a.mbid];
	pred_SAD_min_flag = 1;
    }
    else if((block_a.available == 0) && (block_b.available == 1) && (block_c.available == 1)) // the 1st col, except for 1st block
    {
	pred_SAD_min = MIN(SAD_cost[block_b.mbid], SAD_cost[block_c.mbid]);
	pred_SAD_min_flag = 1;
    }
    else if((block_a.available == 1) && (block_b.available == 1) && (block_c.available == 0)) // the last col
    {
	pred_SAD_min = MIN(SAD_cost[block_a.mbid], SAD_cost[block_b.mbid]);
	pred_SAD_min_flag = 1;
    }
    else 
    {
	pred_SAD_min = MIN(SAD_cost[block_a.mbid], MIN(SAD_cost[block_b.mbid], SAD_cost[block_c.mbid]));
	pred_SAD_min_flag = 1;
    }    


    // ---(1.3) Median SAD ---()

    if((block_a.available == 0) && (block_b.available == 0) && (block_c.available == 0)) // the upper-left 1st block
    {
	pred_SAD_med = 1000000;
	pred_SAD_med_flag = 0;
    }
    else if((block_a.available == 1) && (block_b.available == 0) && (block_c.available == 0)) // the 1st row, except for 1st block
    {
	pred_SAD_med = SAD_cost[block_a.mbid];
	pred_SAD_med_flag = 1;
    }
    else if((block_a.available == 0) && (block_b.available == 1) && (block_c.available == 1)) // the 1st col, except for 1st block
    {
	// if a not, b and c ok, then sad_med = (sad_b+sad_c)/2
	pred_SAD_med = (SAD_cost[block_b.mbid] + SAD_cost[block_c.mbid])>>1;


	pred_SAD_med_flag = 1;
    }
    else if((block_a.available == 1) && (block_b.available == 1) && (block_c.available == 0)) // the last col
    {
	// if a,b, then sad_med = sad_a
	pred_SAD_med = SAD_cost[block_a.mbid];
	pred_SAD_med_flag = 1;
    }
    else 
    {
	pred_SAD_med = SAD_cost[block_a.mbid] + SAD_cost[block_b.mbid] + SAD_cost[block_c.mbid]- 
	    MIN(SAD_cost[block_a.mbid], MIN(SAD_cost[block_b.mbid], SAD_cost[block_c.mbid]))-
	    MAX(SAD_cost[block_a.mbid], MAX(SAD_cost[block_b.mbid], SAD_cost[block_c.mbid]));
	pred_SAD_med_flag = 1;
    }    



    /* [INFO]*/
    //    fprintf(stderr, "[JL-INFO9] Curr_MB[%d], Surrouding:mb_nr{%d, %d, %d}| best_sad{%d, %d, %d}, pred_SAD_med: %d \n",mb_nr, block_a.mbid, block_b.mbid, block_c.mbid, SAD_cost[block_a.mbid], SAD_cost[block_b.mbid], SAD_cost[block_c.mbid], pred_SAD_med); 





    /////////////////////////////////////////////////////////////////////
    //--- if available, [2] get MV, then compute the mv_pred (MEDIAN)
    /////////////////////////////////////////////////////////////////////
    mv_pred_type = MVPRED_MEDIAN;   

    for(hv=0; hv<2; hv++)
    {
	mv_a = block_a.available ? all_mv[block_a.mbid][hv]:0; 
	mv_b = block_b.available ? all_mv[block_b.mbid][hv]:0; 
	mv_c = block_c.available ? all_mv[block_c.mbid][hv]:0; 
	mv_d = block_d.available ? all_mv[block_d.mbid][hv]:0; 

	switch(mv_pred_type)
	{
	    case MVPRED_MEDIAN:
		if(!(block_b.available || block_c.available))// b || c 
		{
		    /*
		       if(block_a.available)
		       {
		       pred_vector = mv_a;
		       }
		       else
		       {
		       pred_vector = 0;
		       }
		       */
		    pred_vector = mv_a; // this centence has the same function as the previous 9 lines of program
		}
		else
		{
		    pred_vector = mv_a + mv_b + mv_c - MIN(mv_a,MIN(mv_b,mv_c))-MAX(mv_a,MAX(mv_b,mv_c));
		}
		break;
	    case MVPRED_UPLAYER:
		break;
	}

	pmv[hv] = pred_vector;
    }
    return 0;

}


/*************************************************************
 *
 * return 1 if the macroblock at the given address is available
 *
 **************************************************************/

int 
check_MB_availability(int abcd_address, 
	              int current_mb_num, 
		      int mb_number_a_row, 
		      int mb_number_a_col)
{
    if ((abcd_address < 0) || (abcd_address > (mb_number_a_row*mb_number_a_col - 1)))
	return 0;

    return 1;

}



/***************************************************************
 * \Fuction:
 *  Dynamic search range decision
 *  
 *  To decide a adaptive search range for the search window
 *     loading and motion search.
 *
 * \Input:
 *  input_SR, 
 * 
 * \Output:
 *  dsr
 *
 ****************************************************************/
int DYNAMIC_SEARCH_RANGE(int input_SR)
{
    int dsr; /*output dynamic search range*/

    /*if qcif*/
 
    return 0;

}











/********************************************************************************
 *
 * This is the core search process 
 *  
 * This function deal with the whole frame, every MB will be processed within 
 * this process. 
 *
 ****************************************************************************** */
int 
ME_SEARCH(int SR, 
	  int MB_NO_A_ROW, 
	  int MB_NO_A_COL)
{
    Block_jl currblk/*, block_a, block_b, block_c*/;

    /*Current search range for the current block is initialized to be the input SR
     * it will be changed after Dynamic search range scheme used*/
    int current_search_range = SR;
    int mvx, mvy, mvt;//1
    int qmvx, qmvy;
    //int blkx, blky;
    int shift_x, shift_y;
    int x, y;
    int i, j, k;
    int t;//2
    int min_sad = INT_MAX;/*16777216*/;
    int sad = 0;
    int mb_nr; // this is the MB index in the frame. 
    short pred_mv[2], pred_mv_x, pred_mv_y;
    int search_center_x, search_center_y;
    int sad_of_search_center; 
    //	short pred_sad;
    int dsr; //dynamic search range

    // Initilize two buffer, and flush them at the end of this function. These two buffer recored the MV and SAD info of 
    // all the searched blocks for the current frame ONLY  !!!!!
    SAD_cost = (int*)malloc(sizeof(int)*MB_NO_A_ROW*MB_NO_A_COL);
    get_mem2Dint(&all_mv, MB_NO_A_ROW*MB_NO_A_COL, 2);  // 352*288/16/16 is the MB number  

    for(k=0;k<2;k++)
	pred_mv[k]=0;

    //--- for every MB --- {1}
    for(currblk.blky = 0; currblk.blky < height; currblk.blky += blocksize)
	for(currblk.blkx = 0; currblk.blkx < width; currblk.blkx += blocksize)//loop over macro blocks (16x16)
	{
	    //[INFO] 
	    //fprintf(stderr, "\n\n\n[JL-INFO5] a new block\n\n\n");
	    min_sad = INT_MAX;/*16777216*/;
	    mvx = 0;
	    mvy = 0;
	    qmvx = 0;
	    qmvy = 0;
	    sad_of_search_center = 0; /*search center's sad value. the first one!*/

	    //pred_sad = 0;
	    //t = 0;//3

	    mb_nr = (currblk.blky/blocksize)*MB_NO_A_ROW + currblk.blkx/blocksize; // up-left corner MB is 0, then 1~21 in the first row and so on!

	    Set_MV_SAD_PRED(currblk.blkx, currblk.blky, mb_nr, MB_NO_A_ROW, MB_NO_A_COL, pred_mv/*, pred_sad*/);
	     
	    //[INFO]
	    //fprintf(stderr, "[JL-INFO5.1] Current MB number is: %d \n", mb_nr); 
	    //[INFO]
	    //fprintf(stderr, "[JL-INFO1.1] pred_mv should be changed here  [%3d,%3d]\n", pred_mv[0], pred_mv[1]);
	    //[INFO]
	    //fprintf(stderr, "[JL-INFO2.1] pred_SAD_avg [%5d]\n", pred_SAD_avg);

	    /***********************************  
	     **                               **
	     **  Two start point method:      **
	     **  1. [pred_mvx, pred_mvy]      **
	     **  2. [0,0]                     **
	     **                               **
	     * ********************************/
	    //{Method 1: Search first point(0,0)}
	    //No pred_mv used!  (0,0) position is the initial search point!
#ifdef START_SEARCH_PRED

	    pred_mv_x = pred_mv[0];
	    pred_mv_y = pred_mv[1];

#endif

	    //{Method 2: Search first point(0,0)}
	    //No pred_mv used!  (0,0) position is the initial search point!
#ifdef START_SEARCH_00

	    pred_mv_x = 0;
	    pred_mv_y = 0;

#endif

	    search_center_x = currblk.blkx + pred_mv_x;
	    search_center_y = currblk.blky + pred_mv_y;

	    /***********************************************************
	     * 
	     * Motion estimation process are seperated into two parts: 
	     * [ME_1: search center position], 
	     * [ME_2: left position except for the search center]
	     * 
	     * \Date: 2010/05/225
	     **********************************************************/	    

	    /*[ME_1: search the center position]*/

	    for(t=0; t < refnum; t++)//4
	    {
		/***************************    [ME_1: only search center]    ******************************/
		
		// {Note}In order to only search the pixels around the search center, here the search window is set to zero
		//       therefore, in the LoadSearchWindow()function, only a block with the up-left corner locating at 
		//       search center is loaded
	        LoadSearchWindow(search_center_x, search_center_y, t, 0 /*current_search_range, width, height*/);//5
		
		k = 0;
	
		x = /*32*//*current_search_range*/0 + sx[k]; // k=0 , center is the (32,32) position in the search window. 	
		y = /*32*//*current_search_range*/0 + sy[k];

		//if( (x >= 0) && (y >= 0) && (x < 2*current_search_range/*80 - blocksize*/) && (y < current_search_range*2/*80 - blocksize*/) )
		{
		    sad = 0;

		    for(j = 0; j<blocksize; j++)		//Calculate block distortion
		    {
			for(i = 0; i<blocksize; i+=1)
			{
			    sad += abs(curFrameY[currblk.blky+j][currblk.blkx+i] - searchwindow[y+j][x+i]);// 
			}
			if(sad > min_sad)
			    break;
		    }

		    if(sad < min_sad) // set mv
		    {
			min_sad = sad;

			/* It should be noticed here: 2010.5.7
			 * Since the predited position is used here as the search center. So, the actural search is performed around the search center not the (0,0) position.
			 * Therefore, the final MV should not only include the full search spiral part, but also include the new search center shift(pred_mvx, pred_mvy)
			 *
			 * */
			/*(1) Old version of (mvx,mvy) computation method, it is suitable for situation in which the (0,0) is used as the search center */

			//mvx = sx[k];
			//mvy = sy[k];

			/*(2) New version of (mvx,mvy) computation method, it is suitable for situation in which the (pred_mvx,pred_mvy) is used as the search center */
			mvx = sx[k] + pred_mv_x;
			mvy = sy[k] + pred_mv_y;

			mvt = t;//6

			for(j = 0; j < blocksize; j++)
			    for(i = 0; i < blocksize; i++)
			    {
				mpFrameY[currblk.blky+j][currblk.blkx+i] = searchwindow[y+j][x+i];
			    }
		    }
		}

		/* SAD value @ initial search point */
		if(k == 0)
		    sad_of_search_center = min_sad;





		/**************************************[ME_2: search the the rest of positions]***********************************/


		/*[DSR] to change the search range dynamically according to the SAD ratio*/
		////DYNAMIC_SEARCH_RANGE(pred_SAD_med,);

		////current_search_range = 
		/*[DSR 1:] fixed sr*/
	        LoadSearchWindow(search_center_x, search_center_y, t , current_search_range/*, width, height*/);//5
		/*[DSR 2:] DSR sr*/
		//LoadSearchWindow(search_center_x, search_center_y, t , dsr/*, width, height*/);//5


		for(k=1; k<maxSp; k++) //loop over full pel search points (spiral)
		{
		    // x, y is the center of search window, 
		    // according to the search window load method 1 and 2. (IN LINE 288.)
		    // the x,y will be calculated in different ways.

		    //--- Method 2. <-> LINE 288 method 2 ---
		    x = /*32*/current_search_range + sx[k]; // k=0 , center is the (32,32) position in the search window. 	
		    y = /*32*/current_search_range + sy[k];

		    if( (x >= 0) && (y >= 0) && (x < 2*current_search_range/*80 - blocksize*/) && (y < current_search_range*2/*80 - blocksize*/) )
		    {
			sad=0;

			for(j = 0; j<blocksize; j++)		//Calculate block distortion
			{
			    for(i = 0; i<blocksize; i+=1)
			    {
				sad += abs(curFrameY[currblk.blky+j][currblk.blkx+i] - searchwindow[y+j][x+i]);// 
			    }
			    if(sad > min_sad)
				break;
			}

			if(sad < min_sad) // set mv
			{
			    min_sad = sad;

			    /* It should be noticed here: 2010.5.7
			     * Since the predited position is used here as the search center. So, the actural search is performed around the search center not the (0,0) position.
			     * Therefore, the final MV should not only include the full search spiral part, but also include the new search center shift(pred_mvx, pred_mvy)
			     *
			     * */
			    /*(1) Old version of (mvx,mvy) computation method, it is suitable for situation in which the (0,0) is used as the search center */
			    
			    //mvx = sx[k];
			    //mvy = sy[k];
			    
			    /*(2) New version of (mvx,mvy) computation method, it is suitable for situation in which the (pred_mvx,pred_mvy) is used as the search center */
			    mvx = sx[k] + pred_mv_x;
			    mvy = sy[k] + pred_mv_y;
			    
			    mvt = t;//6

			    for(j = 0; j < blocksize; j++)
				for(i = 0; i < blocksize; i++)
				{
				    mpFrameY[currblk.blky+j][currblk.blkx+i] = searchwindow[y+j][x+i];
				}
			}
		    }


		}
			    // end of full pel search

		/*!
		 *  Quarter Pixel Search Part
			     *

				for(k=0; k<((z-1)*2+1)*((z-1)*2+1); k++) //loop over quad-pel search points 
				{
				x = 32 + z_mvx*z + sx[k];	//approximated location for quad pel
				y = 32 + z_mvy*z + sy[k];

				if( (x >= 0) && (y >= 0) && (x < 80-blocksize*z) && (y < 80-blocksize*z) )
				{
				sad=0;
				for(i = 0; i<blocksize; i++)		//Calculate block distortion
				{
					for(j = 0; j<blocksize; j+=1)
					{
		//if(z==16)
		//fprintf(stderr, "x+i*z = %d, y+j*z = %d\n", x+i*z, y+j*z);
		sad += abs(curFrameY[blkx+i][blky+j]-searchwindow[x+i*z][y+j*z]);
		}
		if(sad>min_sad)
		break;
		}
		if(sad<min_sad)
		{
		min_sad = sad;
		qmvx = sx[k];
		qmvy = sy[k];
		mvz = z;
		mvt = t;//7
		for(i = 0; i < blocksize; i++)
		for(j = 0; j < blocksize; j++)
		{
		mpFrameY[blkx+i][blky+j] = searchwindow[x+i*z][y+j*z];
		}
		}
		}
		}

*/

	    }//--- EOF for every reference frame loop --- 




	    currblk.Min_sad = min_sad;
	    currblk.Best_mvx = mvx;
	    currblk.Best_mvy = mvy;

	    /* ---[JL-Stat] MV_X --- */
	    shift_x = mvx - pred_mv_x;
            shift_y = mvy - pred_mv_y;

	    //[INFO]
	    //fprintf(stderr, "[JL-INFO3] SAD @ searchcenter is %d \n", sad_of_search_center);
	    //fprintf(stderr, "mvx = %3d, mvy = %3d, qmvx = %3d, qmvy = %3d, zoom = %3d, multiple_frame = %3d, min_sad = %d\n", mvx, mvy, qmvx, qmvy, mvz, mvt, min_sad);//8
	    //[INFO]	
	    //fprintf(stderr, "[JL-INFO6] mvx = %3d, mvy = %3d, best_ref = %3d, min_sad = %d | shiftx = %3d, shify = %3d\n", mvx, mvy, mvt, min_sad, shift_x, shift_y);//8


	    /*[JL-STAT] output to stderr, mvx, mvy, sad_center, sad_avg,....*/
	    
	     
	    /* {Output_method1}*/ 
	    /* Output shift_x and shift_y*/ 
	    
	    fprintf(stderr, "%3d %3d %5d %5d %5d %5d %5d\n", shift_x, shift_y, sad_of_search_center, min_sad, pred_SAD_avg, pred_SAD_min, pred_SAD_med);//8
	    
	    /* {Output_method2} */
	    /* Output mv_x and mv_y */
	    //fprintf(stderr, "%3d %3d %5d %5d %5d %5d %5d\n", mvx, mvy, sad_of_search_center, min_sad, pred_SAD_avg, pred_SAD_min, pred_SAD_med);//8
	    
	    
	    /*************************************************
	     * \Function Call:
	     * To get the statistics data for the MV and SAD
	     * analysis. 
	     * Date: 2010-05-18
	     *************************************************/
	    Stat_calculation(shift_x, shift_y, sad_of_search_center, min_sad, pred_SAD_avg, pred_SAD_min, pred_SAD_med); 


	    /*****************************************************************************************
	     * \Function Call:
	     * Tto store the best mv_x, mv_y and min_sad of the current block into the global buffer
	     * Date: 2010-05-18
	     ****************************************************************************************/
	    Record_MV_SAD(&currblk, mb_nr);

	}// --- EOF for every MB --- {1}

    // free once every search one frame, because SAD_cost only record the searched blocks' SAD value for the current frame!!!!
    free(SAD_cost); 
    free_mem2Dint(all_mv);

    return (0);
}

/****************************************
 *
 * Record the searched block's info into 
 * the global buffer for later use. 
 *
 ****************************************/

//void Record_MV_SAD(int block_x, int block_y, int mv_x, int mv_y, int min_mcost)
void 
Record_MV_SAD(Block_jl *current_block, 
	      int curr_MB_addr)
{
    //int block_x = current_block->blkx;
    //int block_y = current_block->blky;
    /* 
       SAD_cost[block_y][block_x] =  current_block.Min_sad;
       all_mv[block_y][block_x][0] = current_block.Best_mvx;
       all_mv[block_y][block_x][1] = current_block.Best_mvy;
       */	

    SAD_cost[curr_MB_addr] =  current_block->Min_sad;
    all_mv[curr_MB_addr][0] = current_block->Best_mvx;
    all_mv[curr_MB_addr][1] = current_block->Best_mvy;

}

/**********************************************************************
 *
 * \Fuction define:
 *  To get the according stat data for SAD_center/SAD_pred analysis, and
 *  MV features analysis.
 *  After calculation, the analysis data will be write into a stat file.
 * 
 * \Date: 2010-05-18
 * 
 *********************************************************************/

//int
void
Stat_calculation(int offset_x, 
	         int offset_y, 
		 int SAD_center, 
		 int best_SAD, 
		 int SAD_AVG_pred, 
		 int SAD_MIN_pred, 
		 int SAD_MED_pred)
{
    int d, i;
    float r_sad_center_avg, r_sad_center_min, r_sad_center_med;

    /*0. To calculate the Euclide Distance of the best MV*/
    d = offset_x*offset_x + offset_x*offset_y;

    /*1. AVG_SAD part*/
    if((SAD_AVG_pred == 0)||(SAD_AVG_pred == 1000000))
    {
	r_sad_center_avg = -1.00;
    }
    else
    {
	r_sad_center_avg = (float)SAD_center/SAD_AVG_pred;
    }

    /*2. MIN_SAD part*/
    if((SAD_MIN_pred == 0)||(SAD_MIN_pred == 1000000))
    {
	r_sad_center_min = -1.00;
    }
    else
    {
	r_sad_center_min = (float)SAD_center/SAD_MIN_pred;
    }

    /*3. MED_SAD part*/
    if((SAD_MED_pred == 0)||(SAD_MED_pred == 1000000))
    {
	r_sad_center_med = -1.00;
    }
    else
    {
	r_sad_center_med = (float)SAD_center/SAD_MED_pred;
    }

    /*Write three ratio data into the file*/
    if((p_stat_SAD_MV = fopen("stat_SAD_MV.txt", "r")) == 0) // check if the file exist
    {
	if((p_stat_SAD_MV = fopen("stat_SAD_MV.txt", "a")) == NULL) // append new statistic at the end
	{
	    fprintf(stderr,"Error Open File [1]!\n");
	    //snprintf(errortext, ET_SIZE, "")
	}
	else
	{}
    }
    else
    {
	fclose(p_stat_SAD_MV);
	if((p_stat_SAD_MV = fopen("stat_SAD_MV.txt", "a")) == NULL)
	{
	    fprintf(stderr,"Error Open File [2]!\n");
	    //snprintf(errortext, ET_SIZE, "")
	}
    }

    fprintf(p_stat_SAD_MV, "%d %5.3f %5.3f %5.3f\n", d, r_sad_center_avg, r_sad_center_min, r_sad_center_med);

    fclose(p_stat_SAD_MV);
 
  //  return 0;


}


/*********************************************************************
 *\Function define:
 * To get 2D memory by calloc
 *********************************************************************/
int 
get_mem2D(byte ***array2D, 
	  int rows, 
	  int columns)
{
    int i;

    if((*array2D      = (byte**)calloc(rows,        sizeof(byte*))) == NULL)   //byte unsigned char
    {
	fprintf(stderr,"Out of memory");
	return 1;
    }
    if(((*array2D)[0] = (byte* )calloc(columns*rows,sizeof(byte ))) == NULL)
    {
	fprintf(stderr,"Out of memory");
	return 1;
    }

    for(i=1;i<rows;i++)
	(*array2D)[i] = (*array2D)[i-1] + columns ;

    return rows*columns;

}

/*!

 *************************************************************************
 ** \brief
 *   Allocate 2D memory array -> int array2D[rows][columns]
 **
 ** \par Output:
 **    memory size in bytes
 *************************************************************************
 */
int 
get_mem2Dint(int ***array2D, 
	     int rows, 
	     int columns)
{
    int i;

    if((*array2D      = (int**)calloc(rows,        sizeof(int*))) == NULL)
    {
	fprintf(stderr,"Out of memory");
	return 1;
    }
    if(((*array2D)[0] = (int* )calloc(rows*columns,sizeof(int ))) == NULL)
    {
	fprintf(stderr,"Out of memory");
	return 1;
    }

    for(i=1 ; i<rows ; i++)
	(*array2D)[i] =  (*array2D)[i-1] + columns;
    return rows*columns*sizeof(int);
}




/*! 
 ************************************************************************
 * \brief
 *    Allocate 3D memory array -> unsigned char array3D[frames][rows][columns]
*
* \par Output:
*    memory size in bytes
************************************************************************
*/
int 
get_mem3D(byte ****array3D, 
 	  int frames, 
	  int rows, 
	  int columns)
{
    int  j; 

    if(((*array3D) = (byte***)calloc(frames,sizeof(byte**))) == NULL)
    {
	fprintf(stderr,"Out of memory");
	return 1;
    }

    for(j=0;j<frames;j++)
	get_mem2D( (*array3D)+j, rows, columns ) ;

        return frames*rows*columns;
}

/********************************************************************
 *\Main Function:
 * 
 *\Author: Justin@CITYU
 *\Date: May 2010 
 *********************************************************************/

int main(int argc,char **argv)
{

    int i,j = 0;
    int wi,hj;	//iterators witdth_i, height j, frame k
    int framewidth, frameheight;// padding frame, width+blocksize  height+blocksize
    int mb_num_in_row, mb_num_in_col;
    //--- Read configure data into the main function ---
    int search_range;

    refnum = atoi(argv[6]);
    //level = atoi(argv[6]);
    //base_mvres = 1;/* atoi(argv[5])*/;
    search_range = atoi(argv[5]);
    numFrames = atoi(argv[4]);
    height = atoi(argv[3]);
    width = atoi(argv[2]);
    sprintf(seqName,"%s", argv[1]);
   // strcpy(seqName, argv[1]);
    
    if(argc ==1)
    {
	fprintf(stderr, "Usage :\n");
	fprintf(stderr, "%s <source> <width> <height> <num_frames> <search-range> <num_ref>\n",argv[0]);
	fprintf(stderr, "[Example: ./search.exe stefan_cif.yuv 352 288 100 32 1]\n");
	return 0;
    }
    //Open input sequence
    if( (seqFile=fopen(seqName,"rb")) == NULL )
    {
	fprintf(stderr, "Error opening file!");
	return 1;
    }

    //--- Get the frame parameters ---
    framewidth = width/blocksize*blocksize + blocksize;   // frame size with an extra padding(in size of one blocksize). Such that the block search can search 
    // to the position of most right and most bottom side.
    frameheight = height/blocksize*blocksize + blocksize;


    mb_num_in_row = width/blocksize;
    mb_num_in_col = height/blocksize;



    GetMemory(framewidth, frameheight, search_range/*, refnum*/); 

    bufferFrame(width, height/*, refnum*/); //feed one frame to curFrame

 //[INFO] 
 //fprintf(stderr, "[JL-INFO8]---------------------------------------A New Frame----------------------------------------------------\n");
    
    // output the first original frame
    for(hj = 0; hj < height; hj++)
	for(wi = 0; wi < width; wi++)
	    printf("%c",curFrameY[hj][wi]);
    for(hj = 0; hj < height/2; hj++)
	for(wi = 0; wi < width/2; wi++)
	    printf("%c", 128);
    for(hj = 0; hj < height/2; hj++)
	for(wi = 0; wi < width/2; wi++)
	    printf("%c", 128);
	
    initSpiral(search_range); //Initialize spiral full search pattern
    
    for(i=1;i<numFrames;i++)
    {
	bufferFrame(width, height/*, refnum*/);

	ME_SEARCH(search_range, mb_num_in_row, mb_num_in_col);

	
	for(hj = 0; hj < height; hj++)
	    for(wi = 0; wi < width; wi++)
		printf("%c",mpFrameY[hj][wi]);
	for(hj = 0; hj < height/2; hj++)
	    for(wi = 0; wi < width/2; wi++)
		printf("%c", 128);
	for(hj = 0; hj < height/2; hj++)
	    for(wi = 0; wi < width/2; wi++)
	    printf("%c", 128);

	/*// [INFO] 
	for(j = 0; j<mb_num_in_row*mb_num_in_col; j++)
	fprintf(stderr,"\n ----------------------------------\n[JL-INFO09]MB_NO.[%d] all_mv record in a frame:[%d, %d] \n----------------------------------------\n", j, all_mv[j][0], all_mv[j][1]);
	*/
	}

    FreeMemory(refnum);

    return 0;
}
