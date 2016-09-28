#pragma once
#include <exception>
#include <string>

class DataException : public std::exception {
private:
	std::string message;
public:
	DataException(std::string msg) : message(msg) {}
	const char* what() const override { return message.c_str(); }
};
