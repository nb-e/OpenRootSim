################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/tools/StringExtensions.cpp \
../src/tools/Time.cpp 

OBJS += \
./src/tools/StringExtensions.o \
./src/tools/Time.o 

CPP_DEPS += \
./src/tools/StringExtensions.d \
./src/tools/Time.d 


# Each subdirectory must supply rules for building sources it contributes
src/tools/%.o: ../src/tools/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


