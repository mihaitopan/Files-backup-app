#undef UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Repository.h"
#include "ParameterList.h"
#include "Debug.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <iostream>
#include <vector>
#include <memory>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "8000"
#define BUFFERSIZE 4096
#define MAX_USERNAME_LENGTH 50


typedef struct ListenerStruct {
	HANDLE exitListenerEvent;
	Repository* repo;
} ListenerStruct;

typedef struct ClientStruct {
	HANDLE hClientEvents[2];
	SOCKET ClientSocket;
	Repository* repo;
} ClientStruct;

int addition(int a, int b) { 
	return a + b; 
}

int substraction(int a, int b) { 
	return a - b; 
}

std::string concatenation(std::string& a, std::string& b) { 
	return a + b; 
}

std::string MyOperationFunction(Repository* repo, std::string& initialUsername, char* username, std::string& role,
								std::string& operation,	std::string& value1, std::string& value2, std::string& value3) 
{
	std::string result = "done \n";

	if (role == "admin" && operation == "add_user") { /// add user
		try { 
			repo->addUser(value1, value2, value3);
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n"; 
		}

	} else if (role == "admin" && operation == "list_users") { /// list all users
		try { 
			result = "";
			for (auto user : repo->getRepo()) { 
				result += user.getName() + " " + user.getRole() + "\n"; 
			}
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n"; 
		}

	} else if (role == "admin" && operation == "change_pwd") { /// change the password of a user
		try { 
			repo->changePasswordUser(value1, value2);
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n"; 
		}

	} else if (role == "admin" && operation == "delete_user") { /// delete some existent user 
		if (value1 == initialUsername) {
			return "cannot remove current user";
		}

		if (value1 == std::string(username)) { 
			memcpy(username, initialUsername.c_str(), initialUsername.length()); username[value1.length()] = '\0'; 
		}

		try { 
			repo->removeUser(value1);
		} catch (DataException& e) {
			result = std::string(e.what()) + "\n"; 
		}

	} else if (role == "admin" && operation == "rename_user") { /// rename some existent user
		try { 
			repo->renameUser(value1, value2);
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n";
		}

	} else if (role == "admin" && operation == "impersonate") { /// impersonate some existent user
		try { 
			repo->renameUser(value1, value1);
			memcpy(username, value1.c_str(), value1.length()); 
			username[value1.length()] = '\0';
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n";
		}

	} else if (role == "admin" && operation == "revert_to_self") { /// revert to initial self
		memcpy(username, initialUsername.c_str(), initialUsername.length()); 
		username[value1.length()] = '\0';

	} else if (operation == "change_pwd") {
		try { 
			repo->changePasswordUser(std::string(username), value1);
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n"; 
		}

	} else if (operation == "list_files") { /// list all files
		result = repo->listFiles(std::string(username));

	} else if (operation == "delete_file") { /// delete some file
		try { 
			repo->deleteFile(std::string(username), value1);
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n"; 
		}

	} else if (operation == "rename_file") { /// rename some file
		try { 
			repo->renameFile(std::string(username), value1, value2);
		} catch (DataException& e) {
			result = std::string(e.what()) + "\n";
		}

	} else if (operation == "upload_file") { /// upload some file
		try { 
			repo->uploadFile(std::string(username), value1);
		} catch (DataException& e) { 
			result = std::string(e.what()) + "\n"; 
		}

	} else if (operation == "download_file") { /// download some file
		try {
			repo->downloadFile(std::string(username), value1);
		} catch (DataException& e) {
			result = std::string(e.what()) + "\n";
		}

	} else {
		result = "unknown operation - operation skipped"; 
	}

	return result;
}

