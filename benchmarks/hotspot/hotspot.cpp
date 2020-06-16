/*! 
 Usage: ./hotspot <grid_rows> <grid_cols> <sim_time> <no. of threads> <no. of workers> <tao_width> <xitao_sched> <temp_file> <power_file> \n
 (or) \n
 make run \n

 \param <grid_rows>  - number of rows in the grid (positive integer) \n
 \param <grid_cols>  - number of columns in the grid (positive integer) \n
 \param <sim_time>   - number of iterations \n
 \param <no. of threads>   - number of threads \n
 \param <no. of workers>   - number of xitao workers \n
 \param <tao_width>   - tao width \n
 \param <xitao_sched>   - xitao scheduling \n
 \param <temp_file>  - name of the file containing the initial temperature values of each cell \n
 \param <power_file> - name of the file containing the dissipated power values of each cell \n
 \param <output_file> - name of the output file \n     
*/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

#define XITAO
#define OPEN
//#define NUM_THREAD 4

#ifdef XITAO

#include "xitao.h"

using namespace xitao;

/*! A dummy TAO (used as a barrier between iterations) */
class DummyTAO : public AssemblyTask {
  // No data  
public:
  DummyTAO():AssemblyTask(1) { }
  void execute(int nthread) {
    //int tid = nthread - leader;     
  }
  void cleanup () { }
};

/*! A TAO for swaping pointers */
class SwapPtrTAO : public AssemblyTask {
  float** m_r;
  float** m_t;
public:
  SwapPtrTAO(float**r, float**t):
    AssemblyTask(1), m_r(r), m_t(t) { }
  void execute(int nthread) {
    int tid = nthread - leader;
    if(tid==0)
    {      
      float* tmp;
      tmp = *m_t;
      *m_t = *m_r;
      *m_r = tmp;
    }
  }
  void cleanup () { }
};
#endif

// Returns the current system time in microseconds 
long long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;

}

using namespace std;

#define BLOCK_SIZE 16
#define BLOCK_SIZE_C BLOCK_SIZE
#define BLOCK_SIZE_R BLOCK_SIZE

#define STR_SIZE	256

/* maximum power density possible (say 300W for a 10mm x 10mm chip)	*/
#define MAX_PD	(3.0e6)
/* required precision in degrees	*/
#define PRECISION	0.001
#define SPEC_HEAT_SI 1.75e6
#define K_SI 100
/* capacitance fitting factor	*/
#define FACTOR_CHIP	0.5

/* chip parameters	*/
const float t_chip = 0.0005;
const float chip_height = 0.016;
const float chip_width = 0.016;

/* ambient temperature, assuming no package at all	*/
const float amb_temp = 80.0;

int num_omp_threads;

/* Single iteration of the transient solver in the grid model.
 * advances the solution of the discretized difference equations 
 * by one time step
 */
