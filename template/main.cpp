
#include <chrono>/* for clock_gettime */
#include <iostream>
#include <cstdlib>/* for exit() definition */
#include <CL/cl.h>

using namespace std;

//OpenCL globals & labels
#define MAX_SOURCE_SIZE (10000000)

#define CHECK_STATUS(status, message)           \
  if (status != CL_SUCCESS) {                   \
    printf(message);                            \
    printf("\n");                               \
  }


cl_platform_id platform_id;
cl_device_id device_id;
cl_context context;
cl_command_queue command_queue;
cl_mem initial_buf, ending_buf, results_buf, matrix_buf; 

// Location
#define PATHS 64
struct Location
{
    int row, col;
    Location()
    {
        row = col = 0;
    };

    Location(int r, int c)
    {
        row = r;
        col = c;
    };
};

struct Location * initial;
struct Location * ending;

// Matrix
//#define SPARSE
#define ROWS 1024
#define COLS 1024
int * matrix;

int get(int * matrix, int i, int j)
{
	#ifdef SPARSE

	#else	
		return matrix[i*COLS + j];
	#endif
}

//Result
int * results;

void OpenCLInit()
{
  FILE *fp;
  char *source_str;
  size_t source_size;

  fp = fopen("kernel.cl", "r");
  if (!fp) {
    fprintf(stderr, "Failed to load kernel.\n");
    exit(1);
  }
  source_str = (char*)malloc(MAX_SOURCE_SIZE);
  source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  // Get platform and device information
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
  ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1,
                       &device_id, &ret_num_devices);

 

  char *platformVendor;
  size_t platInfoSize;
  clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, 0, NULL,
                    &platInfoSize);

  platformVendor = (char*)malloc(platInfoSize);


  // Create an OpenCL context
  context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

  // Create a command queue
 	command_queue = 
		clCreateCommandQueue(context, device_id, NULL, &ret);

  // Create a program from the kernel source
  cl_program program = clCreateProgramWithSource(context, 1,
                                                 (const char **)&source_str,
                                                 (const size_t *)&source_size,
	                                               &ret);
  // Build the program
  ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

  CHECK_STATUS(ret, "Error: Build Program\n");

  // Create the OpenCL kernel
  cl_kernel kernel = clCreateKernel(program, "IDA", &ret);
  CHECK_STATUS(ret, "Error: Create kernel. (clCreateKernel)\n");

}

void DataInit()
{
	matrix = (int *) malloc (ROWS*COLS*sizeof(int));
	initial = (struct Location *) malloc (PATHS*sizeof(struct Location));
	ending = (struct Location *) malloc (PATHS*sizeof(struct Location));
	results = (int *) malloc (PATHS*ROWS*COLS*sizeof(int));
}

void DataSend()
{
	cl_int ret;
	matrix_buf = clCreateBuffer (		context,
																	CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
																	ROWS*COLS*sizeof(int),
																	matrix,
																	&ret);
	CHECK_STATUS(ret, "Error: Create buffer. (matrix)\n");

	initial_buf = clCreateBuffer (	context,
																	CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
																	PATHS*sizeof(struct Location),
																	initial,
																	&ret);
	CHECK_STATUS(ret, "Error: Create buffer. (initial)\n");

	ending_buf = clCreateBuffer (		context,
																	CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
																	PATHS*sizeof(struct Location),
																	ending,
																	&ret);
	CHECK_STATUS(ret, "Error: Create buffer. (ending)\n");

	
	results_buf = clCreateBuffer (	context,
																	CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
																	PATHS*ROWS*COLS*sizeof(int),
																	results,
																	&ret);
	CHECK_STATUS(ret, "Error: Create buffer. (results)\n");
}

void DataReceive()
{

}

void Execute()
{

}

int main(int argc, char const ** argv)
{
	  chrono::time_point<chrono::system_clock> start_clinit, end_clinit;
   
		start_clinit = chrono::system_clock::now();
		OpenCLInit();
		end_clinit = chrono::system_clock::now();

		chrono::time_point<chrono::system_clock> start_datainit, end_datainit;
    start_datainit = chrono::system_clock::now();
		DataInit();
		end_datainit = chrono::system_clock::now();

		chrono::time_point<chrono::system_clock> start_datasend, end_datasend;
    start_datasend = chrono::system_clock::now();
		DataSend();
		end_datasend = chrono::system_clock::now();

		chrono::time_point<chrono::system_clock> start_execute, end_execute;
		start_execute = chrono::system_clock::now();
		Execute();
		end_execute = chrono::system_clock::now();

		chrono::time_point<chrono::system_clock> start_datareceive, end_datareceive;
		start_datareceive = chrono::system_clock::now();
		DataReceive();
		end_datareceive = chrono::system_clock::now();


		//Summarize results
		cout << "[Timing]" << endl;
		chrono::duration<double> elapsed = end_clinit-start_clinit;
		cout << "InitCL=" << elapsed.count() << endl;
		elapsed = end_datainit-start_datainit;
		cout << "InitData=" << elapsed.count() << endl;
		elapsed = end_datasend-start_datasend;
		cout << "SendData=" << elapsed.count() << endl;
		elapsed = end_execute-start_execute;
		cout << "Execute=" << elapsed.count() << endl;
		elapsed = end_datareceive-start_datareceive;
		cout << "ReceiveData=" << elapsed.count() << endl;
		elapsed = end_datareceive-start_clinit;
		cout << "Total=" << elapsed.count() << endl;
	return 0;
}
