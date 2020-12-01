################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/math/BiCGSTAB.cpp \
../src/math/CSRmatrix.cpp \
../src/math/InterpolationLibrary.cpp \
../src/math/MathLibrary.cpp \
../src/math/SparseMatrix.cpp \
../src/math/SparseSymmetricMatrix.cpp \
../src/math/VectorMath.cpp \
../src/math/pcgSolve.cpp 

OBJS += \
./src/math/BiCGSTAB.o \
./src/math/CSRmatrix.o \
./src/math/InterpolationLibrary.o \
./src/math/MathLibrary.o \
./src/math/SparseMatrix.o \
./src/math/SparseSymmetricMatrix.o \
./src/math/VectorMath.o \
./src/math/pcgSolve.o 

CPP_DEPS += \
./src/math/BiCGSTAB.d \
./src/math/CSRmatrix.d \
./src/math/InterpolationLibrary.d \
./src/math/MathLibrary.d \
./src/math/SparseMatrix.d \
./src/math/SparseSymmetricMatrix.d \
./src/math/VectorMath.d \
./src/math/pcgSolve.d 


# Each subdirectory must supply rules for building sources it contributes
src/math/%.o: ../src/math/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


