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

#ifndef SRC_ENGINE_DATADEFINITIONS_CUSTOMMAP_HPP_
#define SRC_ENGINE_DATADEFINITIONS_CUSTOMMAP_HPP_
#include <vector>
#include <functional>
#include <algorithm>
#include "../../cli/Messages.hpp"
#include <cmath>
#include "Time.hpp"
#include <iostream>
#include <iomanip>

//#define REPORTSIZE
#ifdef REPORTSIZE
#include "../../tools/StringExtensions.hpp"
#endif

template<typename _Key, typename _Tp, typename _Compare = std::less<_Key> >
struct osrMapLess {
	constexpr bool operator()(const std::pair<_Key, _Tp> &l,
			const std::pair<_Key, _Tp> &g, _Compare f = _Compare()) const {
		return f(l.first, g.first);
	}
	constexpr bool operator()(const std::pair<_Key, _Tp> &l,const _Key &g, _Compare f = _Compare()) const {
		return f(l.first, g);
	}
	constexpr bool operator()(const _Key &l,const std::pair<_Key, _Tp> &g, _Compare f = _Compare()) const {
		return f(l, g.first);
	}
};



template<typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
		typename _Alloc = std::allocator<std::pair<_Key, _Tp> >, int reserve_size=6 >
class osrMap {
public:
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef std::pair<_Key, _Tp> _storeType;
	typedef typename std::vector<_storeType, _Alloc> _store;
	typedef typename _store::iterator iterator;
	typedef typename _store::const_iterator const_iterator;
	typedef typename _store::reverse_iterator reverse_iterator;
	typedef typename _store::const_reverse_iterator const_reverse_iterator;
	typedef osrMapLess<_Key, _Tp, _Compare> _less;

	osrMap():table()
	//,gc(false)
	{
		table.reserve(reserve_size);
	}


/*	void deleteHistory(){
		gc=true;
	}
	void keepHistory(){
		gc=false;
	}*/
	mapped_type& operator[](const key_type& k) {
		auto it = std::lower_bound(table.begin(), table.end(),k, _less());
		if (it == table.end() || std::fabs(it->first - k)>TIMEERROR) {
			it=this->insert(it,_storeType(k, mapped_type())); //insert
		}
		return (it->second);
	}
	const_iterator end() const {
		return table.end();
	}
	const_iterator begin() const {
		return table.begin();
	}
	const_reverse_iterator rend() const {
		return table.rend();
	}
	const_reverse_iterator rbegin() const {
		return table.rbegin();
	}
	iterator end() {
		return table.end();
	}
	iterator begin() {
		return table.begin();
	}
	reverse_iterator rend() {
		return table.rend();
	}
	reverse_iterator rbegin() {
		return table.rbegin();
	}

	typedef std::pair<iterator, bool> itb;
	itb insert(const _storeType& x) { //todo move operator &&
		if (table.empty()) {
			table.push_back(x);
			return(itb(table.begin(),true));
		} else {
			//linear, instead of exponential increase of the capacity
			if(table.capacity()==table.size()) {
				table.reserve(table.capacity()+reserve_size);
#ifdef REPORTSIZE
				if(table.capacity()>20){
					msg::warning("table.capacity>20");
				}else{
					msg::warning("table.capacity="+convertToString(table.capacity()));
				}
#endif
			}
			//find the lower bound
			auto f = _less();
			auto it=std::lower_bound(table.begin(), table.end(),x, f);
			iterator r;
			bool success=true;
			if(it==table.end()){
				table.push_back(x);
				r=(--table.end());
			}else if(std::fabs(it->first-x.first)<TIMEERROR){
				//it->second=x.second;
				//msg::warning("double insert");
				r=(it);
				success=false;
			}else{
				r=(table.insert(it,x));
			}
			if(table.capacity()>4*table.size()+reserve_size) table.shrink_to_fit();

			return itb(r,success);
		}

	}

	iterator insert(iterator pos, const _storeType& x) {
		if (table.empty()) {
			table.push_back(x);
			return(table.begin());
		} else {
			//linear, instead of exponential increase of the capacity
			if(table.capacity()==table.size()){
				table.reserve(table.capacity()+reserve_size);
#ifdef REPORTSIZE
				if(table.capacity()>20){
					msg::warning("table.capacity>20");
				}else{
					msg::warning("table.capacity="+convertToString(table.capacity()));
				}
#endif
			}
			//find the lower bound
			auto f = _less();
			iterator it;
			/*if(pos==table.end()){
				it=std::lower_bound(table.begin(), table.end(),x, f);
				if(it==pos){
				msg::warning("not using pos it did not changed");
				}else{
					msg::warning("not using pos it changed");
				}
			}else if(pos->first<=x.first){
				it=std::lower_bound(pos, table.end(),x, f);
				if(it==pos){
					msg::warning("using pos it did not changed");
				}else{
					msg::warning("using pos it changed");
				}
			}else{
				it=std::lower_bound(table.begin(), table.end(),x, f);
				if(it==pos){
					msg::warning("not using pos2 it did not changed");
				}else{
					msg::warning("not using pos2 it changed");
				}
			}segfaults on partialpredefinedroot, somehow pos seems to be invalidated?*/
			it=std::lower_bound(table.begin(), table.end(),x, f);
			iterator r;
			if(it==table.end()){
				table.push_back(x);
				r=(--table.end());
			}else if(std::fabs(it->first-x.first)<TIMEERROR){
				//it->second=x.second;
				//msg::warning("double insert");
				r=(it);
			}else{
				r=(table.insert(it,x));
			}
			if(table.capacity()>4*table.size()+reserve_size) table.shrink_to_fit();
			return r;
		}

	}

	size_t size() const {
		return table.size();
	}
	bool empty() const {
		return table.empty();
	}

	void erase(const_iterator s, const_iterator e) {
		table.erase(s, e);
	}
	void erase(const_iterator s) {
		table.erase(s);
	}
	void erase(const key_type & k) {
		auto it=find(k);
		if(it!=table.end()) table.erase(it);
	}
	const_iterator lower_bound(const key_type & k) const {
		auto r = std::lower_bound(table.begin(), table.end(),
				_storeType(k, _Tp()), _less());
		return (r);
	}
	const_iterator upper_bound(const key_type & k) const {
		auto r = std::upper_bound(table.begin(), table.end(), k, _less());
		return (r);
	}
	const_iterator find(const key_type & k)const{
		auto it = std::lower_bound(table.begin(), table.end(),k, _less());
		if(it!=table.end() && it->first==k){
			return(it);
		}else{
			return(table.end());
		}
	}

private:
	_store table;
//	bool gc;
};

#endif /* SRC_ENGINE_DATADEFINITIONS_CUSTOMMAP_HPP_ */
