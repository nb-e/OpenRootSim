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

#include <string>
#include <map>

/**
 * writes all the warnings the model generated to the screen
 */
void writeWarnings();

/**
 * code executed when exiting the model.
 */
void exitRoutine();
/**
 * code executed when exiting the model.
 * bool for indicating if warnings should be written to screen
 *
 */
void exitRoutine(const bool writewarnings);

/**
 * Simple interface to the cl_ arguments
 *
 */
class cla {
public:
	///constructs based on default parsing of command line arguments
	cla(const int argc, char **argv);

	///looks for the existence of an command line argument, with optionally three alternatives
	bool find(const std::string &lab, const std::string &altlab1 = "", const std::string &altlab2 = "", const std::string &altlab3 = "") const;
	///looks for the existence of a command line argument and returns the value following it
	std::string findValue(const std::string &lab, const std::string &altlab1 = "", const std::string &altlab2 = "", const std::string &altlab3 = "") const;

private:
	///map for storing commandline argument pairs
	std::map<std::string, std::string> cl_;
};

/**
 * print of help message
 */
void printhelp();
void printfunctions();
