################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/engine/DataDefinitions/Coordinates.cpp \
../src/engine/DataDefinitions/CustomMap.cpp \
../src/engine/DataDefinitions/ReferenceLists.cpp \
../src/engine/DataDefinitions/StateRate.cpp \
../src/engine/DataDefinitions/TagClass.cpp \
../src/engine/DataDefinitions/Units.cpp 

OBJS += \
./src/engine/DataDefinitions/Coordinates.o \
./src/engine/DataDefinitions/CustomMap.o \
./src/engine/DataDefinitions/ReferenceLists.o \
./src/engine/DataDefinitions/StateRate.o \
./src/engine/DataDefinitions/TagClass.o \
./src/engine/DataDefinitions/Units.o 

CPP_DEPS += \
./src/engine/DataDefinitions/Coordinates.d \
./src/engine/DataDefinitions/CustomMap.d \
./src/engine/DataDefinitions/ReferenceLists.d \
./src/engine/DataDefinitions/StateRate.d \
./src/engine/DataDefinitions/TagClass.d \
./src/engine/DataDefinitions/Units.d 


# Each subdirectory must supply rules for building sources it contributes
src/engine/DataDefinitions/%.o: ../src/engine/DataDefinitions/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


