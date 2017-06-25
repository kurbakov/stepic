#include "http_server.h"

http_server::http_server(std::string ip, std::string p, std::string dir)
{
	host = ip;
	port = p;
	directory = dir;
}

http_server::~http_server()
{
}

int http_server::initiate_socket(std::string h, std::string p){
	// start socket
	int master_socket = socket(AF_INET, SOCK_STREAM, 0);

	// create sockaddr_in struct
	struct sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(std::stoi(p)); // port is a private varianble of the class
	inet_pton(AF_INET, h.c_str(), &SockAddr);

	// bind
	bind(master_socket, (struct sockaddr *)(&SockAddr), sizeof(SockAddr));

	// make it non block
	set_nonblock(master_socket);

	// start to listen
	listen(master_socket, SOMAXCONN);

	return master_socket;	
}

int http_server::set_nonblock(int fd) {
        int flags;
#if defined(O_NONBLOCK)
        if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
                flags = 0;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
        flags = 1;
        return ioctl(fd, FIOBIO, &flags);
#endif
}

void http_server::handle_requests(int master_socket){
	int epollfd = epoll_create1(0);

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = master_socket;
	
	epoll_ctl(epollfd, EPOLL_CTL_ADD, master_socket, &ev);

	while(1){
		struct epoll_event EVENTS[MAX_EVENTS];
		int N = epoll_wait(epollfd, EVENTS, MAX_EVENTS, -1);

		for(int i=0; i<N; i++){
			if(EVENTS[i].data.fd == master_socket){
				// if master we accept
				int SlaveSocket = accept(master_socket, 0 ,0);
				set_nonblock(SlaveSocket);
				
				struct epoll_event Event;
				Event.events = EPOLLIN;
				Event.data.fd = SlaveSocket;
				
				epoll_ctl(epollfd, EPOLL_CTL_ADD, SlaveSocket, &Event);
			}
			else{
				// read
				static char BUFFER[1024];
				int RecvResult = recv(EVENTS[i].data.fd, BUFFER, 1024, MSG_NOSIGNAL);
				if((RecvResult == 0) && (errno != EAGAIN)){
					shutdown(EVENTS[i].data.fd, SHUT_RDWR);
					close(EVENTS[i].data.fd);
				}
				else if(RecvResult > 0){
					send(EVENTS[i].data.fd, BUFFER, RecvResult, MSG_NOSIGNAL);
				}
			}
		} // foor loop
	} // while loop

	return;
}

void http_server::run()
{
	// create and start to listen
	int my_socket = initiate_socket(host, port);
	handle_requests(my_socket);

}