#define ROWS 1024
#define COLS 1024

struct Location
{
    int row, col;
};

struct Result
{
	int nelem;	
	int data[ROWS*COLS/16];	
};


int getMatrix(__global int * matrix, int f, int c)
{
	return matrix[f*COLS + c];
}

int dist(struct Location start, struct Location end)
{
	return abs(end.row - start.row) + abs(end.col - start.col);
}

//direction: up = 00; down = 01; left = 10; right = 11;
int insertPath(struct Result res, unsigned int direction)
{
	int pos = res.nelem;
	int word = pos/4; //get the integer from data where it is stored;
	int subword = pos%16;
	unsigned int mask = direction << subword;
	res.data[word] = res.data[word] | mask;
	res.nelem++;
}

__kernel void IDA( 	__global struct Location * initial,
										__global struct Location * ending,
										__global int * matrix,
									  int paths,
										__global struct Result * output)
{
	int id  = get_global_id(0);
	
	struct Location init = initial[id];
	struct Location end = ending[id];

	output[id].nelem = id;//+dist(init, end);
	
}
