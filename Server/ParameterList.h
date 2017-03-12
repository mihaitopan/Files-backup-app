#pragma once
#ifndef _PARAMETER_LIST
#define _PARAMETER_LIST

#include "Exceptions.h"
#include <vector>
#include <map>

class ParameterList {
private:
	std::map<int, std::vector<unsigned char>> structure;
	size_t dimension;
public:
	int Add(int id, int value);
	int Add(int id, const std::string& value);
	int Add(int id, const std::vector<unsigned char>& value);
	int Add(int id, ParameterList& value);
public:
	int Get(int id, int& value);
	int Get(int id, std::string& value);
	int Get(int id, std::vector<unsigned char>& value);
	int Get(int id, ParameterList& value);
public:
	ParameterList() : dimension(0) {}
	void setDimension(size_t Dimension) { this->dimension = Dimension; }
	void SaveToBuffer(std::vector<unsigned char>& buffer);
	void LoadFromBuffer(const std::vector<unsigned char>& buffer);
	~ParameterList() {}
};

inline int ParameterList::Add(int id, int value) {
	try {
		if (this->structure.find(id) != this->structure.end()) {
			throw DataException("id already in map");
		}
		std::vector<unsigned char> info;
		info.resize(sizeof(value));
		std::memcpy(info.data(), &value, sizeof(value));
		this->structure.insert(std::pair<int, std::vector<unsigned char>>(id, info));
		this->dimension += 2 * sizeof(int) + sizeof(value);
		return 0;
	} catch (...) {
		return 1;
	}
}
inline int ParameterList::Add(int id, const std::string& value) {
	try {
		if (this->structure.find(id) != this->structure.end()) {
			throw DataException("id already in map");
		}
		std::vector<unsigned char> info;
		info.resize(value.length() + 1);
		std::memcpy(info.data(), value.c_str(), value.length() + 1);
		this->structure.insert(std::pair<int, std::vector<unsigned char>>(id, info));
		this->dimension += 2 * sizeof(int) + value.length() + 1;
		return 0;
	} catch (...) {
		return 1;
	}
}
inline int ParameterList::Add(int id, const std::vector<unsigned char>& value) {
	try {
		if (this->structure.find(id) != this->structure.end()) {
			throw DataException("id already in map");
		}
		std::vector<unsigned char> info;
		info.resize(value.size());
		std::memcpy(info.data(), value.data(), value.size());
		this->structure.insert(std::pair<int, std::vector<unsigned char>>(id, info));
		this->dimension += 2 * sizeof(int) + value.size();
		return 0;
	} catch (...) {
		return 1;
	}
}
inline int ParameterList::Add(int id, ParameterList& value) {
	try {
		if (this->structure.find(id) != this->structure.end()) {
			throw DataException("id already in map");
		}
		std::vector<unsigned char> Buffer;
		value.SaveToBuffer(Buffer);
		this->structure.insert(std::pair<int, std::vector<unsigned char>>(id, Buffer));
		this->dimension += 2 * sizeof(int) + Buffer.size();
		return 0;
	} catch (...) {
		return 1;
	}
}

inline int ParameterList::Get(int id, int& value) {
	try {
		std::vector<unsigned char> info = this->structure.at(id);
		std::memcpy((void*)&value, info.data(), info.size());
		return 0;
	} catch (...) {
		return 1;
	}
}
inline int ParameterList::Get(int id, std::string& value) {
	try {
		std::vector<unsigned char> info = this->structure.at(id);
		unsigned char* string = new unsigned char[info.size()];
		std::memcpy((void*)string, info.data(), info.size());
		value = std::string((char*)string);
		delete string;
		return 0;
	} catch (...) {
		return 1;
	}
}
inline int ParameterList::Get(int id, std::vector<unsigned char>& value) {
	try {
		std::vector<unsigned char> info = this->structure.at(id);
		value.resize(info.size());
		std::memcpy(value.data(), info.data(), info.size());
		return 0;
	} catch (...) {
		return 1;
	}
}
inline int ParameterList::Get(int id, ParameterList& value) {
	try {
		std::vector<unsigned char> info = this->structure.at(id);
		value.LoadFromBuffer(info);
		value.setDimension(info.size());
		return 0;
	} catch (...) {
		return 1;
	}
}


inline void ParameterList::SaveToBuffer(std::vector<unsigned char>& Buffer) {
	int id, size; unsigned char* data; size_t pos = 0;
	try {
		Buffer.resize(this->dimension);
		for (std::map<int, std::vector<unsigned char>>::iterator it = this->structure.begin(); it != this->structure.end(); ++it) {
			id = it->first; size = (int)it->second.size(); data = it->second.data();

			std::memcpy(Buffer.data() + pos, &id, sizeof(id)); pos += sizeof(id);
			std::memcpy(Buffer.data() + pos, &size, sizeof(size)); pos += sizeof(size);
			std::memcpy(Buffer.data() + pos, data, size); pos += size;
		}
	} catch (...) {
		throw DataException("given buffer is invalid");
	}
}

inline void ParameterList::LoadFromBuffer(const std::vector<unsigned char>& Buffer) {
	int id, size; size_t pos = 0;
	try {
		while (pos < Buffer.size()) {
			std::vector<unsigned char> info;

			std::memcpy(&id, Buffer.data() + pos, sizeof(id)); pos += sizeof(id);
			std::memcpy(&size, Buffer.data() + pos, sizeof(size)); pos += sizeof(size);
			info.resize(size);
			std::memcpy(info.data(), Buffer.data() + pos, size); pos += size;

			if (this->structure.find(id) != this->structure.end()) {
				continue;
			}
			this->structure.insert(std::pair<int, std::vector<unsigned char>>(id, info));
			this->dimension += 2 * sizeof(int) + size;
		}
	} catch (...) {
		throw DataException("given buffer is invalid");
	}
}

#endif
