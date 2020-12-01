################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/tools/subdir.mk
-include src/modules/WaterModule/subdir.mk
-include src/modules/Soil/subdir.mk
-include src/modules/ShootModule/subdir.mk
-include src/modules/RootLengthDensity/subdir.mk
-include src/modules/RootGrowth/subdir.mk
-include src/modules/PlantingModule/subdir.mk
-include src/modules/PlantNutrients/subdir.mk
-include src/modules/IntegrationMethods/subdir.mk
-include src/modules/GrowthRegulation/subdir.mk
-include src/modules/GenericModules/subdir.mk
-include src/modules/CarbonModule/subdir.mk
-include src/modules/BarberCushman/subdir.mk
-include src/modules/Atmosphere/subdir.mk
-include src/math/subdir.mk
-include src/import/xmlReader/subdir.mk
-include src/export/VTK/subdir.mk
-include src/export/Text/subdir.mk
-include src/export/RSML/subdir.mk
-include src/export/General/subdir.mk
-include src/export/3dimage/subdir.mk
-include src/export/subdir.mk
-include src/engine/DataDefinitions/subdir.mk
-include src/engine/subdir.mk
-include src/cli/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: OpenSimRoot

# Tool invocations
OpenSimRoot: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "OpenSimRoot" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) OpenSimRoot
	-@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
