#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include "../util.h"

// #pragma comment(lib, "Ws2_32.lib")

#define PORT 13377

DWORD WINAPI thread_func(void *data) {
	SOCKET client_socket = (SOCKET)(uintptr_t)data;

	char buf[1024];

	for (;;) {
		int bytes_recv = recv(client_socket, buf, 1024, 0);
		if (bytes_recv == 0) break;
		printf("received %d bytes: %.*s\n", bytes_recv, bytes_recv, buf);
	}

	printf("client disconnected\n");
	defer { closesocket(client_socket); }
}

int main() {
	WSADATA wsa_data;
	assert(WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
	defer { WSACleanup(); }

	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(listen_socket != INVALID_SOCKET);
	defer { closesocket(listen_socket); }

	BOOL opt = TRUE;
	assert(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) != SOCKET_ERROR);

	struct sockaddr_in server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	assert(bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != SOCKET_ERROR);

	assert(listen(listen_socket, SOMAXCONN) != SOCKET_ERROR);

	printf("server listening on port %d\n", PORT);

	for (;;) {
		struct sockaddr_in client_addr;
		int client_addr_len = sizeof(client_addr);

		SOCKET client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_addr_len);
		assert(client_socket != INVALID_SOCKET);

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
		int client_port = ntohs(client_addr.sin_port);

		printf("client connected: %s:%d\n", client_ip, client_port);
		CreateThread(NULL, 0, thread_func, (void *)(uintptr_t)client_socket, 0, NULL);
	}
}