DWORD __stdcall MyClientThreadFunction(LPVOID LpParam) {
	ClientStruct* cs = (ClientStruct*)LpParam;

	/// declarations
	int iResult; bool loggedIn;
	char username[MAX_USERNAME_LENGTH];
	std::string usernameS, passwordS;
	std::string status, initialUsername, role, operation;
	std::string value1, value2, value3, result;
	std::vector<unsigned char> buffer;
	DWORD dwWaitResult, bufferSize, flags;
	DWORD nBytesDone, nBytesToDo;
	WSABUF DataBuf;
	char BUFF[BUFFERSIZE];
	WSAOVERLAPPED ovr;

	/// initializations
	status = "nowReadMessage";
	nBytesDone = 0; flags = 0; loggedIn = false;
	ZeroMemory(&ovr, sizeof(WSAOVERLAPPED));
	ovr.hEvent = WSACreateEvent(); WSAResetEvent(ovr.hEvent);
	cs->hClientEvents[1] = ovr.hEvent;
	DataBuf.len = sizeof(DWORD);
	DataBuf.buf = BUFF;

	// do the login
	{
		/// set it back to non-blocking
		WSAEventSelect(cs->ClientSocket, ovr.hEvent, 0);
		u_long type = 0;
		iResult = ioctlsocket(cs->ClientSocket, FIONBIO, &type);

		result = "bad username or password";

		/// receive loggin info
		iResult = recv(cs->ClientSocket, (char*)&bufferSize, sizeof(DWORD), 0);
		buffer.resize(bufferSize);
		iResult = recv(cs->ClientSocket, (char*)buffer.data(), bufferSize, 0);
		ParameterList loginIn, loginOut;
		loginIn.LoadFromBuffer(buffer);
		loginIn.Get(0, usernameS); 
		loginIn.Get(1, passwordS);
		for (auto user : cs->repo->getRepo()) {
			if (user.getName() == usernameS && user.getPassword() == passwordS) {

				/// set usernam, initialUsername and role
				memcpy(username, usernameS.c_str(), usernameS.length());
				username[usernameS.length()] = '\0';
				initialUsername = std::string(username);
				role = user.getRole();
				
				/// set loggedIn
				loggedIn = true; 
				result = "logged in successfully"; 
				break;
			}
		}

		loginOut.Add(0, result);
		if (loggedIn) {	
			loginOut.Add(1, role); 
		}

		buffer.clear();
		loginOut.SaveToBuffer(buffer);
		bufferSize = (DWORD)buffer.size();

		/// send result
		iResult = send(cs->ClientSocket, (char*)&bufferSize, sizeof(DWORD), 0);
		iResult = send(cs->ClientSocket, (char*)buffer.data(), bufferSize, 0);

		if (loggedIn == false) {

			/// shutdown the connection
			iResult = shutdown(cs->ClientSocket, SD_SEND);
			if (SOCKET_ERROR == iResult) {
				std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
				closesocket(cs->ClientSocket); 
				return 1;
			}

			/// cleanup
			closesocket(cs->ClientSocket);
			return 0;
		}
	}

	/// read message size
	nBytesToDo = sizeof(DWORD);
	iResult = WSARecv(cs->ClientSocket, &DataBuf, 1, &nBytesDone, &flags, &ovr, NULL);
	if (SOCKET_ERROR == iResult && WSAGetLastError() != WSA_IO_PENDING) {
		std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
		closesocket(cs->ClientSocket); 
		WSACleanup(); 
		return 1;
	}
	
	/// now loop!
	while (true) {
		dwWaitResult = WSAWaitForMultipleEvents(2, cs->hClientEvents, FALSE, INFINITE, FALSE);

		switch (dwWaitResult) {
		case WAIT_OBJECT_0 + 0:
			/// shutdown the connection
			iResult = shutdown(cs->ClientSocket, SD_SEND);
			if (SOCKET_ERROR == iResult) {
				std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
				closesocket(cs->ClientSocket);
				return 1;
			}

			/// cleanup
			closesocket(cs->ClientSocket);
			return 0;

		case WAIT_OBJECT_0 + 1:
			iResult = WSAGetOverlappedResult(cs->ClientSocket, &ovr, &nBytesDone, FALSE, &flags);
			if (FALSE == iResult) {
				std::cout << "package send or receive failed with error: " << WSAGetLastError() << std::endl;
				closesocket(cs->ClientSocket);
				WSACleanup();
				return 1;

			} else if (nBytesDone == 0) {
				/// socket was close, close connection
				iResult = shutdown(cs->ClientSocket, SD_SEND);
				if (SOCKET_ERROR == iResult) {
					std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
					closesocket(cs->ClientSocket); 
					return 1;
				}

				closesocket(cs->ClientSocket);
				return 0;
			}

			if (status == "nowReadMessage") {
				status = "nowWriteResultSize";
				memcpy(&nBytesToDo, BUFF, sizeof(DWORD));

				/// prepare ovr struct
				ZeroMemory(&ovr, sizeof(WSAOVERLAPPED));
				ovr.hEvent = WSACreateEvent(); WSAResetEvent(ovr.hEvent);
				CloseHandle(cs->hClientEvents[1]); cs->hClientEvents[1] = ovr.hEvent;
				DataBuf.len = nBytesToDo; // tell how much to receive

				/// read message
				buffer.clear(); buffer.resize(nBytesToDo); // resize buffer
				iResult = WSARecv(cs->ClientSocket, &DataBuf, 1, &nBytesDone, &flags, &ovr, NULL);
				if (SOCKET_ERROR == iResult && WSAGetLastError() != WSA_IO_PENDING) {
					std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
					closesocket(cs->ClientSocket); 
					WSACleanup(); 
					return 1;
				}
			} else if (status == "nowWriteResultSize") {
				status = "nowWriteResult";
				memcpy(buffer.data(), BUFF, nBytesDone);

				/// load buffer - compute result - load buffer
				ParameterList pl, plpl;
				pl.LoadFromBuffer(buffer);
				pl.Get(0, operation);
				pl.Get(1, value1);
				pl.Get(2, value2);
				pl.Get(3, value3);
				result = MyOperationFunction(cs->repo, initialUsername, username, role, operation, value1, value2, value3);
				plpl.Add(0, result);
				buffer.clear();
				plpl.SaveToBuffer(buffer);

				/// prepare ovr struct
				ZeroMemory(&ovr, sizeof(WSAOVERLAPPED));
				ovr.hEvent = WSACreateEvent(); WSAResetEvent(ovr.hEvent);
				CloseHandle(cs->hClientEvents[1]); cs->hClientEvents[1] = ovr.hEvent;
				DataBuf.len = sizeof(DWORD); // tell how much to send

				/// write result size
				bufferSize = (DWORD)buffer.size();
				memcpy(BUFF, &bufferSize, sizeof(DWORD));
				nBytesToDo = sizeof(DWORD);
				iResult = WSASend(cs->ClientSocket, &DataBuf, 1, &nBytesDone, 0, &ovr, NULL);
				if (SOCKET_ERROR == iResult && WSAGetLastError() != WSA_IO_PENDING) {
					std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
					closesocket(cs->ClientSocket); 
					WSACleanup(); 
					return 1;
				}
			} else if (status == "nowWriteResult") {
				status = "nowReadMessageSize";

				/// prepare ovr struct
				ZeroMemory(&ovr, sizeof(WSAOVERLAPPED));
				ovr.hEvent = WSACreateEvent(); WSAResetEvent(ovr.hEvent);
				CloseHandle(cs->hClientEvents[1]); cs->hClientEvents[1] = ovr.hEvent;
				DataBuf.len = (DWORD)buffer.size(); // tell how much to send

				/// write result
				memcpy(BUFF, buffer.data(), (DWORD)buffer.size());
				nBytesToDo = (DWORD)buffer.size();
				iResult = WSASend(cs->ClientSocket, &DataBuf, 1, &nBytesDone, 0, &ovr, NULL);
				if (SOCKET_ERROR == iResult && WSAGetLastError() != WSA_IO_PENDING) {
					std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
					closesocket(cs->ClientSocket); 
					WSACleanup();
					return 1;
				}
			} else if (status == "nowReadMessageSize") {
				status = "nowReadMessage";

				/// prepare ovr struct
				ZeroMemory(&ovr, sizeof(WSAOVERLAPPED));
				ovr.hEvent = WSACreateEvent(); WSAResetEvent(ovr.hEvent);
				CloseHandle(cs->hClientEvents[1]); cs->hClientEvents[1] = ovr.hEvent;
				DataBuf.len = sizeof(DWORD); // tell how much to receive

				/// read message size
				nBytesToDo = sizeof(DWORD);
				iResult = WSARecv(cs->ClientSocket, &DataBuf, 1, &nBytesDone, &flags, &ovr, NULL);
				if (SOCKET_ERROR == iResult && WSAGetLastError() != WSA_IO_PENDING) {
					std::cout << "package receive failed with error: " << WSAGetLastError() << std::endl;
					closesocket(cs->ClientSocket);
					WSACleanup();
					return 1;
				}
			} else {
				std::cout << "server-client package delivery failed with error: " << WSAGetLastError() << std::endl;
				closesocket(cs->ClientSocket);
				WSACleanup(); 
				return 1;
			}
		}
	}

	return 0;
}

