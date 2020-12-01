/*
Copyright © 2016, The Pennsylvania State University
All rights reserved.

Copyright © 2016 Forschungszentrum Jülich GmbH
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted under the GNU General Public License v3 and provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

Disclaimer
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You should have received the GNU GENERAL PUBLIC LICENSE v3 with this file in license.txt but can also be found at http://www.gnu.org/licenses/gpl-3.0.en.html

NOTE: The GPL.v3 license requires that all derivative work is distributed under the same license. That means that if you use this source code in any other program, you can only distribute that program with the full source code included and licensed under a GPL license.

 */
#ifndef CARBONALLOCATION_HPP_
#define CARBONALLOCATION_HPP_

#include "../../engine/BaseClasses.hpp"

/// finish relative carbon allocation once: from input - potential - to meat a certain K
/// net carbon available (without respiration) should be included

/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the root system. This is should be between 0-1.
 *
 * notes:
 * 1) This uses fixed carbon allocation as specified in
 * the input files. See resource section as part of the parameter section
 * 2) See other "RelativeCarbonAllocation2RootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */
class RelativeCarbonAllocation2RootsFromInputFile:public DerivativeBase{
public:
	RelativeCarbonAllocation2RootsFromInputFile(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *c2rootSimulator;
	Time plantingTime;
};
/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the shoot. This is should be between 0-1.
 *
 * notes:
 * 1) This uses fixed carbon allocation as specified in
 * the input files. See resource section as part of the parameter section
 * 2) See other "RelativeCarbonAllocation2ShootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */
class RelativeCarbonAllocation2ShootFromInputFile:public DerivativeBase{
public:
	RelativeCarbonAllocation2ShootFromInputFile(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *c2rootSimulator;
	Time plantingTime;
};
/**
 * Calculates how of the much carbon that is available for shoot growth (construction)
 * should go to the leaves. This is should be between 0-1.
 *
 * notes:
 * 1) This uses fixed carbon allocation as specified in
 * the input files. See resource section as part of the parameter section
 * 2) See other "RelativeCarbonAllocation2LeafsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of the shoot.
 */

class RelativeCarbonAllocation2LeafsFromInputFile:public DerivativeBase{
public:
	RelativeCarbonAllocation2LeafsFromInputFile(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *c2leafsSimulator, *leafsink, *stemsink;
	Time plantingTime;
};
/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the root system. This is should be between 0-1.
 *
 * notes:
 * 1) This allocates as much carbon as is needed for potential growth of
 * the root system.
 * 2) See other "RelativeCarbonAllocation2RootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */
class RelativeCarbonAllocation2RootsPotentialGrowth:public DerivativeBase{
public:
	RelativeCarbonAllocation2RootsPotentialGrowth(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *Ctotal, *Cpotential;
	Time plantingTime;
};
/**
 * Calculates how much of the carbon that is available for growth (construction)
 * should go to the shoot. This is should be between 0-1.
 *
 * notes:
 * 1) As much carbon as necessary for potential growth.
 * 2) If not enough carbon is available for growth, it will allocate everything
 * that is available. However, you may want to specify another allocation maximum
 * than 100%, for example 85%. You can specify this in the resource section in the
 * input files with "maxCarbonAllocation2Shoot".
 * 3) See other "RelativeCarbonAllocation2ShootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */
class RelativeCarbonAllocation2ShootPotentialGrowth:public DerivativeBase{
public:
	RelativeCarbonAllocation2ShootPotentialGrowth(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *Ctotal, *Cpotential,*threshold, *carbon4potentialRootGrowth;
	Time plantingTime;
};
/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the shoot. This is should be between 0-1.
 *
 * notes:
 * 1) Hybrid: first 10 days potential growth is followed, between 10-20 days
 * a weighted average between potential growth and allocation based on input files
 * is used, than after 20 days the input files are used.
 * 2) See other "RelativeCarbonAllocation2ShootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */
class RelativeCarbonAllocation2ShootSwitch:public DerivativeBase{
public:
	RelativeCarbonAllocation2ShootSwitch(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *Ctotal, *Cpotential,*threshold,*parameterSection;
	Time plantingTime;
};

/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the root system. This is should be between 0-1.
 *
 * notes:
 * 1) Carbon is allocated assuming that both shoot and root have equal
 * sink strength. The sink strength here is the amount of carbon that is needed
 * for potential growth.
 * 2) See other "RelativeCarbonAllocation2RootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */

