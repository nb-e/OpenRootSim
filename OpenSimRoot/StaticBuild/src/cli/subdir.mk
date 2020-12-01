################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/cli/Info.cpp \
../src/cli/Messages.cpp \
../src/cli/OpenSimRoot.cpp \
../src/cli/Signals.cpp 

OBJS += \
./src/cli/Info.o \
./src/cli/Messages.o \
./src/cli/OpenSimRoot.o \
./src/cli/Signals.o 

CPP_DEPS += \
./src/cli/Info.d \
./src/cli/Messages.d \
./src/cli/OpenSimRoot.d \
./src/cli/Signals.d 


# Each subdirectory must supply rules for building sources it contributes
src/cli/%.o: ../src/cli/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