DWORD __stdcall MyListenerThreadFunction(LPVOID LpParam) {
	ListenerStruct* ls = (ListenerStruct*)LpParam;

	/// declarations
	std::string command;
	int iResult;
	WSADATA wsaData;
	struct addrinfo *result = NULL, hints;
	SOCKET listenSocket, newClientSocket;
	std::vector<std::unique_ptr<ClientStruct>> clientStrucs;
	std::vector<HANDLE> hClientThreads;
	HANDLE hListenerEvents[2], hClientExitEvent;
	DWORD hWaitResult;
	u_long type;

	/// initializations
	listenSocket = INVALID_SOCKET;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	/// initializing Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed with error: " << iResult << std::endl;
		return 1;
	}

	/// resolving the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
		WSACleanup(); 
		return 1;
	}
	
	/// creating a SOCKET for connecting to server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (INVALID_SOCKET == listenSocket) {
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	/// set operations on listenSocket to be non-blocking
	type = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &type);		// the better way is acceptEx
	if (SOCKET_ERROR == iResult) {
		std::cout << "ioctlsocket failed with error: " << iResult << std::endl;
		freeaddrinfo(result); 
		closesocket(listenSocket); 
		WSACleanup(); 
		return 1;
	}

	/// setup of the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (SOCKET_ERROR == iResult) {
		std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	/// create events
	hListenerEvents[0] = ls->exitListenerEvent;
	hListenerEvents[1] = WSACreateEvent(); WSAResetEvent(hListenerEvents[1]);
	WSAEventSelect(listenSocket, hListenerEvents[1], FD_ACCEPT);
	hClientExitEvent = WSACreateEvent(); WSAResetEvent(hClientExitEvent);

	/// listening on a socket
	iResult = listen(listenSocket, SOMAXCONN);
	if (SOCKET_ERROR == iResult) {
		std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup(); 
		return 1;
	}

	/// aceepting clients loop
	while (true) {
		hWaitResult = WaitForMultipleObjects(2, hListenerEvents, FALSE, INFINITE);
		switch (hWaitResult) {
		case WAIT_OBJECT_0:
			/// set exit event
			SetEvent(hClientExitEvent);

			/// wait and close handles
			WaitForMultipleObjects((DWORD)hClientThreads.size(), hClientThreads.data(), TRUE, INFINITE);
			for (std::vector<HANDLE>::iterator it = hClientThreads.begin(); it != hClientThreads.end(); ++it) {
				CloseHandle(*it);
			}
			CloseHandle(hClientExitEvent);

			/// close socket and return
			closesocket(listenSocket);
			return 0;

		case WAIT_OBJECT_0 + 1:
			WSAResetEvent(hListenerEvents[1]);

			/// accept client
			newClientSocket = accept(listenSocket, NULL, NULL);
			if (INVALID_SOCKET == newClientSocket) {
				closesocket(newClientSocket); newClientSocket = accept(listenSocket, NULL, NULL); continue;
			}

			/// store ptr to client structure
			std::unique_ptr<ClientStruct> cs_ptr(new ClientStruct{});
			cs_ptr->ClientSocket = newClientSocket;
			cs_ptr->hClientEvents[0] = hClientExitEvent;
			cs_ptr->hClientEvents[1] = NULL;
			cs_ptr->repo = ls->repo;
			clientStrucs.push_back(std::unique_ptr<ClientStruct>(cs_ptr.get()));
			cs_ptr.release();

			/// create clientThreads
			hClientThreads.push_back(CreateThread(NULL, 0, MyClientThreadFunction, (LPVOID)clientStrucs.back().get(), 0, NULL));
			if (NULL == hClientThreads.back()) {
				std::cout << "CreateThread failed with error: " << GetLastError() << std::endl; continue;
			}
		}
	}

	return 0;
}

