################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/engine/BaseClasses.cpp \
../src/engine/Database.cpp \
../src/engine/ObjectGenerator.cpp \
../src/engine/Origin.cpp \
../src/engine/SimulaBase.cpp \
../src/engine/SimulaDynamic.cpp \
../src/engine/SimulaExternal.cpp \
../src/engine/SimulaGrid.cpp \
../src/engine/SimulaLink.cpp \
../src/engine/SimulaPoint.cpp \
../src/engine/SimulaStochastic.cpp \
../src/engine/SimulaTimeDriven.cpp \
../src/engine/SimulaVariable.cpp \
../src/engine/timeStepParameters.cpp 

OBJS += \
./src/engine/BaseClasses.o \
./src/engine/Database.o \
./src/engine/ObjectGenerator.o \
./src/engine/Origin.o \
./src/engine/SimulaBase.o \
./src/engine/SimulaDynamic.o \
./src/engine/SimulaExternal.o \
./src/engine/SimulaGrid.o \
./src/engine/SimulaLink.o \
./src/engine/SimulaPoint.o \
./src/engine/SimulaStochastic.o \
./src/engine/SimulaTimeDriven.o \
./src/engine/SimulaVariable.o \
./src/engine/timeStepParameters.o 

CPP_DEPS += \
./src/engine/BaseClasses.d \
./src/engine/Database.d \
./src/engine/ObjectGenerator.d \
./src/engine/Origin.d \
./src/engine/SimulaBase.d \
./src/engine/SimulaDynamic.d \
./src/engine/SimulaExternal.d \
./src/engine/SimulaGrid.d \
./src/engine/SimulaLink.d \
./src/engine/SimulaPoint.d \
./src/engine/SimulaStochastic.d \
./src/engine/SimulaTimeDriven.d \
./src/engine/SimulaVariable.d \
./src/engine/timeStepParameters.d 


# Each subdirectory must supply rules for building sources it contributes
src/engine/%.o: ../src/engine/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


