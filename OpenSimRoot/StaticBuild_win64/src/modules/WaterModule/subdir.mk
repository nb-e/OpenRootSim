################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/WaterModule/DoussanModel.cpp \
../src/modules/WaterModule/WaterUptakeByRoots.cpp 

OBJS += \
./src/modules/WaterModule/DoussanModel.o \
./src/modules/WaterModule/WaterUptakeByRoots.o 

CPP_DEPS += \
./src/modules/WaterModule/DoussanModel.d \
./src/modules/WaterModule/WaterUptakeByRoots.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/WaterModule/%.o: ../src/modules/WaterModule/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


