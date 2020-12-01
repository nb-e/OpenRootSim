################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/CarbonModule/Carbon2DryWeight.cpp \
../src/modules/CarbonModule/CarbonAllocation.cpp \
../src/modules/CarbonModule/CarbonBalance.cpp \
../src/modules/CarbonModule/CarbonCosts.cpp \
../src/modules/CarbonModule/CarbonSinks.cpp \
../src/modules/CarbonModule/CarbonSources.cpp \
../src/modules/CarbonModule/Respiration.cpp \
../src/modules/CarbonModule/SeedReserves.cpp 

OBJS += \
./src/modules/CarbonModule/Carbon2DryWeight.o \
./src/modules/CarbonModule/CarbonAllocation.o \
./src/modules/CarbonModule/CarbonBalance.o \
./src/modules/CarbonModule/CarbonCosts.o \
./src/modules/CarbonModule/CarbonSinks.o \
./src/modules/CarbonModule/CarbonSources.o \
./src/modules/CarbonModule/Respiration.o \
./src/modules/CarbonModule/SeedReserves.o 

CPP_DEPS += \
./src/modules/CarbonModule/Carbon2DryWeight.d \
./src/modules/CarbonModule/CarbonAllocation.d \
./src/modules/CarbonModule/CarbonBalance.d \
./src/modules/CarbonModule/CarbonCosts.d \
./src/modules/CarbonModule/CarbonSinks.d \
./src/modules/CarbonModule/CarbonSources.d \
./src/modules/CarbonModule/Respiration.d \
./src/modules/CarbonModule/SeedReserves.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/CarbonModule/%.o: ../src/modules/CarbonModule/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