int __cdecl main(int argc, char* argv[]) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	ListenerStruct ls;
	HANDLE hListenerThread;
	std::string command, configFile, backupDir;

	/// validating arguments
	if (argc < 2 || argc > 3) {
		std::cout << "Invalid arguments\n";
		std::cout << "\t see -help\n";
		return 1;
	}

	/// help
	if (argc == 2 && std::string(argv[1]) == "-help") {
		std::cout << "\t-help					possible arguments\n";
		std::cout << "\t<xml file> <backup dir>			runs the server\n";
		return 0;
	}

	/// creating repo
	configFile = "";
	backupDir = "";
	if (argc < 2) { 
		configFile = CONFIG_FILE;
		backupDir = BACKUP_DIR;
	} else {
		configFile = std::string(argv[1]);
		backupDir = std::string(argv[2]);
	}
	Repository repo = Repository{ configFile, backupDir };

	/// ListenerStruct
	ls.exitListenerEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("ExitListener"));
	ls.repo = &repo;

	/// listener thread
	hListenerThread = CreateThread(NULL, 0, MyListenerThreadFunction, (LPVOID)&ls, 0, NULL);
	if (NULL == hListenerThread) {
		std::cout << "CreateListenerThread error: " << GetLastError() << std::endl;
		return 1;
	}

	/// console
	std::cout << "available commands:" << std::endl;
	std::cout << "\t exit - for exiting program" << std::endl;
	std::cout << "give your command" << std::endl;
	std::cout << "> ";

	while (true) {
		std::getline(std::cin, command);
		if (command == "exit") {
			/// set exit event
			SetEvent(ls.exitListenerEvent);
			/// wait and close handle
			WaitForSingleObject(hListenerThread, INFINITE);
			CloseHandle(hListenerThread);

			break;
		} else {
			std::cout << "> ";
		}
	}

	return 0;
}
