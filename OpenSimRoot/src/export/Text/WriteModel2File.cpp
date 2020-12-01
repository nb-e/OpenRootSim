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
#include "../../export/Text/WriteModel2File.hpp"

#include <ostream>
#include <fstream>
#include "../../cli/Messages.hpp"
#include <string>
#include "../../engine/Origin.hpp"
#include "../../engine/Database.hpp"
#include "../../export/ExportLibrary.hpp"
#include "../../import/xmlReader/Indenting.hpp"
#include "../../tools/StringExtensions.hpp"
#if _WIN32 || _WIN64
#include <process.h>
#define execl _execl
#else
#include <unistd.h>
#endif

ModelDump::ModelDump():ExportBase("modelDump"),probe(ORIGIN),
currentSearchDepth(0), maxSearchDepth(1000),
dir(getCurrentInputFolderName()){}
void ModelDump::initialize(){
	if(dir.substr(0,2)=="..") {
		//stylesheets are up in the hierarchy - better copy them
		//r=
		execl("cp","-r",(dir+"XML").c_str(),".", nullptr);
		//if(!r)
			dir.clear();
	}
}

void ModelDump::finalize(){
}

void ModelDump::run(const Time &t){
	//Open file
	std::string filename(std::to_string(t));
	filename.resize(6,0);
	std::string::size_type pos(filename.find(".",0));
	if (pos<3){
		size_t n(3-pos);
		for(std::size_t i=0; i<n; i++) filename.insert(0,"0");
		filename.resize(6,0);
	}
	filename+=".xml";
	filename.insert(0,"modelDump");
	os.open( filename.c_str() );
	if ( !os.is_open() ) msg::error("ModelDump: Failed to open "+filename);

	//dump the model into the file
	dumpModel(os);
	//Write table footer
	os<<std::endl<<std::flush;

	//Close file
	os.close();
}

//Function to dump the whole model to a stream (generally a file or std::cout)
//This functions simply dumps the ORIGIN to a file since everything in the model is a child of the ORIGIN
void ModelDump::dumpModel(std::ostream &os)const{
	//set indenting to zero
	indent()=0;
	//Write xml header
	os<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
	os<<std::endl<<"<?xml-stylesheet type=\"text/xsl\" href=\""<< dir <<"XML/outlineview.xsl\" ?>";
	//write a comment
	os<<LBREAK<<"<!--Modeldump. note that the dump may not be complete.-->";
	//write opening tag
	//todo use model name, include data and time, allow for comment
	os<<LBREAK<<"<SimulationModel name=\"notimplemented\" data=\"notimplemented\" id=\"notimplemented\">";
	//increase indenting
	indentIncrease();
	//write top object, which in its tern will write its children, etc till bottom of tree as been reached
	dumpModel(os, ORIGIN);
	//decrease indenting
	indentDecrease();
	//write closing tag
	os<<LBREAK<<"</SimulationModel>"<<std::endl;//todo include name, data, etc
}
void ModelDump::dumpModel(std::ostream &os, SimulaBase* top)const{
	//write top object, which in its tern will write its children, etc till bottom of tree as been reached
	//top->dumpXML(os);
	Tag tag;
	top->getXMLtag(tag);
	os<<LBREAK<<tag;
	if(!tag.closed){
		if(top->getNumberOfChildren()){
			indentIncrease();
			SimulaBase::List l;
			top->getAllChildren(l);
			for(auto &it:l){
				dumpModel(os,it);
			}
			indentDecrease();
		}
		os<<LBREAK<<"</"<<tag.name<<">";
	}
}

