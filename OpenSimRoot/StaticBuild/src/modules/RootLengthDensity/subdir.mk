################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/RootLengthDensity/RootLengthProfile.cpp \
../src/modules/RootLengthDensity/proximity.cpp 

OBJS += \
./src/modules/RootLengthDensity/RootLengthProfile.o \
./src/modules/RootLengthDensity/proximity.o 

CPP_DEPS += \
./src/modules/RootLengthDensity/RootLengthProfile.d \
./src/modules/RootLengthDensity/proximity.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/RootLengthDensity/%.o: ../src/modules/RootLengthDensity/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


