################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/import/xmlReader/DataReader.cpp \
../src/import/xmlReader/Indenting.cpp \
../src/import/xmlReader/ReadString.cpp \
../src/import/xmlReader/ReadXMLfile.cpp \
../src/import/xmlReader/SimulaReaders.cpp \
../src/import/xmlReader/StreamPositionInfo.cpp \
../src/import/xmlReader/Tag.cpp 

OBJS += \
./src/import/xmlReader/DataReader.o \
./src/import/xmlReader/Indenting.o \
./src/import/xmlReader/ReadString.o \
./src/import/xmlReader/ReadXMLfile.o \
./src/import/xmlReader/SimulaReaders.o \
./src/import/xmlReader/StreamPositionInfo.o \
./src/import/xmlReader/Tag.o 

CPP_DEPS += \
./src/import/xmlReader/DataReader.d \
./src/import/xmlReader/Indenting.d \
./src/import/xmlReader/ReadString.d \
./src/import/xmlReader/ReadXMLfile.d \
./src/import/xmlReader/SimulaReaders.d \
./src/import/xmlReader/StreamPositionInfo.d \
./src/import/xmlReader/Tag.d 


# Each subdirectory must supply rules for building sources it contributes
src/import/xmlReader/%.o: ../src/import/xmlReader/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	x86_64-w64-mingw32-g++ -std=c++14 -D"GITHASH=$(GIT_HASH)" -O3 -Wall -c -fmessage-length=0   -Wa,-mbig-obj -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