class RelativeCarbonAllocation2RootsScaledGrowth:public DerivativeBase{
public:
	RelativeCarbonAllocation2RootsScaledGrowth(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *CpotentialPlant, *CpotentialRoot;
};
/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the shoot. This is should be between 0-1.
 *
 * notes:
 * 1) Carbon is allocated assuming that both shoot and root have equal
 * sink strength. The sink strength here is the amount of carbon that is needed
 * for potential growth.
 * 3) See other "RelativeCarbonAllocation2ShootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */
class RelativeCarbonAllocation2ShootScaledGrowth:public DerivativeBase{
public:
	RelativeCarbonAllocation2ShootScaledGrowth(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *CpotentialPlant, *CpotentialShoot;
};
/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the root system. This is should be between 0-1.
 *
 * notes:
 * 1) This uses what ever is not used by the shoot. See relative carbon
 * allocation to the shoot.
 * 2) See other "RelativeCarbonAllocation2RootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */

class RemainingProportion:public DerivativeBase{
public:
	RemainingProportion(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *s;
};
/**
 * Calculates how of the much carbon that is available for growth (construction)
 * should go to the root system. This is should be between 0-1.
 *
 * notes:
 * 1) This uses what ever is not used by the shoot. See relative carbon
 * allocation to the shoot.
 * 2) See other "RelativeCarbonAllocation2RootsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of a plant.
 */

class RelativeCarbonAllocation2RootsOneMinusShoot:public DerivativeBase{
public:
	RelativeCarbonAllocation2RootsOneMinusShoot(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *shootAlloc;
};
/**
 * Calculates how of the much carbon that is available for shoot growth (construction)
 * should go to the stems. This is should be between 0-1.
 *
 * notes:
 * 1) Whatever does not go to the leaves goes to the stems.
 * 2) See other "RelativeCarbonAllocation2StemsXXXX" classes for alternatives.
 *
 * Database location:
 * This should be a child object of the shoot.
 */
class RelativeCarbonAllocation2StemsOneMinusLeafs:public DerivativeBase{
public:
	RelativeCarbonAllocation2StemsOneMinusLeafs(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *leafAlloc;
};

/**
 * Calculates how of the much carbon that is available for shoot growth (g/day)
 *
 * notes:
 * 1) use relative carbon allocation classes to control carbon allocation, this
 * simply converts relative to absolute allocation.
 *
 * Database location:
 * This should be a child object of the plant.
 */
class CarbonAllocation2Shoot:public DerivativeBase{
public:
	CarbonAllocation2Shoot(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *Ctotal, *c2shootSimulator;
};
/**
 * Calculates how of the much carbon that is available for leaf growth (g/day)
 *
 * notes:
 * 1) use relative carbon allocation classes to control carbon allocation, this
 * simply converts relative to absolute allocation.
 *
 * Database location:
 * This should be a child object of the shoot.
 */
class CarbonAllocation2Leafs:public DerivativeBase{
public:
	CarbonAllocation2Leafs(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *CtotalShoot, *c2leafsSimulator;
};
/**
 * Calculates how of the much carbon that is available for stem growth (g/day)
 *
 * notes:
 * 1) use relative carbon allocation classes to control carbon allocation, this
 * simply converts relative to absolute allocation.
 *
 * Database location:
 * This should be a child object of the shoot.
 */

class CarbonAllocation2Stems:public DerivativeBase{
public:
	CarbonAllocation2Stems(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *CtotalShoot, *c2stemsSimulator;
};
/**
 * Calculates how of the much carbon that is available for root growth (g/day)
 *
 * notes:
 * 1) use relative carbon allocation classes to control carbon allocation, this
 * simply converts relative to absolute allocation.
 *
 * Database location:
 * This should be a child object of the plant.
 */
class CarbonAllocation2Roots:public DerivativeBase{
public:
	CarbonAllocation2Roots(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *Ctotal, *c2rootsSimulator;
};
/**
 * Calculates how much the longitudinal and or secondary growth of the roots
 * needs to be scaled when carbon allocation is not equal to the carbon needed for
 * potential growth. Ideally this factor should be around 1.
 *
 * notes:
 * 1) This module can handle both secondary and primary growth and switches based
 * on it's name.
 * 2) It's basically the ratio between actual and potential growth rates.
 *
 * Database location:
 * This should be a child object of the plant.
 */
class RootGrowthScalingFactor:public DerivativeBase{
public:
	RootGrowthScalingFactor(SimulaDynamic* pSD);
	std::string getName()const;
	bool postIntegrationCorrection(SimulaVariable::Table & data);
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *potentialRootGrowthSimulator, *potentialRootGrowthSimulatorMajorAxis , *c2rootsSimulator, *c2SecondaryGrowth, *threshold, *reserves;
	bool rates;
	unsigned int mode;
	Time plantingTime,mt;
	double scale_;
};

#endif /*CARBONALLOCATION_HPP_*/
