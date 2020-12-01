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
#include "Tag.hpp"
#include <sstream>
#include "../../cli/Messages.hpp"
#include "StreamPositionInfo.hpp"
#include "ReadString.hpp"
#include "../../tools/StringExtensions.hpp"

///@todo this header file is here for "getCurrentInputFileName()" we really need a better way of doing this. But first the import module needs to be converted to a class.
#include "../../engine/DataDefinitions/TagClass.hpp"
#include "ReadXMLfile.hpp"
//=================TAG CLASS======================================


//==================reading tags==================================
//read a XML tag from is and stores it in tag
//function similar to std::getLine
void getTag(std::istream &is, Tag &tag, const std::string & filename){
	//clear the tag
	tag.name.clear();
	tag.attributes.clear();

	//remove trailing whitespace
	is>>std::ws;
	MOVETONEXT(is) //just to skip over bomm characters etc.

	//check for comment, if found ignore
	skipComments(is);

	//remove trailing < so that getWord doesn't complain about empty string
	is>>std::ws;
	char c(is.get());
	/*if(c!='<' && c!='&'){
		//this is a hack to correct a problem with the include files which somehow set the cursor one character to far?
		is.unget();
		is.unget();
		c=is.get();
	}*/

	//check if this is of type <!Doctype
	char cn(is.peek()),ch;
	if(cn=='!'){
		c=cn;
	}
    std::string w,wp,s,v;
	switch (c) {
		case '<':
			//normal tag found
			//read the tag
			tag.name="";
			ch='-';
			//read words as long end of file is not reached
			while (!is.eof()){
				getWord(is,s,ch);
				//act based on the word delimiter
				switch (ch) {
					case '<':
						//found beginning of next tag before this one was closed. Throw exception
						//place delimiter back on stream, so that filePosition correctly shows where the error occured
						//is.unget();
						//throw exception
					    msg::error("getTag: found '<' but expected '>' first"+filePosition(is,getCurrentInputFileName()));
					    return;
						break;
					case '=':
						//the word that was read was an attribute name
						//check wether the attribute name was already found, in which case an exception is thrown
						if (tag.attributes.find(s)!=tag.attributes.end()) msg::error("getTag: Found two times '"+s+"' in tag '"+tag.name+"'."+filePosition(is));
						//read the next word which should be the attributes value between "" or ''
						getQuotedString(is,v);
						//store the the found attribute label with its value in the tags attributes list
						tag.attributes[s]=v;
						//check if end of tag has been found, if yes return
						is>>std::ws;
						///@todo this is not strict enough, it means that the / can be anywhere behind a word in the tag, not just at the end
						if(is.peek()== '/') {
							tag.closed=true;
							is.get();
						}
						if(is.peek()=='>') {
							is.get();
							return;
						}
						break;
					default:
						//a tagname has been found
						//check wether it is not an orphaned word
		    			if (tag.name!="") msg::error("getTag: Found two tagnames in one tag namely: '"+tag.name+"' and '"+s+"'. Tag may be missing a '=' or a '>'");
		    			//OK, store it in tag
		    			tag.name=s;
						///@todo this is not strict enough, it means that the / can be anywhere behind a word in the tag, not just at the end
						is>>std::ws;
		    			//check if end of tag has been found, if yes return
		    			if(ch=='>') {
		    				if (s=="/") tag.closed=true;
		    				return;
		    			}
						break;
				}
			}
			// end of file reached
			msg::error("getTag: error or reached end of file before '>' was found while reading: "+convertToString<Tag>(tag)+" in file "+getCurrentInputFileName() );
			break;
		case '!' :
			//<!Doctype found or the like
			//we doe not support this at all, but will try to load the path
			while(!is.eof()){
				wp=w;
				getWord(is,w,cn);
				if(w=="SYSTEM"){
					//next word is path, previous word is entity label
					getWord(is,w,cn);
					stripQuotationMarks(w);
					Tag::entities[wp]=w;
				}
				if(cn=='>' && w=="]") break;
				if(w.empty()) break;
			}
			//read next tag
			getTag(is,tag,filename);
			///@todo this is not very robust yet for coding errors.
			break;
		case '&' :
			//escape or entity found
			//no intention to support the xml standard here, but will try to support entities that represent an include file.
			getWord(is,w);
			//delete ';' from word
			if(*(w.rbegin())==';'){
				//delete last character
				w.erase(w.size()-1);
				//set tag to SimulaInclude (we do not support any other entities currently
				tag.name="SimulaIncludeFile";
				//look up entity
				//close tag
				tag.closed=true;
				//insert attribute
				Tag::Attributes::iterator it(Tag::entities.find(w));
				if(it!=Tag::entities.end()){
					tag.attributes["fileName"]=it->second;
				}else{
					for(Tag::Attributes::iterator it(Tag::entities.begin()); it!=Tag::entities.end();++it){
						std::cout<<std::endl<<it->first<<" "<<it->second;
					}
					msg::error("getTag: missing external entity declaration for entity "+w+filePosition(is,filename));
				}
			}else{
				msg::error("getTag: failed to read entity. Missing ';'"+filePosition(is,filename));
			}
			is.unget(); //hack as getword does not stop at ; but at the next tag. So unget the delimiter which belongs to the next tag.
			break;
		default:
			msg::error("getTag: Didn't find an opening bracket."+filePosition(is,filename));
			MOVETONEXT(is)
			break;
	}


}

