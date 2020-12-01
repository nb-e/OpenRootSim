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
/* See headerfile for explanation and change log*/
#include "Signals.hpp"
#include "Info.hpp"
#include <stdlib.h>
#include <csignal>
#include <iostream>

void signalHandler(int param) {
	//write warnings
	writeWarnings();
#if _WIN32 || _WIN64
	//at the moment nothing, as we simple exit.
#else
	//read answer
	char a('o');
	//check answer
	int count(0);
#endif
	//switch box for handle
	switch (param) {
	case SIGINT:
#if _WIN32 || _WIN64
		//abort the program
		std::_Exit(1);
#else
		//write mesage
		std::cerr << "\nSigInt: Do you want to exit? Y/n:";
		std::cin.get(a);
		//canonicalize to capital letter
		a = toupper(a);
		while (a != 'N' && a != 'Y') {
			count++;
			std::cerr << std::endl << "Wrong input, answer with Y or N (" << count << " try):";
			//read answer (clear last enter by an extra get)
			std::cin.get(a);
			//canonicalize to capital letter
			a = toupper(a);
			if (count > 5)
				a = 'Y';
		}
		if (a == 'Y') {
			//reset the terminal colors.
			exitRoutine(false);
			//abort the program
			std::abort(); //std::quick_exit(1);
		}
#endif
		break;
	case SIGTERM:
		std::cerr << "\nSigTerm:";
		//reset the terminal colors.
		exitRoutine(false);
		//abort the program
		std::abort(); //std::quick_exit(1);
		break;
	default:
		std::cerr << "\nSig" << param << ":";
		//reset the terminal colors.
		exitRoutine(false);
		//abort the program
		std::abort(); //std::quick_exit(1);
		break;
	}
}

