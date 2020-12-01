################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/export/ExportBaseClass.cpp \
../src/export/ExportLibrary.cpp \
../src/export/circle.cpp 

OBJS += \
./src/export/ExportBaseClass.o \
./src/export/ExportLibrary.o \
./src/export/circle.o 

CPP_DEPS += \
./src/export/ExportBaseClass.d \
./src/export/ExportLibrary.d \
./src/export/circle.d 


# Each subdirectory must supply rules for building sources it contributes
src/export/%.o: ../src/export/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


