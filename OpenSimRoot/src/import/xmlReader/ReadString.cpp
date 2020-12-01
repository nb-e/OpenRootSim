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
#include "ReadString.hpp"
#include "StreamPositionInfo.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"

//==============Reading text from files==============================================

//this function overloads istream operator >> for string. It causes one word to be read as defined by the getWord function
//note: the delimiter is left on the stream just as when you read a character, States etc. 
std::istream &operator>>(std::istream &is, std::string  &s){
	if(!is) msg::error(">>string: Received stream with failbit set");
	//remove white space
	is>>std::ws;
	//read the next word (till the next bracket - ignore any end of line characters
	while (!is.eof() && is.peek()!='<'){
		if(is.peek()!='\n') {
			s+=is.get();
		}else{
			is.get();
		}
	}
	//remove trailing whitespace 
	while(!s.empty() && (*(s.rbegin())==' ' || *(s.rbegin())=='\t' || *(s.rbegin())=='\n')) s.erase(--s.end());
	//check for empty strings
	if(s.empty()) msg::error(">>std::string: missing string."+filePosition(is));
	//return the word
	return is;
}

//reads a word from stream is and puts it into s. The delimiter is also read and placed in ch.
//this is very similar to the std::getLine function, except that it uses more than one delimiter
//note: delimiter taken from stream, just as getLine works. 
void getWord(std::istream &is, std::string &s, char &ch){
	if(!is) {
		msg::error("getWord: Received stream with failbit set");
		return;
	}
	s.clear();
	is>>std::ws;
	while (is.get(ch) && !is.eof() && ch != ' ' && ch != '<' && ch != '>' && ch != '\t' && ch != '\n' && ch != '=') s += ch;
	if (s.empty()){
		msg::error("getWord: No text found before non white space delimiter."+filePosition(is));
		return;
	}
	if(ch == '<' || ch == '>' || ch == '=') {
		//fine continue
	}else{
		is>>std::ws;
	    ch=is.peek();
	    if(ch == '<' || ch == '>' || ch == '=' ) is.ignore(1);
	}
	if(!is) msg::error("getWord: Failbit in stream is set after trying to read a word");	
}

//simplified wrapper for getWord where delimiter ch is dumped
void getWord(std::istream &is, std::string &s){
	char ch;
	getWord(is,s,ch);
}

void getQuotedString(std::istream &is, std::string &s){
	//clear s
	s.clear();
	
	//remove preceding white space
	is>>std::ws;
	
	//store original stream position
	const std::streamsize op=is.tellg();
	
	//look for quote
	const char ch=is.get();
	if(ch!='"' && ch!='\'') msg::error("getQuotedString: expected opening quote."+filePosition(is));
	
	//get the quoted string
	getline(is,s,ch);
	
	//see if end of file is reached or error occured
	if (!is) msg::error("getQuotedString: error occured"+filePosition(is));
	if (is.eof()) {is.seekg(op);msg::error("getQuotedString: missing matching quote?"+filePosition(is));}
	
	//look for illicit characters that suggest we read until the next quoted string
	/*for (std::string::iterator it=s.begin() ; it!=s.end() ; ++it){
		if(*it=='=' | *it=='>' | *it=='<') msg::error("getQuotedString: found illigal character '"+std::to_string<char>(*it)+"' in quoted string"+filePosition(is));
	}*/
}
