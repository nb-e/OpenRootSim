################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/PlantingModule/GenerateSeedling.cpp 

OBJS += \
./src/modules/PlantingModule/GenerateSeedling.o 

CPP_DEPS += \
./src/modules/PlantingModule/GenerateSeedling.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/PlantingModule/%.o: ../src/modules/PlantingModule/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


