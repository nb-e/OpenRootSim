################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/Soil/Mapping.cpp \
../src/modules/Soil/Material.cpp \
../src/modules/Soil/Mesh.cpp \
../src/modules/Soil/MichealisMenten.cpp \
../src/modules/Soil/Mineralization.cpp \
../src/modules/Soil/Output.cpp \
../src/modules/Soil/SoilTemperature.cpp \
../src/modules/Soil/Solute.cpp \
../src/modules/Soil/Swms3d.cpp \
../src/modules/Soil/Watflow.cpp 

OBJS += \
./src/modules/Soil/Mapping.o \
./src/modules/Soil/Material.o \
./src/modules/Soil/Mesh.o \
./src/modules/Soil/MichealisMenten.o \
./src/modules/Soil/Mineralization.o \
./src/modules/Soil/Output.o \
./src/modules/Soil/SoilTemperature.o \
./src/modules/Soil/Solute.o \
./src/modules/Soil/Swms3d.o \
./src/modules/Soil/Watflow.o 

CPP_DEPS += \
./src/modules/Soil/Mapping.d \
./src/modules/Soil/Material.d \
./src/modules/Soil/Mesh.d \
./src/modules/Soil/MichealisMenten.d \
./src/modules/Soil/Mineralization.d \
./src/modules/Soil/Output.d \
./src/modules/Soil/SoilTemperature.d \
./src/modules/Soil/Solute.d \
./src/modules/Soil/Swms3d.d \
./src/modules/Soil/Watflow.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/Soil/%.o: ../src/modules/Soil/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


