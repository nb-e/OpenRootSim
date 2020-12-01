################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/GenericModules/DefaultParameters.cpp \
../src/modules/GenericModules/FormulaInterpreter.cpp \
../src/modules/GenericModules/Generic.cpp \
../src/modules/GenericModules/Totals.cpp \
../src/modules/GenericModules/TotalsForAllPlants.cpp 

OBJS += \
./src/modules/GenericModules/DefaultParameters.o \
./src/modules/GenericModules/FormulaInterpreter.o \
./src/modules/GenericModules/Generic.o \
./src/modules/GenericModules/Totals.o \
./src/modules/GenericModules/TotalsForAllPlants.o 

CPP_DEPS += \
./src/modules/GenericModules/DefaultParameters.d \
./src/modules/GenericModules/FormulaInterpreter.d \
./src/modules/GenericModules/Generic.d \
./src/modules/GenericModules/Totals.d \
./src/modules/GenericModules/TotalsForAllPlants.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/GenericModules/%.o: ../src/modules/GenericModules/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


