#pragma once
#include <string>

class User {
private:
	std::string name;
	std::string password;
	std::string role;
public:
	User() {}
	User(std::string& Name, std::string& Password, std::string& Role) : name(Name), password(Password), role(Role) {}
	std::string getName() const { return this->name; }
	std::string getPassword() const { return this->password; }
	std::string getRole() const { return this->role; }
	void setName(std::string& Name) { this->name = Name; }
	void setPassword(std::string& Password) { this->password = Password; }
	void setRole(std::string& Role) { this->role = Role; }
	~User() {}

	// for set<User> usage 
	friend bool operator<(const User& one, const User& another) { return one.getName() < another.getName(); }
};

// for finding User in set<User> by name directly
class findByName {
private:
	std::string name;
public:
	findByName(std::string& Name) : name(Name) {}
	bool operator()(const User& user) { return user.getName() == this->name; }
};
