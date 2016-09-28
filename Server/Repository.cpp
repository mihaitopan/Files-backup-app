#include "Repository.h"
#include "Exceptions.h"
#include "files\tinyxml2.h"
#include <algorithm>
#include <windows.h>


Repository::Repository() { 
	this->configFile = CONFIG_FILE;
	this->backupDir = BACKUP_DIR;

	/// create backup dir
	if (0 == CreateDirectory(this->backupDir.c_str(), NULL) &&
		ERROR_PATH_NOT_FOUND == GetLastError()) {
		throw DataException("directory path doesn't exist or there are no write permissions");
	}

	this->readXML(); 
}

Repository::Repository(std::string ConfigFile, std::string BackupDir) {
	this->configFile = ConfigFile; 
	this->backupDir = BackupDir;

	/// create backup dir
	if (0 == CreateDirectory(this->backupDir.c_str(), NULL) &&
		ERROR_PATH_NOT_FOUND == GetLastError()) {
		throw DataException("directory path doesn't exist or there are no write permissions");
	}

	this->readXML();
}

void Repository::readXML() {
	tinyxml2::XMLDocument file;
	file.LoadFile(this->configFile.c_str());
	tinyxml2::XMLNode* node = file.FirstChildElement("users");
	tinyxml2::XMLElement* element = node->FirstChildElement("user");

	/// add each element in config file into local repository
	while (element != NULL) {
		std::string name = element->Attribute("name");
		std::string password = element->Attribute("password");
		std::string role = element->Attribute("role");

		User user = User(name, password, role);
		try {
			this->users.insert(user);
		} catch (...) {
			throw DataException("invalid xml file");
		}

		element = element->NextSiblingElement();
	}
}

void Repository::addUser(std::string& name, std::string& password, std::string& role) {
	try {
		this->users.insert(User(name, password, role));
	} catch (...) {
		throw DataException("a user with this name already exists");
	}
}

void Repository::renameUser(std::string& oldName, std::string& newName) {
	std::set<User>::iterator it = std::find_if(this->users.begin(), this->users.end(), findByName(oldName));
	if (it == this->users.end()) {
		throw DataException("no such user");
	} else {
		User user = *it;
		this->users.erase(it); /// remove old user 
		user.setName(newName);
		try {
			this->users.insert(user); /// add new user
		} catch (...) {
			throw DataException("a user with this name already exists");
		}
	}
}

void Repository::removeUser(std::string& name) {
	std::set<User>::iterator it = std::find_if(this->users.begin(), this->users.end(), findByName(name));
	if (it == this->users.end()) {
		throw DataException("no such user");
	} else {
		this->users.erase(it);
	}
}

void Repository::changePasswordUser(std::string name, std::string& password) {
	std::set<User>::iterator it = std::find_if(this->users.begin(), this->users.end(), findByName(name));
	if (it == this->users.end()) {
		throw DataException("no such user");
	} else {
		User user = *it;
		this->users.erase(it); /// remove old user 
		user.setPassword(password);
		try {
			this->users.insert(user); /// add new user
		} catch (...) {
			throw DataException("a user with this name already exists");
		}
	}
}

