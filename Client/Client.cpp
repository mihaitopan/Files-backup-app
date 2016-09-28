#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "ParameterList.h"
#include "Debug.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "8000"
#define BUFFERSIZE 4096


std::string adminCommands() {
	std::string myString = "";
	myString += "available operations: \n";
	myString += "\t as ADMIN: \n";
	myString += "\t\t add_user <name> <password> <admin/user> \n";
	myString += "\t\t list_users \n";
	myString += "\t\t change_pwd <user> \n";
	myString += "\t\t delete_user <name> \n";
	myString += "\t\t rename_user <old_name> <new_name> \n";
	myString += "\t\t impersonate <name> \n";
	myString += "\t\t revert_to_self \n";
	myString += "\t\t exit \n";
	myString += "\t as impersonated USER: \n";
	myString += "\t\t change_pwd \n";
	myString += "\t\t list_files \n";
	myString += "\t\t delete_file <name> \n";
	myString += "\t\t rename_file <name> \n";
	myString += "\t\t upload_file <file> \n";
	myString += "\t\t download_file <file> \n";
	myString += "\t\t exit \n";
	return myString;
}

std::string userCommands() {
	std::string myString = "";
	myString += "available operations: \n";
	myString += "\t change_pwd \n";
	myString += "\t list_files \n";
	myString += "\t delete_file <name> \n";
	myString += "\t rename_file <name> \n";
	myString += "\t upload_file <file> \n";
	myString += "\t download_file <file> \n";
	myString += "\t exit \n";
	return myString;
}

bool writeMessage(SOCKET ConnectSocket, std::string operation, std::string value1, std::string value2, std::string value3) {
	int iResult;
	DWORD bufferSize;
	char messageSize[sizeof(DWORD)];

	std::vector<unsigned char> buffer;
	ParameterList pl = ParameterList();
	pl.Add(0, operation); pl.Add(1, value1); pl.Add(2, value2); pl.Add(3, value3);
	pl.SaveToBuffer(buffer);

	/// write message size
	bufferSize = (DWORD)buffer.size();
	memcpy(messageSize, &bufferSize, sizeof(DWORD));
	iResult = send(ConnectSocket, messageSize, sizeof(DWORD), 0);

	if (SOCKET_ERROR == iResult) {
		std::cout << "package send failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ConnectSocket); 
		WSACleanup(); 
		return false;
	}

	/// write message
	iResult = send(ConnectSocket, (char*)buffer.data(), (DWORD)buffer.size(), 0);

	if (SOCKET_ERROR == iResult) {
		if (GetLastError() == 10053) {		// pipe has been closed in server
			std::cout << "connection with server was lost" << std::endl;
			return true;
		}

		std::cout << "package send failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ConnectSocket);
		WSACleanup(); 
		return false;
	}

	return true;
}

bool readResult(SOCKET ConnectSocket) {
	int iResult;
	DWORD nBytesToRead;

	std::vector<unsigned char> buffer;
	ParameterList pl = ParameterList();
	std::string result;

	/// read result size
	iResult = recv(ConnectSocket, (char*)&nBytesToRead, sizeof(DWORD), 0);
	if (iResult <= 0) {
		std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return false;
	}
	buffer.resize(nBytesToRead);

	/// read result
	iResult = recv(ConnectSocket, (char*)buffer.data(), nBytesToRead, 0);
	if (iResult <= 0) {
		std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ConnectSocket); 
		WSACleanup();
		return false;
	}

	pl.LoadFromBuffer(buffer);
	pl.Get(0, result);
	std::cout << result << std::endl;

	return true;
}

bool adminConsole(SOCKET ConnectSocket) {
	std::string operation, value1, value2, value3;
	
	std::cout << adminCommands() << std::endl;
	while (true) {
		std::cout << "> ";
		std::string command;
		std::getline(std::cin, command);

		value1 = value2 = "";
		std::istringstream iss(command);
		iss >> operation >> value1 >> value2 >> value3;

		if (operation == "exit") { break; }
		if (operation != "change_pwd" && operation != "list_files" && operation != "delete_file" &&
			operation != "rename_file" && operation != "upload_file" && operation != "download_file" &&
			operation != "add_user" && operation != "list_users" && operation != "change_pwd" && operation != "delete_user" &&
			operation != "rename_user" && operation != "impersonate" && operation != "revert_to_self") 
		{
			std::cout << "bad request - " << adminCommands(); 
			continue;
		}

		/// write message
		if (writeMessage(ConnectSocket, operation, value1, value2, value3) == false) { 
			return false; 
		}

		/// read and print result
		if (readResult(ConnectSocket) == false) {
			return false; 
		}
	}

	return true;
}

