################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/modules/BarberCushman/NutrientUptake.cpp \
../src/modules/BarberCushman/NutrientUptake2.cpp \
../src/modules/BarberCushman/NutrientUptake3.cpp 

OBJS += \
./src/modules/BarberCushman/NutrientUptake.o \
./src/modules/BarberCushman/NutrientUptake2.o \
./src/modules/BarberCushman/NutrientUptake3.o 

CPP_DEPS += \
./src/modules/BarberCushman/NutrientUptake.d \
./src/modules/BarberCushman/NutrientUptake2.d \
./src/modules/BarberCushman/NutrientUptake3.d 


# Each subdirectory must supply rules for building sources it contributes
src/modules/BarberCushman/%.o: ../src/modules/BarberCushman/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


