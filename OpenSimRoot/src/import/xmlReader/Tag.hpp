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
#ifndef TAG_HPP_
#define TAG_HPP_


#include <string>
#include <map>
#include <iostream>
#include "StreamPositionInfo.hpp"

class Tag;


//Try to read formated data from STREAM and store it in STORE
#define MOVETONEXT(STREAM)\
		while(STREAM && !STREAM.eof()){\
			char ctmp=STREAM.get();\
			if(ctmp=='<' || ctmp=='!' || ctmp=='&'){\
				STREAM.unget();\
				break;\
			}\
		}

#define READTAG(STREAM,STORE)\
	STREAM.clear();\
	STREAM>>STORE;\
	if(!STREAM){\
		STREAM.clear();\
		msg::error("readTag: failed to read data."+filePosition(STREAM));\
		MOVETONEXT(STREAM)\
	}\
	STREAM>>std::ws;\

//Looks for opening tag corresponding to TAGNAME and stores it in openingtag.
#define OPENINGTAG(TAGNAME,STREAM)\
	/*check for possible errors on stream*/\
	CHECKSTREAM(STREAM)\
	/*ignore any comments*/\
	int fw;\
	while (peek4comment(is,fw)) is.seekg (fw, std::ios::cur);\
	/*find opening tag*/\
	Tag openingtag;\
	getTag(STREAM,openingtag);\
	/*check opening tag*/\
	if (!(openingtag == TAGNAME)) {\
		msg::error ("OPENINGTAG: Expected opening tag "+std::string(TAGNAME)+" but found "+openingtag.name+"."+filePosition(is));\
		MOVETONEXT(STREAM)\
	}

#define OPENINGTAG_REREAD(TAGNAME,STREAM)\
	/*referse 1 tag and read opening tag*/\
	ungetTag(STREAM);\
	OPENINGTAG(TAGNAME,STREAM)


/*A loop looks for tags and stores their data in store. A loop is build like this:
 * TAGLOOPBEGIN
 * minimially one STORE4TAG, STORE4TAG_ONCE, OR ACTION4TAG
 * TAGLOOPEND OF TAGLOOPEND_POSTACTION
 */


#define TAGLOOPBEGIN(TAGNAME,STREAM) \
	/*closingtag*/\
	std::string	closingtagname=TAGNAME;\
	closingtagname.insert(closingtagname.begin(),'/');\
	/*loop till closing tag is found OR end of file*/\
	Tag tag;\
	while (!STREAM.eof()){\
		STREAM.clear();\
		int fw;\
		if(peek4comment(is,fw)){\
			/*ignore comments*/\
			is.seekg (fw, std::ios::cur);\
		} else {\
			/*not a comment, store position and read the tag*/\
			const std::streamsize op=is.tellg();\
			getTag(STREAM,tag);\
			/*evaluate tags 1)it is the closing tag: break the loop 2) it is one of the other tags: set in store4tag etc, 3) it is unexpected tag: throw exception*/\
			if(tag==closingtagname){\
				is.seekg(op); /*put closing cursor before closing tag*/\
				break;\
			}

//read and store a tagdata when it matches TAGNAME
#define STORE4TAG(TAGNAME, STORE, STREAM)\
			else if (tag==TAGNAME){\
				READTAG(STREAM, STORE)\
				closeTag(STREAM, TAGNAME);\
			}

//read and store a tagdata when it matches TAGNAME and set FOUND to true
#define STORE4TAG_ONCE(TAGNAME, STORE, FOUND,STREAM)\
			else if (tag==TAGNAME){\
				if (FOUND) msg::error("STORE4TAG_ONCE: found tag "+tag.name+" twice "+filePosition(STREAM));\
				READTAG(STREAM, STORE)\
				closeTag(STREAM, TAGNAME);\
				FOUND=true;\
			}

//perform a custom action on finding a tag
#define ACTION4TAG(TAGNAME, ACTION)\
			else if (tag==TAGNAME){\
				ACTION;\
			}

//end the tagloop
#define TAGLOOPEND(STREAM) \
			else{\
				msg::error("TAGLOOPEND: unexpected tag: <"+tag.name+">"+filePosition(STREAM));\
			}\
		}/*close if (comment) ..else.. */\
		if(!STREAM) msg::error("TAGLOOPEND: error on stream during tagloop");\
	}/*close while loop*/

//end the tagloop. perform custom action after each tag evaluation
#define TAGLOOPEND_POSTACTION(STREAM,POSTACTION) \
			else{\
				msg::error("TAGLOOPEND_POSTACTION: unexpected tag: <"+tag.name+">"+filePosition(STREAM));\
			}\
		}/*his bracket closes if (comment) ..else..  started in TAGLOOPBEGIN */\
		if(!STREAM) msg::error("TAGLOOPEND_POSTACTION: error on stream during tagloop");\
		POSTACTION;\
	}/*close while loop*/



//==================reading tags==================================
// these functions are similar to the std::getline, get and peek functions

//read a tag from the stream and store it in tag
void getTag(std::istream &is, Tag &tag, const std::string & filename="");

//read a tag from the stream and store it in tag, restore cursor position
long int peekTag(std::istream &is, Tag &tag);

//reverses cursor till '<' is found. return number of characters reversed.
long int ungetTag(std::istream &is);

//take closingTag matching tag (either string or of type Tag) of the stream. Throw exceptions if something else is found
void closeTag(std::istream &is, const std::string &tag);
void closeTag(std::istream &is, const Tag &tag);

//see wether next tag is a comment. store numbers of characters to pass the comment in fw. (store the comment in comment)
bool peek4comment(std::istream &is, std::string &comment, long int &fw);
bool peek4comment(std::istream &is, long int &fw);
void skipComments(std::istream &is);


#endif /*TAG_HPP_*/

