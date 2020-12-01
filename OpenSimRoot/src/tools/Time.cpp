/*
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
#include "Time.hpp"
#include "../cli/Messages.hpp"
#include "../engine/SimulaBase.hpp"
#include "../engine/Origin.hpp"
#include <math.h>

void TimeConversion::setStartTimeOfTheYear(){
	if(day<0){
		SimulaBase* d 				= ORIGIN->getPath("/environment/startDay");
		SimulaBase* m				= ORIGIN->getPath("/environment/startMonth");
		SimulaBase* y				= ORIGIN->existingPath("/environment/startYear");

		d	   ->get(day);
		if(day>31 || day<1 )msg::error("TimeConversion:: day outside 1 ... 31");
		m  ->get(month);
		if(month>12 || month<1 )msg::error("TimeConversion:: startMonth outside 1 ... 12");
		if(y) {
			y->get(year);
		}else{
			msg::warning("TimeConversion:: startYear not given. Using 2015",2);
		}
		doy = daysToMonth[isLeapYear( year ) ? 1 : 0][month] + day;
	}
}


int TimeConversion::day(-1), TimeConversion::month(0), TimeConversion::doy(0),TimeConversion::year(0);
SimulaBase* TimeConversion::degreesDays_(nullptr);

int TimeConversion::daysToMonth[2][12] =
{
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
};

bool TimeConversion::isLeapYear( int year )
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

/*****************************************************************************/
unsigned int TimeConversion::dateToNumber(const Time &t) {
/*****************************************************************************/
	TimeConversion::setStartTimeOfTheYear();
	return (doy+trunc(t));
} // end dateToNumber

double TimeConversion::timeToDegreesDays(const Time &t){
	if(!degreesDays_){
		degreesDays_= ORIGIN->existingPath("/environment/atmosphere/degreesDays");
	}
	double r;
	if(degreesDays_) {
		degreesDays_->get(t,r);
	}else{
		r=t*25;
		msg::warning("TimeConversion::timeToDegreesDays: Conversion between time and degreesDays requested, but degreesDays not found in /environment/atmosphere/degreesDays, assuming constant temperature of 25 degrees C");
	}
	return r;
}
