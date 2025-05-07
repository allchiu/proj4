# Compiler and Flags
CC = cl
NVCC = nvcc
CFLAGS = /Wall /Zi /MTd  # /Wall for warnings, /Zi for debug info, /MTd for multi-threaded debug mode
LDFLAGS = /out:proj4.exe  # Output executable name
CUDA_FLAGS = -arch=sm_50  # CUDA architecture version

# Source files
SRC_C = proj4.c main.c
SRC_CUDA = proj4_cuda.cu
OBJ_C = $(SRC_C:.c=.obj)
OBJ_CUDA = $(SRC_CUDA:.cu=.obj)

# Targets
all: proj4.exe

# Compile C source files to object files (.obj)
%.obj: %.c
	$(CC) $(CFLAGS) /c $<

# Compile CUDA source files to object files (.obj)
%.obj: %.cu
	$(NVCC) $(CUDA_FLAGS) -g -c $<

# Link the object files to create the executable
proj4.exe: $(OBJ_C) $(OBJ_CUDA)
	$(CC) $(OBJ_C) $(OBJ_CUDA) $(LDFLAGS)

# Run the executable
run: proj4.exe
	./proj4.exe in1.txt out1.txt 10 1

# Clean up object and executable files
clean:
	del /Q *.obj *.exe
