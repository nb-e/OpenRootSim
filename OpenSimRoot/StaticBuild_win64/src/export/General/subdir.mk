################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/export/General/GarbageCollection.cpp \
../src/export/General/PrimeModel.cpp \
../src/export/General/ProbeAllObjects.cpp 

OBJS += \
./src/export/General/GarbageCollection.o \
./src/export/General/PrimeModel.o \
./src/export/General/ProbeAllObjects.o 

CPP_DEPS += \
./src/export/General/GarbageCollection.d \
./src/export/General/PrimeModel.d \
./src/export/General/ProbeAllObjects.d 


# Each subdirectory must supply rules for building sources it contributes
src/export/General/%.o: ../src/export/General/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


