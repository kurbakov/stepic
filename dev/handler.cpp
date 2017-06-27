#include <unistd.h>
#include <stdio.h>

class handler{
public:
	handler();
	void reply(int, std::string);

private:
	std::string get_file_name(std::string);
	std::string read_file(FILE*);
	std::string build_reply(std::string, std::string);

};

handler::handler()
{
}

std::string handler::build_reply(std::string header, std::string data)
{
	std::string reply;

	reply.append(header);
	reply.append("\r\n");
	reply.append("Content-length: ");
	reply.append(std::to_string(data.size()));
	reply.append("\r\n");
	reply.append("Content-Type: text/html");
	reply.append("\r\n\r\n");
	reply.append(data);

	return reply;
}

std::string handler::read_file(FILE* file_in)
{
	std::fseek(file_in, 0, SEEK_END);
	long fsize = std::ftell(file_in);
	std::fseek(file_in, 0, SEEK_SET); 

	char *file_data = (char*)std::malloc(fsize + 1);
	std::fread(file_data, fsize, 1, file_in);

	file_data[fsize] = 0;

	std::string res = file_data;
	return res;
}

std::string handler::get_file_name(std::string request)
{
	std::string path_file;
	int start=request.find("/");
	int end = start;

	for(unsigned long i=start; i<request.length(); i++){
		if(request[i] == ' ' || request[i] == '?'){
			end = i;
			break;
		}
	}
	path_file = request.substr(start, end-start);
	return path_file;
}

void handler::reply(int file_descriptor, std::string dir)
{
	static char BUFFER[1024];

	int RecvResult = recv(file_descriptor, BUFFER, 1024, MSG_NOSIGNAL);
	
	if((RecvResult == 0) && (errno != EAGAIN))
	{
		shutdown(file_descriptor, SHUT_RDWR);
		close(file_descriptor);
	}
	else if(RecvResult > 0)
	{
		std::string file_path = get_file_name(std::string(BUFFER));
		file_path = dir+file_path;

		FILE* file_in = std::fopen(file_path.c_str(), "r");
		std::string data;
		std::string reply;

		std::cout << "request: " << BUFFER << "\n";
		std::cout << "file: " << file_path << "\n";

		if(file_in){
			data = read_file(file_in);
			reply = build_reply("HTTP/1.0 200 OK", data);
		}
		else{
			reply = build_reply("HTTP/1.0 404 NOT FOUND", data);
		}

		std::cout << "reply: "<< "\n";
		std::cout << reply << "\n";

		send(file_descriptor, reply.c_str(), reply.length(), MSG_NOSIGNAL);
	}
	return;
}
