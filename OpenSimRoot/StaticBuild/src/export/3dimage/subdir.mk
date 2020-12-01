################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/export/3dimage/rawimage.cpp 

OBJS += \
./src/export/3dimage/rawimage.o 

CPP_DEPS += \
./src/export/3dimage/rawimage.d 


# Each subdirectory must supply rules for building sources it contributes
src/export/3dimage/%.o: ../src/export/3dimage/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


