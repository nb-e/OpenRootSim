################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/IntegrationMethods/IntegrationLibrary.cpp 

OBJS += \
./src/modules/IntegrationMethods/IntegrationLibrary.o 

CPP_DEPS += \
./src/modules/IntegrationMethods/IntegrationLibrary.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/IntegrationMethods/%.o: ../src/modules/IntegrationMethods/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


