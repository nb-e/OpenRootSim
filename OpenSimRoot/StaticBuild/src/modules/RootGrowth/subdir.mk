################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/RootGrowth/GenerateRootNodes.cpp \
../src/modules/RootGrowth/Geometry.cpp \
../src/modules/RootGrowth/GrowthDirection.cpp \
../src/modules/RootGrowth/LongitudinalGrowth.cpp \
../src/modules/RootGrowth/RootBranching.cpp \
../src/modules/RootGrowth/RootBranchingOfTillers.cpp \
../src/modules/RootGrowth/RootDryWeight.cpp \
../src/modules/RootGrowth/RootSegmentDryWeight.cpp \
../src/modules/RootGrowth/SecondaryGrowth.cpp 

OBJS += \
./src/modules/RootGrowth/GenerateRootNodes.o \
./src/modules/RootGrowth/Geometry.o \
./src/modules/RootGrowth/GrowthDirection.o \
./src/modules/RootGrowth/LongitudinalGrowth.o \
./src/modules/RootGrowth/RootBranching.o \
./src/modules/RootGrowth/RootBranchingOfTillers.o \
./src/modules/RootGrowth/RootDryWeight.o \
./src/modules/RootGrowth/RootSegmentDryWeight.o \
./src/modules/RootGrowth/SecondaryGrowth.o 

CPP_DEPS += \
./src/modules/RootGrowth/GenerateRootNodes.d \
./src/modules/RootGrowth/Geometry.d \
./src/modules/RootGrowth/GrowthDirection.d \
./src/modules/RootGrowth/LongitudinalGrowth.d \
./src/modules/RootGrowth/RootBranching.d \
./src/modules/RootGrowth/RootBranchingOfTillers.d \
./src/modules/RootGrowth/RootDryWeight.d \
./src/modules/RootGrowth/RootSegmentDryWeight.d \
./src/modules/RootGrowth/SecondaryGrowth.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/RootGrowth/%.o: ../src/modules/RootGrowth/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


