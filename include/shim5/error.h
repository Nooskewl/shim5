#ifndef NOO_ERROR_H
#define NOO_ERROR_H

#include "shim5/main.h"

namespace noo {

namespace util {

class SHIM5_EXPORT Error {
public:
	Error();
	Error(std::string error_message);
	virtual ~Error();
	
	std::string error_message;
};

class SHIM5_EXPORT MemoryError : public Error {
public:
	MemoryError(std::string error_message);
	virtual ~MemoryError();
};

class SHIM5_EXPORT LoadError : public Error {
public:
	LoadError(std::string error_message);
	virtual ~LoadError();
};

class SHIM5_EXPORT FileNotFoundError : public Error {
public:
	FileNotFoundError(std::string error_message);
	virtual ~FileNotFoundError();
};

class SHIM5_EXPORT ParseError : public Error {
public:
	ParseError(std::string error_message);
	virtual ~ParseError();
};

class SHIM5_EXPORT GLError : public Error {
public:
	GLError(std::string error_message);
	virtual ~GLError();
};

} // End namespace util

} // End namespace noo

#endif // NOO_ERROR_H
