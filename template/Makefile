
all-amd : main.cpp
	g++ -std=c++11 -w -o main main.cpp -I /opt/AMDAPPSDK-2.9-1/include -L /opt/AMDAPPSDK-2.9-1/lib/x86_64 -lOpenCL

all-macos : main.cpp
		g++ -std=c++11 -w -o main main.cpp -framework OpenCL

clean :
	rm *.o
	rm main
