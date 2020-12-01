################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/Atmosphere/AirParameter.cpp \
../src/modules/Atmosphere/DiffusionResistance.cpp \
../src/modules/Atmosphere/ETbase.cpp \
../src/modules/Atmosphere/EvapoEquations.cpp \
../src/modules/Atmosphere/Interception.cpp \
../src/modules/Atmosphere/Radiation.cpp \
../src/modules/Atmosphere/Transpiration.cpp \
../src/modules/Atmosphere/VaporPressure.cpp 

OBJS += \
./src/modules/Atmosphere/AirParameter.o \
./src/modules/Atmosphere/DiffusionResistance.o \
./src/modules/Atmosphere/ETbase.o \
./src/modules/Atmosphere/EvapoEquations.o \
./src/modules/Atmosphere/Interception.o \
./src/modules/Atmosphere/Radiation.o \
./src/modules/Atmosphere/Transpiration.o \
./src/modules/Atmosphere/VaporPressure.o 

CPP_DEPS += \
./src/modules/Atmosphere/AirParameter.d \
./src/modules/Atmosphere/DiffusionResistance.d \
./src/modules/Atmosphere/ETbase.d \
./src/modules/Atmosphere/EvapoEquations.d \
./src/modules/Atmosphere/Interception.d \
./src/modules/Atmosphere/Radiation.d \
./src/modules/Atmosphere/Transpiration.d \
./src/modules/Atmosphere/VaporPressure.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/Atmosphere/%.o: ../src/modules/Atmosphere/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


