#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <random>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <fstream>
#include <future>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <cstring>
#include <iterator>

#define PORT 8082 // telnet 127.0.0.1 8082

using namespace std;

string column = "id"; //Sort column
unsigned long int start_row = 0; // Start row number to send over socket

struct tbl_data {
	string s1;
	string s2;
}; //Struct for data

map <unsigned long int, tbl_data> m_tbl; //Table with data

//Get random string
string gen_random(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

//Predicate to sort
bool c_by_col(pair<unsigned long int,tbl_data> a, pair<unsigned long int,tbl_data> b) {
	bool c = false;
	if (column == "id") {
		c = a.first < b.first;
	} else if (column == "s1") {
		c = a.second.s1 < b.second.s1;
	} else if (column == "s2") {
		c = a.second.s2 < b.second.s2;
	}
	return c;
}

//Get body to html answer
stringstream create_body (vector<pair<unsigned long int,tbl_data>> v, unsigned long int max_row) {
	stringstream table_b; // string with table data, json-like format
	stringstream general_b; //string with general info; Table_size, Table_sort_by, Max_row, End_row
	stringstream body; //returning string
	unsigned long int end_row; //Number of last row in this answer
	unsigned int wr_body = 0; // When table_b can be filling up
	if (v.begin()->first >= start_row and column == "id") {
		wr_body = 1;
	}
	unsigned long int i = 1;
	for (auto x : v) {
		if (x.first == start_row) {
			wr_body = 1;
		}
		if (wr_body == 1) {
			table_b << "{\n"
				 << "\"ID\":" << x.first << ",\n"
				 << "\"Column1\":\"" << x.second.s1 << "\",\n"
				 << "\"Column2\":\"" << x.second.s2 << "\"\n"
				 << "}";
			if (i < v.size() and i < max_row ) {
				table_b << ",\n";
			} else {
				table_b << "\n";
			}
			if (i == max_row or i == v.size()) {
				end_row = x.first;
				start_row = x.first;
				break;
			}
			i++;
		}
	}
	if (wr_body == 1) {
		table_b << "]}\n";

		general_b << "{\"Table_size\":" << v.size() << "},\n"
				 << "{\"Table_sort_by\":\"" << column << "\"},\n"
				 << "{\"Max_row\":" << max_row << "},\n"
				 << "{\"End_row\":" << end_row << "},\n"
				 << "{\"Table\": [\n";
		body << general_b.str() << table_b.str();
	}
	return body;
}

//Get new row for table
tbl_data add_row () {
	tbl_data ts;
	ts.s1 = gen_random(1 + rand()/((RAND_MAX + 1u) / 12));
	ts.s2 = gen_random(1 + rand()/((RAND_MAX + 1u) / 6));
	return ts;
}

//Delete row from table
bool del_row (unsigned long int id) {
	m_tbl.erase(id);
	return 0;
}

//Change row at table
void ch_row (unsigned long int id) {
	m_tbl[id] = add_row();
}

int main() {
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	const int max_client_buffer_size = 1024;
	char buf[max_client_buffer_size];

	tbl_data ts;
	// Initialize Table
	for (unsigned long int i = 0;i < 2000000;++i) {
		m_tbl[i] = add_row();
	}
	//emulation of table changes
	//TODO: periodically delete row = start_row
	int count = 3;
	auto future1 = async(launch::async, [&count] {
		unsigned long int i = 0;
		for(;;) {
			time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
			srand(t);
			ch_row(rand()%10);
			if (i%3){del_row(rand() % 6);};
		    this_thread::sleep_for(chrono::milliseconds(1000));
		    i++;
		}
	});

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,	&opt, sizeof(opt)))	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	// Forcefully attaching socket to the port
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, SOMAXCONN) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	//Wait for connect
	for (;;) {
		new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen);
		if (new_socket < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		stringstream response;
		stringstream response_body;

		ssize_t result = recv(new_socket, buf, max_client_buffer_size, 0);

		if (result < 0) {
		    cerr << "recv failed: " << result << "\n";
		    close(new_socket);
		} else if (result == 0) {
		    cerr << "connection closed...\n";
		} else if (result > 0) {
			//Receive command from socket
			buf[result] = '\0';
		    stringstream ss;
		    ss << buf;
		    //Maximum returning row, default = 100
		    unsigned long int max_row = 3;

		    stringstream s_max_row;
		    stringstream s_start_row;
		    //Split command string
		    string recv_str = ss.str();
		    string token;
		    size_t pos = 0;
		    while ((pos = recv_str.find(";")) != string::npos) {
		        token = recv_str.substr(0, pos);
		        recv_str.erase(0, pos + 1);
		        if (token.find("max_row") != string::npos) {
		        	try {
		        		//get max_row
		        		//TODO: to unsigned long int
		        		max_row = stoi(token.substr(token.find("=")+1));
		        	} catch (exception &err){
		        		cout << "Error: max_row must be 'unsigned long int'\n";
		        	}
		        }else if (token.find("start_row") != string::npos) {
		        	try {
		        		//get start_row
		        		//TODO: to unsigned long int
		        		start_row = stoi(token.substr(token.find("=")+1));
		        	} catch (exception &err){
		        		cout << "Error: start_row must be 'unsigned long int'\n";
		        	}
		        }
		    }
			//Command get - get N record, sort by previous sort command, default sort = row number
			// N = max_row
			// get from Row with id = start_row
		    if (ss.str().find("get") != string::npos) {
		    	vector<pair<unsigned long int,tbl_data>> v(m_tbl.cbegin(),m_tbl.cend());
		    	sort(v.begin(),v.end(),c_by_col);
		    	response_body = create_body(v, max_row);
			//Command sort_id - get N record, sort by 'id' column, from start_row
			// N = max_row
		    } else if (ss.str().find("sort_id") != string::npos) {
		    	column = "id";
		    	vector<pair<unsigned long int,tbl_data>> v(m_tbl.cbegin(),m_tbl.cend());
		    	sort(v.begin(),v.end(),c_by_col);
		    	response_body = create_body(v, max_row);
			//Command sort_s1 - get N record, sort by 's1' column, from start_row
			// N = max_row
		    } else if (ss.str().find("sort_s1") != string::npos) {
		    	column = "s1";
		    	vector<pair<unsigned long int,tbl_data>> v(m_tbl.cbegin(),m_tbl.cend());
		    	sort(v.begin(),v.end(),c_by_col);
		    	response_body = create_body(v, max_row);
			//Command sort_s2 - get N record, sort by 's2' column, from start_row
			// N = max_row
		    } else if (ss.str().find("sort_s2") != string::npos) {
		    	column = "s2";
		    	vector<pair<unsigned long int,tbl_data>> v(m_tbl.cbegin(),m_tbl.cend());
		    	sort(v.begin(),v.end(),c_by_col);
		    	response_body = create_body(v, max_row);
		    } else {
		    	cout << "Info: Unknown command'. TODO responce\n";
		    }
			// TODO: Code != 200 OK
		    response << "HTTP/1.1 200 OK\n"
		             << "Version: HTTP/1.1\n"
		             << "Content-Type: application/json\n"
		             << "Content-Length: " << response_body.str().length()
		             << "\n\n"
		             << response_body.str();
			// Send data over socket
		    result = send(new_socket, response.str().c_str(),response.str().length(), 0);

		    if (result == 0) {
		        cerr << "send failed: " << endl;
		    }
		    close(new_socket);
		}
	}

	close(server_fd);
	return 0;
}
