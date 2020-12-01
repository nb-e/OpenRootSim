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

/**@mainpage SimRoot
 SimRoot is a functional-structural plant model. Please see
 the manual and publications for more info.
 */

#include "Signals.hpp"
#include "Info.hpp"
#include "Messages.hpp"
#include "ANSImanipulators.hpp"
#include "../import/xmlReader/ReadXMLfile.hpp"
#include "../export/ExportLibrary.hpp"
#include "../engine/Origin.hpp"
#include "../engine/SimulaConstant.hpp"
#include "../engine/SimulaStochastic.hpp"
#include "../engine/ObjectGenerator.hpp"
#include <stdlib.h>
#include <iostream>
#include <csignal>
#include "gitversion.hpp"
#if _WIN32 || _WIN64
  bool dialog_mode(false); //true when started without arguments
  #define _AFXDLL
  #define getpid _getpid
  #include <process.h>
#if _MSC_VER
  #include <afxdlgs.h>
#else
  //#include <windows.h>
  #include  <shobjidl.h>
#endif
#else
  #include <unistd.h>//for getting pid
#endif



#include <ctime> //for getting system time

int main(int argc, char **argv) {

	//set the exitcode
	int exitCode = 0;
	atexit(exitRoutine);

	//define signals
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	//commandline arguments
	cla arg(argc, argv);


	//help
#if _WIN32 || _WIN64
	//without arguments windows will show dialog box, and not help
	if (arg.find("-h", "--help", "/?", "-lh")) {
#else
	if ( argc == 1 || arg.find("-h", "--help", "/?", "-lh")) {
#endif
		printhelp();
		if(arg.find("-l", "--list","-lh")){
			std::cout<<std::endl;
			printfunctions();
		}
		return (exitCode);
	}

	if(arg.find("-l", "--list")){
		printfunctions();
		return (exitCode);
	}

	//verbose
	if (arg.find("-v"))
		msg::setVerboseLevel(10);
	if (arg.find("-q"))
		msg::setVerboseLevel(-10);
	if (arg.find("-ww"))
		msg::warningsToScreen();


#if _WIN32 || _WIN64
	std::string fn;
	if (argc == 1) {
		printhelp();
		dialog_mode = true;

#if _MSC_VER
		std::cout	<< "\nSee above message how to to start OpenSimRoot in a console without file dialog. \nDialog mode: \nSELECT YOUR XML InputFile ";

		CFileDialog dlgFile(TRUE);

		// show Filename and get ok button
		if (dlgFile.DoModal() == IDOK) {
			CString pathName = dlgFile.GetPathName();
			fn = pathName;

			//TODO set working directory

			//AfxMessageBox(pathName);
		}
#else
//todo impement here an opensource dialog (Qt or so) instead of exiting.
		std::cout	<< "\nSee above message how to to start OpenSimRoot in a console (CMD) without file dialog.\n\n";

		// memory buffer to contain the file name
		char szFile[100];

		// open a file name
		OPENFILENAME ofn ;
		ZeroMemory( &ofn , sizeof( ofn));
		ofn.lStructSize = sizeof ( ofn );
		ofn.hwndOwner = NULL  ;
		ofn.lpstrFile = szFile ;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof( szFile );
		ofn.lpstrFilter = "All\0*.*\0xml\0*.XML\0";
		ofn.nFilterIndex =2;
		ofn.lpstrFileTitle = NULL ;
		ofn.nMaxFileTitle = 0 ;
		ofn.lpstrInitialDir=NULL ;
		ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
		GetOpenFileName( &ofn );

		// Now simpley display the file name
		//MessageBox ( NULL , ofn.lpstrFile , "File Name" , MB_OK);
		fn=szFile;
	    //return (exitCode);
#endif
	}
	else {
		//Try to load model from file
		fn = arg.findValue("-f", "--file");
	}
#else
	//Try to load model from file
	const std::string fn(arg.findValue("-f", "--file"));
#endif
	try {
		//construct origin
		const Coordinate ori(0, 0, 0);
		ORIGIN = new SimulaConstant<Coordinate>("origin", nullptr, ori,0);
		//load file
		std::cout << ANSI_Black << "Running OpenSimRoot \n\tbuild on "<<__DATE__<<", \n\tlicensed GPLv3" << " \n\tgit "<< gitversion << std::flush;
		msg::warning("OpenSimRoot was build on "+(std::string)__DATE__+" from git version "+ gitversion );
		std::cout << std::endl << "Trying to load model from file: " << std::flush;
		bool verifyOnly(arg.find("-V", "--verify"));
		if(verifyOnly) msg::setErrorIsWarningOn();
		readFile(fn);
		if(verifyOnly) {
			if(msg::errorOccured()){
				msg::setErrorIsWarningOff("Inputfile contains errors. See warnings for all the issues.");
			}else{
				ANSI_OKmessage;
				std::cout<<std::endl<<"Inputfile contains no errors, end of verification.";
				return (exitCode);
			}
		}
		ANSI_OKmessage
		//set seed for random number generator
		int seed=static_cast<int>(std::time(0)*getpid());
		SimulaBase *p=ORIGIN->existingPath("/simulationControls/randomNumberGeneratorSeed");
		if(p) p->get(seed);
		setRandomSeed(seed);
		//signal object generators that they can start updating
		ObjectGenerator::startUpdating();
		//run the export modules which drive the model.
		std::cout << ANSI_Black << "\nRunning modules: " << std::flush;
#if _WIN32 || _WIN64
		std::cout<<std::endl;
#endif
		runExportModules(exitCode,fn);
		//delete ORIGIN
		delete ORIGIN;//causes segfault sometimes. Don't know why yet. But not deleting causes the destructors not to be called.
	} catch (std::exception& error) {
		ANSI_FAILmessage
		std::cout << ANSI_Red << error.what() << ANSI_Black;
		exitCode = 1;
	}

	//exit normally
	return (exitCode);
}

