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

#define FORMULAINTEPRETER
#ifdef FORMULAINTEPRETER

#include "../../engine/SimulaDynamic.hpp"
#include "../../cli/Messages.hpp"
#include "fastmathparser/exprtk.hpp"

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;
typedef exprtk::parser_error::type eerror_t;
typedef typename parser_t::dependent_entity_collector::symbol_t symbol_t;


///A symbol resolver which we will use to hook up the variables in the formula to a SimulaBase simulator.
template<typename T>
struct UserDefinedSymbolResolver: public parser_t::unknown_symbol_resolver {
	typedef typename parser_t::unknown_symbol_resolver usr_t;


	UserDefinedSymbolResolver(SimulaBase* p, std::map<std::string, SimulaBase*> & symbolList) :
			usr_t(usr_t::e_usrmode_extended),
			pSD(p),
			symbolList_(symbolList),
			ptl(p)
	{

		//check if variablePaths container is declared.
		SimulaBase *po = pSD->existingChild("variablePaths");
		if (po)
			ptl = po;

	}

	virtual bool process(const std::string& unknown_symbol,
			symbol_table_t& symbol_table, std::string& error_message) {

		//look up sting
		SimulaBase* p = ptl->existingChild(unknown_symbol);
		if (p) {
			//path to symbol found
			std::string path;
			p->get(path);
			p = pSD->getParent()->getPath(path);
		} else {
			if(unknown_symbol=="t"){
				p = NULL;
			}else{
				p = pSD->getParent()->getChild(unknown_symbol);
			}
		}

		//enter it into the symbolList
		symbolList_[unknown_symbol] = p;
		bool result = symbol_table.create_variable(unknown_symbol, 0);

		if (!result) {
			error_message = "FormulaInterpreter: Failed to create variable...";
		}

		return result;
	}
private:
	SimulaBase *pSD;
	std::map<std::string, SimulaBase*> & symbolList_;
	SimulaBase *ptl;
};

///class wrapping the exprtk formula interpreter https://fastmathparser.codeplex.com
class FormulaInterpreter: public DerivativeBase {
protected:
	expression_t expression_; //holds the user defined expression
	std::map<std::string, SimulaBase*> symbolList; //for mapping symbolnames to simulabase pointers.

public:
	FormulaInterpreter(SimulaDynamic* const pSV) :
			DerivativeBase(pSV) {
		//Get formula string
		std::string expression_str;
		pSD->getChild("formula")->get(expression_str);

		//We leaf on purpose the symbol table empty here, so they are resolve later
		symbol_table_t symbol_table;
		expression_.register_symbol_table(symbol_table);

		//check if this parses
		parser_t parser;
		UserDefinedSymbolResolver<double> musr(pSD,symbolList);
		parser.enable_unknown_symbol_resolver(&musr);

		//compile the expression, but now with the complete symbol table.
		if (!parser.compile(expression_str, expression_)) {
			msg::error(
					"FormulaInterpreter: " + parser.error() + " in formula "
							+ expression_str.c_str() + " defined in "
							+ pSD->getPath());

		}

	}
	std::string getName() const {
		return "useFormula";
	}
protected:
	void calculate(const Time &t, double& result) {

		//update the values in the symbol table then compute

		//get a reference to the symbol table
		auto &st = expression_.get_symbol_table(0);

		//get a vector of symbol names in the table
		std::vector<std::string> vl;
		st.get_variable_list(vl);

		//for each symbolname, get a reference to each value, look up the value and set it.
		for (auto &k : vl) {
			//find the simulabase pointer that goes with the symbol
			auto it=symbolList.find(k);
			//retrieve the value for the symbol table
			if(it!=symbolList.end()){
				if(it->second){
					it->second->get(t, st.variable_ref(k) );
				}else{
					//asume we return t.
					st.variable_ref(k)=t;
				}
			}else{
				msg::error("FormulaInterpreter: unknown symbol.");
			}
		}

		//compute
		result = expression_.value();
	}
};

DerivativeBase * newInstantiationFormulaInterpreter(SimulaDynamic* const pSD) {
	return new FormulaInterpreter(pSD);
}

//registration of classes
static class AutoRegisterFormulaInterpreterInstantiationFunctions {
public:
	AutoRegisterFormulaInterpreterInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["useFormula"] =
				newInstantiationFormulaInterpreter;
	}
} p875767rtz65453456758654;

#endif
