#pragma once
#include "User.h"
#include <set>
#include <map>
#include <vector>

#define CONFIG_FILE "ServerApp\\files\\config.xml"
#define BACKUP_DIR std::string("backup")

class Repository {
private:
	std::set<User> users;
	std::string configFile;
	std::string backupDir;
public:
	Repository();
	Repository(std::string ConfigFile, std::string BackupDir);
	void readXML();
	void addUser(std::string& name, std::string& password, std::string& role);
	void renameUser(std::string& oldName, std::string& newName);
	void removeUser(std::string& name);
	void changePasswordUser(std::string name, std::string& password);
	void createUploadPath(std::string username, std::string& filename);
	void createDownloadPath(std::string username, std::string& filename);
	std::string listFiles(std::string username);
	void deleteFile(std::string username, std::string& filename);
	void renameFile(std::string username, std::string& filename, std::string& newfilename);
	void uploadFile(std::string username, std::string& filename);
	void downloadFile(std::string username, std::string& filename);
	std::set<User> getRepo();
	void writeXML();
	~Repository();
};