void single_iteration(float *result, float *temp, float *power, int row, int col,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, 
					  float step)
{
    float delta;
    int r, c;
    int chunk;
    int num_chunk = row*col / (BLOCK_SIZE_R * BLOCK_SIZE_C);
    int chunks_in_row = col/BLOCK_SIZE_C;
    int chunks_in_col = row/BLOCK_SIZE_R;

#ifdef OPEN
    #ifndef __MIC__
    	omp_set_num_threads(num_omp_threads);
    #endif
#pragma omp parallel for shared(power, temp, result) private(chunk, r, c, delta) firstprivate(row, col, num_chunk, chunks_in_row) schedule(static)
#endif
    for ( chunk = 0; chunk < num_chunk; ++chunk )
    {
        int r_start = BLOCK_SIZE_R*(chunk/chunks_in_col);
        int c_start = BLOCK_SIZE_C*(chunk%chunks_in_row); 
        int r_end = r_start + BLOCK_SIZE_R > row ? row : r_start + BLOCK_SIZE_R;
        int c_end = c_start + BLOCK_SIZE_C > col ? col : c_start + BLOCK_SIZE_C;

        if ( r_start == 0 || c_start == 0 || r_end == row || c_end == col )
        {
            for ( r = r_start; r < r_start + BLOCK_SIZE_R; ++r ) {
                for ( c = c_start; c < c_start + BLOCK_SIZE_C; ++c ) {
                    /* Corner 1 */
                    if ( (r == 0) && (c == 0) ) {
                        delta = (Cap_1) * (power[0] +
                            (temp[1] - temp[0]) * Rx_1 +
                            (temp[col] - temp[0]) * Ry_1 +
                            (amb_temp - temp[0]) * Rz_1);
                    }	/* Corner 2 */
                    else if ((r == 0) && (c == col-1)) {
                        delta = (Cap_1) * (power[c] +
                            (temp[c-1] - temp[c]) * Rx_1 +
                            (temp[c+col] - temp[c]) * Ry_1 +
                        (   amb_temp - temp[c]) * Rz_1);
                    }	/* Corner 3 */
                    else if ((r == row-1) && (c == col-1)) {
                        delta = (Cap_1) * (power[r*col+c] + 
                            (temp[r*col+c-1] - temp[r*col+c]) * Rx_1 + 
                            (temp[(r-1)*col+c] - temp[r*col+c]) * Ry_1 + 
                        (   amb_temp - temp[r*col+c]) * Rz_1);					
                    }	/* Corner 4	*/
                    else if ((r == row-1) && (c == 0)) {
                        delta = (Cap_1) * (power[r*col] + 
                            (temp[r*col+1] - temp[r*col]) * Rx_1 + 
                            (temp[(r-1)*col] - temp[r*col]) * Ry_1 + 
                            (amb_temp - temp[r*col]) * Rz_1);
                    }	/* Edge 1 */
                    else if (r == 0) {
                        delta = (Cap_1) * (power[c] + 
                            (temp[c+1] + temp[c-1] - 2.0*temp[c]) * Rx_1 + 
                            (temp[col+c] - temp[c]) * Ry_1 + 
                            (amb_temp - temp[c]) * Rz_1);
                    }	/* Edge 2 */
                    else if (c == col-1) {
                        delta = (Cap_1) * (power[r*col+c] + 
                            (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.0*temp[r*col+c]) * Ry_1 + 
                            (temp[r*col+c-1] - temp[r*col+c]) * Rx_1 + 
                            (amb_temp - temp[r*col+c]) * Rz_1);
                    }	/* Edge 3 */
                    else if (r == row-1) {
                        delta = (Cap_1) * (power[r*col+c] + 
                            (temp[r*col+c+1] + temp[r*col+c-1] - 2.0*temp[r*col+c]) * Rx_1 + 
                            (temp[(r-1)*col+c] - temp[r*col+c]) * Ry_1 + 
                            (amb_temp - temp[r*col+c]) * Rz_1);
                    }	/* Edge 4 */
                    else if (c == 0) {
                        delta = (Cap_1) * (power[r*col] + 
                            (temp[(r+1)*col] + temp[(r-1)*col] - 2.0*temp[r*col]) * Ry_1 + 
                            (temp[r*col+1] - temp[r*col]) * Rx_1 + 
                            (amb_temp - temp[r*col]) * Rz_1);
                    }
                    result[r*col+c] =temp[r*col+c]+ delta;
                }
            }
            continue;
        }

        for ( r = r_start; r < r_start + BLOCK_SIZE_R; ++r ) {
#pragma omp simd        
            for ( c = c_start; c < c_start + BLOCK_SIZE_C; ++c ) {
            /* Update Temperatures */
                result[r*col+c] =temp[r*col+c]+ 
                     ( Cap_1 * (power[r*col+c] + 
                    (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
                    (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
                    (amb_temp - temp[r*col+c]) * Rz_1));
            }
        }
    }
}

/* Transient solver driver routine: simply converts the heat 
 * transfer differential equations to difference equations 
 * and solves the difference equations by iterating
 */
void compute_tran_temp(float *result, int num_iterations, float *temp, float *power, int row, int col) 
{
	#ifdef VERBOSE
	int i = 0;
	#endif

	float grid_height = chip_height / row;
	float grid_width = chip_width / col;

	float Cap = FACTOR_CHIP * SPEC_HEAT_SI * t_chip * grid_width * grid_height;
	float Rx = grid_width / (2.0 * K_SI * t_chip * grid_height);
	float Ry = grid_height / (2.0 * K_SI * t_chip * grid_width);
	float Rz = t_chip / (K_SI * grid_height * grid_width);

	float max_slope = MAX_PD / (FACTOR_CHIP * t_chip * SPEC_HEAT_SI);
    float step = PRECISION / max_slope / 1000.0;

    float Rx_1=1.f/Rx;
    float Ry_1=1.f/Ry;
    float Rz_1=1.f/Rz;
    float Cap_1 = step/Cap;
	#ifdef VERBOSE
	fprintf(stdout, "total iterations: %d s\tstep size: %g s\n", num_iterations, step);
	fprintf(stdout, "Rx: %g\tRy: %g\tRz: %g\tCap: %g\n", Rx, Ry, Rz, Cap);
	#endif
        {
            float* r = result;
            float* t = temp;
            for (int i = 0; i < num_iterations ; i++)
            {
                #ifdef VERBOSE
                fprintf(stdout, "iteration %d\n", i++);
                #endif
                single_iteration(r, t, power, row, col, Cap_1, Rx_1, Ry_1, Rz_1, step);
                float* tmp = t;
                t = r;
                r = tmp;
            }	
        }
	#ifdef VERBOSE
	fprintf(stdout, "iteration %d\n", i++);
	#endif
}

/* Transient solver driver routine: simply converts the heat 
 * transfer differential equations to difference equations 
 * and solves the difference equations by iterating
 */
#ifdef XITAO
void compute_tran_temp_xitao(float *result, int num_iterations, float *temp, float *power, int row, int col, DummyTAO** headTAO, int tao_width, int sched) 
{
	#ifdef VERBOSE
	int i = 0;
	#endif

	float grid_height = chip_height / row;
	float grid_width = chip_width / col;

	float Cap = FACTOR_CHIP * SPEC_HEAT_SI * t_chip * grid_width * grid_height;
	float Rx = grid_width / (2.0 * K_SI * t_chip * grid_height);
	float Ry = grid_height / (2.0 * K_SI * t_chip * grid_width);
	float Rz = t_chip / (K_SI * grid_height * grid_width);

	float max_slope = MAX_PD / (FACTOR_CHIP * t_chip * SPEC_HEAT_SI);
	float step = PRECISION / max_slope / 1000.0;
	
	float Rx_1=1.f/Rx;
	float Ry_1=1.f/Ry;
	float Rz_1=1.f/Rz;
	float Cap_1 = step/Cap;
	#ifdef VERBOSE
	fprintf(stdout, "total iterations: %d s\tstep size: %g s\n", num_iterations, step);
	fprintf(stdout, "Rx: %g\tRy: %g\tRz: %g\tCap: %g\n", Rx, Ry, Rz, Cap);
	#endif

        SwapPtrTAO* swapPtrTAO;
        DummyTAO* itrTAO = new DummyTAO();
        *headTAO = itrTAO;

	int num_chunk = row*col / (BLOCK_SIZE_R * BLOCK_SIZE_C);
	int chunks_in_row = col/BLOCK_SIZE_C;
	int chunks_in_col = row/BLOCK_SIZE_R;

        for (int i = 0; i < num_iterations ; i++)
        { 	  

            #ifdef VERBOSE
            fprintf(stdout, "iteration %d\n", i++);
            #endif
	    
	    int chunk=0;

            auto vec = __xitao_vec_non_immediate_multiparallel_region(1, chunk, num_chunk, xitao_vec_dynamic, 1,
               {
  		    float delta;
                    int r;
                    int c;
                    int r_start = BLOCK_SIZE_R*(chunk/chunks_in_col);
                    int c_start = BLOCK_SIZE_C*(chunk%chunks_in_row); 
                    int r_end = r_start + BLOCK_SIZE_R > row ? row : r_start + BLOCK_SIZE_R;
                    int c_end = c_start + BLOCK_SIZE_C > col ? col : c_start + BLOCK_SIZE_C;
		    
                    if ( r_start == 0 || c_start == 0 || r_end == row || c_end == col )
                    {
                        for ( r = r_start; r < r_start + BLOCK_SIZE_R; ++r ) {
                            for ( c = c_start; c < c_start + BLOCK_SIZE_C; ++c ) {
                                // Corner 1 //
                                if ( (r == 0) && (c == 0) ) {
                                    delta = (Cap_1) * (power[0] +
                                        (temp[1] - temp[0]) * Rx_1 +
                                        (temp[col] - temp[0]) * Ry_1 +
                                        (amb_temp - temp[0]) * Rz_1);
                                }	// Corner 2 //
                                else if ((r == 0) && (c == col-1)) {
                                    delta = (Cap_1) * (power[c] +
                                        (temp[c-1] - temp[c]) * Rx_1 +
                                        (temp[c+col] - temp[c]) * Ry_1 +
                                    (   amb_temp - temp[c]) * Rz_1);
                                }	// Corner 3 //
                                else if ((r == row-1) && (c == col-1)) {
                                    delta = (Cap_1) * (power[r*col+c] + 
                                        (temp[r*col+c-1] - temp[r*col+c]) * Rx_1 + 
                                        (temp[(r-1)*col+c] - temp[r*col+c]) * Ry_1 + 
                                    (   amb_temp - temp[r*col+c]) * Rz_1);					
                                }	// Corner 4	//
                                else if ((r == row-1) && (c == 0)) {
                                    delta = (Cap_1) * (power[r*col] + 
                                        (temp[r*col+1] - temp[r*col]) * Rx_1 + 
                                        (temp[(r-1)*col] - temp[r*col]) * Ry_1 + 
                                        (amb_temp - temp[r*col]) * Rz_1);
                                }	// Edge 1 //
                                else if (r == 0) {
                                    delta = (Cap_1) * (power[c] + 
                                        (temp[c+1] + temp[c-1] - 2.0*temp[c]) * Rx_1 + 
                                        (temp[col+c] - temp[c]) * Ry_1 + 
                                        (amb_temp - temp[c]) * Rz_1);
                                }	// Edge 2 //
                                else if (c == col-1) {
                                    delta = (Cap_1) * (power[r*col+c] + 
                                        (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.0*temp[r*col+c]) * Ry_1 + 
                                        (temp[r*col+c-1] - temp[r*col+c]) * Rx_1 + 
                                        (amb_temp - temp[r*col+c]) * Rz_1);
                                }	// Edge 3 
                                else if (r == row-1) {
                                    delta = (Cap_1) * (power[r*col+c] + 
                                        (temp[r*col+c+1] + temp[r*col+c-1] - 2.0*temp[r*col+c]) * Rx_1 + 
                                        (temp[(r-1)*col+c] - temp[r*col+c]) * Ry_1 + 
                                        (amb_temp - temp[r*col+c]) * Rz_1);
                                }	// Edge 4 
                                else if (c == 0) {
                                    delta = (Cap_1) * (power[r*col] + 
                                        (temp[(r+1)*col] + temp[(r-1)*col] - 2.0*temp[r*col]) * Ry_1 + 
                                        (temp[r*col+1] - temp[r*col]) * Rx_1 + 
                                        (amb_temp - temp[r*col]) * Rz_1);
                                }
                                result[r*col+c] =temp[r*col+c]+ delta;
                            }
                        }
                        continue;
                    }

                    for ( r = r_start; r < r_start + BLOCK_SIZE_R; ++r ) {
                        for ( c = c_start; c < c_start + BLOCK_SIZE_C; ++c ) {
			  // Update Temperatures
                            result[r*col+c] =temp[r*col+c]+ 
                                ( Cap_1 * (power[r*col+c] + 
                                (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
                                (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
                                (amb_temp - temp[r*col+c]) * Rz_1));
                        }
                    }
               }
            );

            for(auto dataparallelNode : vec) {
            itrTAO->make_edge(dataparallelNode);
            }

            itrTAO = new DummyTAO();
            for(auto dataparallelNode : vec) {
            dataparallelNode->make_edge(itrTAO);
            }

            swapPtrTAO = new SwapPtrTAO(&result, &temp);
            itrTAO->make_edge(swapPtrTAO);

            itrTAO = new DummyTAO();
            swapPtrTAO->make_edge(itrTAO);
        }	

	#ifdef VERBOSE
	fprintf(stdout, "iteration %d\n", i++);
	#endif

	gotao_push(*headTAO);
	gotao_start();
	gotao_fini();
}
#endif

void fatal(char *s)
{
	fprintf(stderr, "error: %s\n", s);
	exit(1);
}

void writeoutput(float *vect, int grid_rows, int grid_cols, char *file) {

    int i,j, index=0;
    FILE *fp;
    char str[STR_SIZE];

    if( (fp = fopen(file, "w" )) == 0 )
        printf( "The file was not opened\n" );


    for (i=0; i < grid_rows; i++) 
        for (j=0; j < grid_cols; j++)
        {

            sprintf(str, "%d\t%g\n", index, vect[i*grid_cols+j]);
            fputs(str,fp);
            index++;
        }

    fclose(fp);	
}

void read_input(float *vect, int grid_rows, int grid_cols, char *file)
{
  	int i, index;
	FILE *fp;
	char str[STR_SIZE];
	float val;

	fp = fopen (file, "r");
	if (!fp)
		fatal ("file could not be opened for reading");

	for (i=0; i < grid_rows * grid_cols; i++) {
		fgets(str, STR_SIZE, fp);
		if (feof(fp))
			fatal("not enough lines in file");
		if ((sscanf(str, "%f", &val) != 1) )
			fatal("invalid file format");
		vect[i] = val;
	}

	fclose(fp);	
}

void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <grid_rows> <grid_cols> <sim_time> <no. of threads> <no. of workers> <tao_width> <xitao_sched> <temp_file> <power_file>\n", argv[0]);
	fprintf(stderr, "\t<grid_rows>  - number of rows in the grid (positive integer)\n");
	fprintf(stderr, "\t<grid_cols>  - number of columns in the grid (positive integer)\n");
	fprintf(stderr, "\t<sim_time>   - number of iterations\n");
	fprintf(stderr, "\t<no. of threads>   - number of threads\n");
    fprintf(stderr, "\t<no. of workers>   - number of xitao workers\n");
    fprintf(stderr, "\t<tao_width>   - tao width\n");
    fprintf(stderr, "\t<xitao_sched>   - xitao scheduling\n");
	fprintf(stderr, "\t<temp_file>  - name of the file containing the initial temperature values of each cell\n");
	fprintf(stderr, "\t<power_file> - name of the file containing the dissipated power values of each cell\n");
    fprintf(stderr, "\t<output_file> - name of the output file\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int grid_rows, grid_cols, sim_time, i;
	float *temp, *power, *result;
	char *tfile, *pfile, *ofile;
	char *xofile = "xoutput.out";
	int xitao_workers, tao_width, xitao_sched;
	/* check validity of inputs	*/
	if (argc != 11)
		usage(argc, argv);
	if ((grid_rows = atoi(argv[1])) <= 0 ||
		(grid_cols = atoi(argv[2])) <= 0 ||
		(sim_time = atoi(argv[3])) <= 0 || 
		(num_omp_threads = atoi(argv[4])) <= 0 ||
        (xitao_workers = atoi(argv[5])) <= 0 ||
        (tao_width = atoi(argv[5])) < 0 ||
        (xitao_sched = atoi(argv[5])) < 0
		)
		usage(argc, argv);

	/* allocate memory for the temperature and power arrays	*/
	temp = (float *) calloc (grid_rows * grid_cols, sizeof(float));
	power = (float *) calloc (grid_rows * grid_cols, sizeof(float));
	result = (float *) calloc (grid_rows * grid_cols, sizeof(float));
	if(!temp || !power)
		fatal("unable to allocate memory");

	/* read initial temperatures and input power	*/
	tfile = argv[8];
	pfile = argv[9];
	ofile = argv[10];
    
    
// OpenMP Start
	read_input(temp, grid_rows, grid_cols, tfile);
	read_input(power, grid_rows, grid_cols, pfile);
#ifdef OPEN
	printf("Start computing (OpenMP) the transient temperature\n");
#else
    printf("Start computing the transient temperature\n");
#endif
    long long start_time = get_time();
       
    compute_tran_temp(result,sim_time, temp, power, grid_rows, grid_cols);
    
    long long end_time = get_time();
    
#ifdef OPEN
    printf("Ending simulation (OpenMP) \n");
    printf("Total time (OpenMP) : %.3f seconds\n", ((float) (end_time - start_time)) / (1000*1000));
#else
    printf("Ending simulation\n");
    printf("Total time : %.3f seconds\n", ((float) (end_time - start_time)) / (1000*1000));
#endif
// OpenMP End    
    
    writeoutput((1&sim_time) ? result : temp, grid_rows, grid_cols, ofile);
    
// XiTAO Start
#ifdef XITAO
    read_input(temp, grid_rows, grid_cols, tfile);
    read_input(power, grid_rows, grid_cols, pfile);

    printf("\nStart computing (XiTAO) the transient temperature\n");
	
    long long start_time_x = get_time();
    
    gotao_init_hw(xitao_workers, -1 , -1);
    DummyTAO* headTAO;

    float* r = result;
    float* t = temp;

    compute_tran_temp_xitao(result,sim_time, temp, power, grid_rows, grid_cols, &headTAO, tao_width, xitao_sched);
        
    //gotao_push(headTAO);
    //gotao_start();
    //gotao_fini();
    
    long long end_time_x = get_time();

    result = r;
    temp = t;
    
    printf("Ending simulation (XiTAO)\n");
    printf("Total time (XiTAO): %.3f seconds\n", ((float) (end_time_x - start_time_x)) / (1000*1000));
    printf("Total successful steals: %d\n", tao_total_steals); 
#endif
// XiTAO End

    writeoutput((1&sim_time) ? result : temp, grid_rows, grid_cols, xofile);

	/* output results	*/
#ifdef VERBOSE
	fprintf(stdout, "Final Temperatures:\n");
#endif

#ifdef OUTPUT
	for(i=0; i < grid_rows * grid_cols; i++)
	fprintf(stdout, "%d\t%g\n", i, temp[i]);
#endif
	/* cleanup	*/
	free(temp);
	free(power);
	free(result);

	return 0;
}
/* vim: set ts=4 sw=4  sts=4 et si ai: */
