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

/**

 * This file defines escape sequences that can be used in bash and dos to manipulate the text. 
 * the \033 or \e or \E decodes in bash for an ascii escape code
 * 
 */

#ifndef COLORMANIPULATORS_HPP_
#define COLORMANIPULATORS_HPP_

#if _WIN32_WINNT_WIN7 || __MINGW32__
#define nUSE_ANSI_MANIPULATORS
#else
#define USE_ANSI_MANIPULATORS
#endif 

#ifdef USE_ANSI_MANIPULATORS

#define ESC "\033"

/* 
 Foreground colors
 30    Black
 31    Red
 32    Green
 33    Yellow
 34    Blue
 35    Magenta
 36    Cyan
 37    White
 
 Background colors
 40    Black
 41    Red
 42    Green
 43    Yellow
 44    Blue
 45    Magenta
 46    Cyan
 47    White
 */

//colour
#define ANSI_ResetTerminal  ESC<<"[0m"
#define ANSI_Black ESC<<"[30;47m"
#define ANSI_Red ESC<<"[31;47m"
#define ANSI_Green ESC<<"[32;47m"
#define ANSI_Yellow ESC<<"[33;47m"
#define ANSI_Blue ESC<<"[34;47m"
#define ANSI_BlackOnWhite ESC<<"[30;47m"
#define ANSI_RedOnWhite ESC<<"[31;47m"
#define ANSI_GreenOnWhite ESC<<"[32;47m"
#define ANSI_YellowOnWhite ESC<<"[33;47m"
#define ANSI_BlueOnWhite ESC<<"[34;47m"
#define ANSI_WhiteOnBlack ESC<<"[37;40m"

/*
 *     Text attributes
 0    All attributes off
 1    Bold on
 4    Underscore (on monochrome display adapter only)
 5    Blink on
 7    Reverse video on
 8    Concealed on
 
 */
//bold text or normal
#define ANSI_Normal ESC<<"[0m"
#define ANSI_Bold ESC<<"[1m"
#define ANSI_Underscore ESC<<"[5m"
#define ANSI_Blink ESC<<"[5m"

/* move cursor position*/
#define ANSI_save ESC<<"[s"
#define ANSI_restore ESC<<"[u"

//right colum
#define ANSI_right ESC<<"[800C"
#define ANSI_rigth_tab1  ESC<<"[1D"
#define ANSI_rigth_tab3  ESC<<"[3D"
#define ANSI_rigth_tab7  ESC<<"[7D"

//print OK / FAIL message on screen
#define ANSI_OKmessage std::cout<< ANSI_Green << ESC<<"[800C" << ANSI_rigth_tab1 <<"OK"<< ANSI_Black;
#define ANSI_FAILmessage std::cout<< ANSI_Red << ESC<<"[800C" << ANSI_rigth_tab3 <<"FAIL"<< ANSI_Black;

#endif
#ifndef USE_ANSI_MANIPULATORS

#define ESC " "
#define ANSI_ResetTerminal  ESC
#define ANSI_Black ESC
#define ANSI_Red ESC
#define ANSI_Green ESC
#define ANSI_Yellow ESC
#define ANSI_Blue ESC
#define ANSI_BlackOnWhite ESC
#define ANSI_RedOnWhite ESC
#define ANSI_GreenOnWhite ESC
#define ANSI_YellowOnWhite ESC
#define ANSI_BlueOnWhite ESC
#define ANSI_WhiteOnBlack ESC
#define ANSI_Normal ESC
#define ANSI_Bold ESC
#define ANSI_Underscore ESC
#define ANSI_Blink ESC

/* move cursor position*/
#define ANSI_save ESC
#define ANSI_restore ESC

#define ANSI_right ESC
#define ANSI_rigth_tab1  ESC
#define ANSI_rigth_tab3  ESC
#define ANSI_rigth_tab7  ESC

//print OK / FAIL message on screen
#define ANSI_OKmessage std::cout<<"OK";
#define ANSI_FAILmessage std::cout<<"FAIL";

#endif

#endif /*COLORMANIPULATORS_HPP_*/
