#include "IOCP.h"

int main()
{
	IOCP listen, h;
	Event sock;
	listen.CreateListenSocket([&listen, &sock](SOCKET &socket, sockaddr_in &addr) {
		sock._socket = socket;
		sock._addr = addr;
		printf("¿Í»§¶î %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		listen.AddSocket(&sock);
	}, 4567);


	while (true)
	{
		Sleep(1000);
	}
	return 0;
}