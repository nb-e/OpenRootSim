#include <string>


// Used to report build number at runtime.
// To enable, compile with `-DGITHASH="<your revision here>"`

#ifndef GITVERSION_HPP_
#define GITVERSION_HPP_

#ifdef GITHASH
	#define str(x) #x
	#define xstr(x) str(x) // bah, macro string expansion rules
	static std::string gitversion = xstr(GITHASH);
#else
	static std::string gitversion = "revision not known";
#endif

#endif /*GITVERSION_HPP_*/