std::vector<std::string> tokenize(const char* str, char c) {
	std::vector<std::string> result;

	do {
		const char *begin = str;
		while (*str != c && *str)
			str++;
		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	return result;
}

/// create upload path folder by folder 
void Repository::createUploadPath(std::string username, std::string& filename) {
	std::string path = "";
	path += this->backupDir + "\\" + username;

	CreateDirectory(path.c_str(), NULL);
	path += "\\";

	std::vector<std::string> tokens = tokenize(filename.c_str(), '\\');
	for (int i = 0; i < tokens.size() - 1; i++) {
		path += tokens[i];
		CreateDirectory(path.c_str(), NULL);
		path += "\\";
	}
}

/// create download path folder by folder 
void Repository::createDownloadPath(std::string username, std::string& filename) {
	std::string path = "";

	std::vector<std::string> tokens = tokenize(filename.c_str(), '\\');
	for (int i = 0; i < tokens.size() - 1; i++) {
		path += tokens[i];
		CreateDirectory(path.c_str(), NULL);
		path += "\\";
	}
}

/// recursively put desired relative paths in the string "result"
bool ListTreeFilesRec(std::string& result, int inPathSize, const char* dir) {
	WIN32_FIND_DATA fileData;
	HANDLE hFind = NULL;
	char absolutePath[2048];

	// *.* -> we want everything!
#pragma warning(suppress: 4996) /// sprintf is safe
	std::sprintf(absolutePath, "%s\\*.*", dir);

	/// doublecheck the handle value
	hFind = FindFirstFile(absolutePath, &fileData);
	if (INVALID_HANDLE_VALUE == hFind) { 
		return false; 
	}

	do {
		/// if we do find something
		if (strcmp(fileData.cFileName, ".") != 0 && strcmp(fileData.cFileName, "..") != 0) {
#pragma warning(suppress: 4996) /// sprintf is safe
			std::sprintf(absolutePath, "%s\\%s", dir, fileData.cFileName);

			if (fileData.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY) {		/// if it's a directory we continue the recursive algorithm
				ListTreeFilesRec(result, inPathSize, absolutePath);
			} else {														/// if it's a file we add the path to the string
				char* relativePath = absolutePath + inPathSize;
				result += std::string(relativePath) + "\n";
			}
		}
	} while (FindNextFile(hFind, &fileData));

	FindClose(hFind);
	return true;
}

std::string Repository::listFiles(std::string username) {
	std::string result = "";
	std::string source = this->backupDir + "\\" + username + "\\";

	if (false == ListTreeFilesRec(result, (int)source.length() + 1, source.c_str())) {
		return "your backup folder is empty - go add some files ;) \n";
	}

	return result;
}

void Repository::deleteFile(std::string username, std::string& filename) {
	if(remove((this->backupDir + username + "\\" + filename).c_str()) != 0) {
		throw DataException("no such file");
	}
}

void Repository::renameFile(std::string username, std::string& oldfilename, std::string& newfilename) {
	this->createUploadPath(username, newfilename);

	/// copy the old file as the new file
	if (CopyFile((this->backupDir + "\\" + username + "\\" + oldfilename).c_str(), (this->backupDir + "\\" + username + "\\" + newfilename).c_str(), false) == 0) {
		throw DataException("no such file");
	}

	/// delete old file
	if (remove((this->backupDir + "\\" + username + "\\" + oldfilename).c_str()) != 0) {
		throw DataException("no such file");
	}
}

void Repository::uploadFile(std::string username, std::string& filename) {
	this->createUploadPath(username, filename);
	if (CopyFile(filename.c_str(), (this->backupDir + "\\" + username + "\\" + filename).c_str(), false) == 0) {
		throw DataException("no such file");
	}
}

void Repository::downloadFile(std::string username, std::string& filename) {
	this->createDownloadPath(username, filename);
	if (CopyFile((this->backupDir + "\\" + username + "\\" + filename).c_str(), filename.c_str(), false) == 0) {
		throw DataException("no such file");
	}
}

std::set<User> Repository::getRepo() {
	return this->users; 
}

void Repository::writeXML() {
	tinyxml2::XMLDocument file;
	tinyxml2::XMLNode* root = file.NewElement("users");
	file.InsertFirstChild(root);

	/// save each user in repo to xml file
	for (std::set<User>::iterator it = this->users.begin(); it != this->users.end(); ++it) {
		User user = *it;
		tinyxml2::XMLElement* element = file.NewElement("user");
		element->SetAttribute("name", user.getName().c_str());
		element->SetAttribute("password", user.getPassword().c_str());
		element->SetAttribute("role", user.getRole().c_str());
		root->InsertEndChild(element);
	}

	file.SaveFile(this->configFile.c_str());
}

Repository::~Repository() { 
	this->writeXML(); 
}