//function that will read a tag, but will place the cursor back to original position.
//It returns the number of characters the tag is long (including trailing whitespace)
//inspired by std::peek()
long int peekTag(std::istream &is, Tag &tag){
	//store original position
	std::streamsize op=is.tellg();
	//read the tag
	getTag(is, tag);
	//new position
	std::streamsize np=is.tellg();
	//return to original position
	is.seekg(op);
	return np-op;
}

//move a tag backwards on stream is. return number of reversed characters
long int ungetTag(std::istream &is){
	//remember original position
	std::streamsize count=is.tellg();
	//if not at beginning of file always reverse one (since if cursor is in front oc '<' we still want to reverse one tag
	if (count>0)is.seekg (-1, std::ios::cur);
	//reverse till '<' is found
	while(is.peek()!='<'){
		//if beginning of file is found before '<' throw and exception
		if(is.tellg()<=0 )msg::error("ungetTag: reached beginning of file before finding beginning of tag"+filePosition(is,getCurrentInputFileName()));
		//reverse one character
		is.seekg (-1, std::ios::cur);
	}
	//return number of reversed characters
	return count-is.tellg();
}

//move cursor beyond the closing tag </tag>
void closeTag(std::istream &is, const std::string &tag){
	//read the next tag
	Tag nexttag;
	getTag(is, nexttag);
	//check if next tag really is the closing tag we were looking for, if not throw exception
	if(nexttag.name!="/"+tag) 	msg::error("closeTag: expected closing tag for <"+tag+"> but found <"+nexttag.name+">."+filePosition(is,getCurrentInputFileName()));
}

//overloading closing tag to except the type Tag
void closeTag(std::istream &is, const Tag &tag){
	closeTag(is, tag.name);
}

//peek wether the next tag on stream is is a comment formatted as <!--some text -->
//note comment contains the comment and fw contains the number of character to to forward in order to pass the comment on the stream
bool peek4comment(std::istream &is, std::string &comment, long int &fw){
	//initial conditions
	bool r=false;
	fw=0;

	//store original position
	std::streamsize op=is.tellg();

	//ignore trailing whitespace and opening bracket
	is>>std::ws;
	if (is.peek()=='<') is.ignore(1);

	//look for !-- if not return false
	char c(is.get()), ch('n');
	switch (c) {
		case '!':
			if (is.get()=='-') {
				if (is.get()=='-') {
					//comment found
					r= true;
					//add start of comment
					comment="!--";
					//read commment till --> is encountered
					while (!is.eof() && is.peek() != '>'){
						while (!is.eof() && is.peek() != '-'){
							// read the comment
							ch='n';
							while (!is.eof() && ch!='-') {
								is.get(ch);
								comment += ch;
							}
						}
						//add the last character that was peeked at
						if(!is.eof()) comment+= is.get();
					}
				}
			}
			break;
		case '?' :
			//comment found
			r=true;
			//add start of comment
			comment="?";
			//read rest of comment
			while (!is.eof() ){
				ch='n';
				while (!is.eof() && ch!='?') {
					is.get(ch);
					comment += ch;
				}
				//add the last character that was peeked at
				if(!is.eof()) {
					is.get(ch);
					comment+= ch;
					if(ch=='>') break;
				}
			}
			break;
		default:
			break;
	}
	//see if loop terminated because end of file was reached
	if (is.eof()) msg::error("peek4comment: reached end of file while searching for comment terminator in file "+getCurrentInputFileName());
	//store new position
	std::streamsize np=is.tellg();
	fw=np-op+1;//+1 to include the trailing '>'

	//restore old position
	is.seekg(op);

	//return succes/failure to find a comment
	return r;
}

//overload peek4comment to ignore the content of the comment
bool peek4comment(std::istream &is, long int &fw){
	std::string s;
	return peek4comment(is,s,fw);
}

//skip any comments that are sitting on the stream in front of the cursor
void skipComments(std::istream &is){
	long int fw;
	while (peek4comment(is,fw)){
		is.seekg (fw, std::ios::cur);
	}
}

