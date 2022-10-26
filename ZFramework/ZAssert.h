#pragma once

#include <set>
#include <algorithm>
#include <string>
#include <assert.h>


static std::set<std::string> gZAssertExclusionSet;		// This is a global set of excluded assertions.
static char gZAssertBuf[256];							// Buffer for creating assert identifier

#ifdef _DEBUG

// CEASSERT(assertion) - Displays a generic assertion failed dialog box
#define ZASSERT(x) {\
	if (!(x)) {\
	printf(gZAssertBuf, "%s%d", __FILE__, __LINE__);\
	std::string sFileAndLine(gZAssertBuf);\
	if (find(gZAssertExclusionSet.begin(), gZAssertExclusionSet.end(), sFileAndLine) == gZAssertExclusionSet.end()) \
{\
	assert(x);\
}\
	}\
}

#define ZRELEASE_ASSERT(x) ZASSERT(x)

// CEASSERT_MESSAGE(assertion, message) - Displays a custom message in the dialog box if the assertion fails
#define ZASSERT_MESSAGE(x, msg)	{\
	assert(x);\
}
#else
#define ZASSERT(x)
#define ZRELEASE_ASSERT(x) { if (!(x)) { __debugbreak(); } }
#define ZASSERT_MESSAGE(x, msg)
#endif

