#pragma once

#include <stdexcept>

#ifdef _DEBUG
#define VOID_ASSERT(expression, message) \
	if(!(expression)) \
	{ \
		/*Logger::error(message, "Assertion");*/ \
		throw std::runtime_error(message); \
	}
#else
#define VOID_ASSERT(expression, message) ((void)0)
#endif

#define VOID_ERROR(expression, message) \
	if(!(expression)) \
	{ \
		/*Logger::error(message, "Assertion");*/ \
		throw std::runtime_error(message); \
	}