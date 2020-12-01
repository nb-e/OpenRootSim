################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/ShootModule/LeafArea.cpp \
../src/modules/ShootModule/LeafSenescence.cpp \
../src/modules/ShootModule/Photosynthesis.cpp \
../src/modules/ShootModule/ShootDryWeigth.cpp \
../src/modules/ShootModule/TillerFormation.cpp 

OBJS += \
./src/modules/ShootModule/LeafArea.o \
./src/modules/ShootModule/LeafSenescence.o \
./src/modules/ShootModule/Photosynthesis.o \
./src/modules/ShootModule/ShootDryWeigth.o \
./src/modules/ShootModule/TillerFormation.o 

CPP_DEPS += \
./src/modules/ShootModule/LeafArea.d \
./src/modules/ShootModule/LeafSenescence.d \
./src/modules/ShootModule/Photosynthesis.d \
./src/modules/ShootModule/ShootDryWeigth.d \
./src/modules/ShootModule/TillerFormation.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/ShootModule/%.o: ../src/modules/ShootModule/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


