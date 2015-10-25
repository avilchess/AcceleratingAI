
#include <chrono>/* for clock_gettime */
#include <iostream>
#include <cstdlib>/* for exit() definition */
#ifndef __APPLE__
#include <CL/cl.h>
#else
#include <opencl/cl.h>
#endif

using namespace std;

//OpenCL globals & labels
#define MAX_SOURCE_SIZE (10000000)

#define CHECK_STATUS(status, message)           \
  if (status != CL_SUCCESS) {                   \
		printf("%d ", status);											\    
		printf(message);                            \
    printf("\n");                               \
  }


cl_platform_id platform_id;
cl_device_id device_id;
cl_context context;
cl_command_queue command_queue;
cl_mem initial_buf, ending_buf, results_buf, matrix_buf; 
cl_kernel kernel;


// Location
#define PATHS 16
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
#define ROWS 64
#define COLS 64
int * matrix;

int get(int * matrix, int i, int j)
{
	#ifdef SPARSE

	#else
		return matrix[i*COLS + j];
	#endif
}

//Result
struct Result
{
	int nelem;	
	unsigned int data[ROWS*COLS/16];	
};


struct Result * results;

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
	if (ret != CL_SUCCESS)
	{
		char build_c[4096];
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 4096, 			build_c, NULL );
		printf( "LOG: %s\n", build_c );

	}
  CHECK_STATUS(ret, "Error: Build Program\n");

  // Create the OpenCL kernel
  kernel = clCreateKernel(program, "IDA", &ret);
  CHECK_STATUS(ret, "Error: Create kernel. (clCreateKernel)\n");

}

void DataInit()
{
	matrix = (int *) malloc (ROWS*COLS*sizeof(int));
	initial = (struct Location *) malloc (PATHS*sizeof(struct Location));
	ending = (struct Location *) malloc (PATHS*sizeof(struct Location));
	results = (struct Result *) malloc (PATHS*sizeof(struct Result));
	for (int i = 0; i < PATHS; i++) results[i].nelem = 0;

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
																	PATHS*sizeof(struct Result),
																	results,
																	&ret);
	CHECK_STATUS(ret, "Error: Create buffer. (results)\n");
}

void DataReceive()
{
	
	int ret = clEnqueueReadBuffer(command_queue,results_buf,CL_TRUE, 0,
			PATHS*sizeof(struct Result),(void *) results ,0, NULL, NULL);
	CHECK_STATUS(ret, "Error: Read buffer. (results)\n");
}

void Execute()
{
//SET ARGS
//		__kernel void IDA( 	__global struct Location * initial,
//										__global struct Location * ending,
//										__global int * matrix,
//										 int paths,
//										__global int * output)    

		size_t sizes[2] = {256, 256};
		int ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&initial_buf);
    CHECK_STATUS(ret, "Error:  Set Arg. (0)\n");
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&ending_buf);
    CHECK_STATUS(ret, "Error:  Set Arg. (1)\n");
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&matrix_buf);
    CHECK_STATUS(ret, "Error:  Set Arg. (2)\n");
    ret = clSetKernelArg(kernel, 3, sizeof(int), (void *)&sizes[1]);
    CHECK_STATUS(ret, "Error:  Set Arg. (3)\n");
		ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&results_buf);
    CHECK_STATUS(ret, "Error:  Set Arg. (4)\n");

    ret = clFinish(command_queue);
    
    
    
    //CALL KERNEL
    
    ret = clEnqueueNDRangeKernel(
                                    command_queue,
                                    kernel,
                                    1,
                                    NULL,
                                    &sizes[0],
                                    &sizes[1],
                                    0,
                                    0,
                                    NULL);
    CHECK_STATUS(ret, "Error:  NDRange. \n");

    ret = clFinish(command_queue);

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


		for (int i = 0; i < 16; i++)
		{
				cout << results[i].nelem << " ";
		}
		cout << endl;
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