bool userConsole(SOCKET ConnectSocket) {
	std::string operation, value1, value2, value3;

	std::cout << userCommands() << std::endl;
	while (true) {
		std::cout << "> ";
		std::string command;
		std::getline(std::cin, command);

		value1 = value2 = "";
		std::istringstream iss(command);
		iss >> operation >> value1 >> value2 >> value3;

		if (operation == "exit") { 
			break; 
		}

		if (operation != "change_pwd" && operation != "list_files" && operation != "delete_file" &&
			operation != "rename_file" && operation != "upload_file" && operation != "download_file")
		{
			std::cout << "bad request - " << adminCommands(); 
			continue;
		}

		/// write message
		if (writeMessage(ConnectSocket, operation, value1, value2, value3) == false) { 
			return false;
		}

		/// read and print result
		if (readResult(ConnectSocket) == false) { 
			return false; 
		}
	}

	return true;
}

int __cdecl main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	/// declarations
	WSADATA wsaData;
	SOCKET ConnectSocket;
	struct addrinfo *addrresult = NULL, *ptr = NULL, hints;
	int iResult;
	std::string role;

	/// definitions
	ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

	ConnectSocket = INVALID_SOCKET;

	/// initializing Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed with error: " << iResult << std::endl;
		return 1;
	}

	/// resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &addrresult);
	if (iResult != 0) {
		std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
		WSACleanup(); 
		return 1;
	}

	/// attempting to connect to an address until one succeeds
	while (true) {
		for (ptr = addrresult; ptr != NULL; ptr = ptr->ai_next) {

			/// creating a SOCKET for connecting to server
			ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (INVALID_SOCKET == ConnectSocket) {
				std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
				WSACleanup(); 
				return 1;
			}

			/// connecting to server
			iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (SOCKET_ERROR == iResult) {
				closesocket(ConnectSocket);	
				ConnectSocket = INVALID_SOCKET; 
				continue;
			}

			break;
		}

		if (INVALID_SOCKET != ConnectSocket) { 
			break; 
		}

		std::cout << "Unable to connect to server! - Trying again in 3 seconds." << std::endl;
		Sleep(3000);
	}

	freeaddrinfo(addrresult);

	/// loggin
	{
		std::string loggin, username, password, result;
		std::cout << "please loggin - <username> <password>" << std::endl;
		std::getline(std::cin, loggin);
		std::istringstream iss(loggin);
		iss >> username >> password;

		ParameterList loginIn, loginOut;
		std::vector<unsigned char> buffer;
		DWORD bufferSize;

		/// save username and password to buffer
		loginIn.Add(0, username);
		loginIn.Add(1, password);
		loginIn.SaveToBuffer(buffer);
		bufferSize = (DWORD)buffer.size();

		/// send loggin info
		iResult = send(ConnectSocket, (char*)&bufferSize, sizeof(DWORD), 0);
		iResult = send(ConnectSocket, (char*)buffer.data(), bufferSize, 0);

		/// receive result
		iResult = recv(ConnectSocket, (char*)&bufferSize, sizeof(DWORD), 0);
		buffer.clear(); buffer.resize(bufferSize);
		iResult = recv(ConnectSocket, (char*)buffer.data(), bufferSize, 0);
		loginOut.LoadFromBuffer(buffer);
		loginOut.Get(0, result);

		std::cout << result << std::endl;
		if (result != "logged in successfully") {

			/// shutdown the connection since no more data will be sent
			iResult = shutdown(ConnectSocket, SD_SEND);
			if (SOCKET_ERROR == iResult) {
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket); 
				WSACleanup(); 
				return 1;
			}

			/// cleanup
			closesocket(ConnectSocket);
			WSACleanup();
			return 0;

		} else {
			loginOut.Get(1, role);
		}
	}
	
	if (role == "admin") {
		if (adminConsole(ConnectSocket) == false) { 
			return 1; 
		}
	} else if (role == "user") {
		if (userConsole(ConnectSocket) == false) { 
			return 1; 
		}
	} else {
		std::cout << "user data on server has been corrupted - please don't sue us";
	}

	/// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (SOCKET_ERROR == iResult) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup(); 
		return 1;
	}

	/// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
	return 0;
}
