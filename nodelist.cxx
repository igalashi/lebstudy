/*
 *
 *
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

const char *SEPARATOR = " ";

struct nodeprop {
	int id;
	std::string host;
	int port;
	bool is_dummy;
};

struct rbcp_com {
	std::string host;
	unsigned int port;
	unsigned int address;
	std::vector<char> data;
};

bool isNumber(const std::string &s)
{
	bool val;
	if (s.find_first_not_of(".0123456789") == std::string::npos
		&& s.front() != '.'
		&& s.back() != '.') {
		val = true;
	} else {
		val = false;
	}

	return val;
}

bool isHexNumber(const std::string &s)
{
	bool val;
	if (s.find_first_not_of("0123456789abcdefABCDEF")
		== std::string::npos) {
		val = true;
	} else {
		val = false;
	}

	return val;
}


int process_node(std::vector<struct nodeprop> &nodes)
{

	return 0;
}

int process_rbcp(std::vector <std::string> &words, std::vector<struct rbcp_com> &com)
{
	static std::string host("bc");
	static unsigned int port(4660);

	#if 0
	std::cout << "#DR ";
	for (auto &i : words) std::cout << " " << i;
	std::cout << std::endl;
	#endif

	if ((words[0] == "rbcp") || (words[0] == "RBCP")) {
		if ((words[1] == "host") && (words.size() > 2)) {
			host = words[2];
		}
		if ((words[1] == "port") && (words.size() > 2)) {
			port = std::stoi(words[2], nullptr, 10);
		}
		if (words.size() > 3) {
			if (isHexNumber(words[1]) && isHexNumber(words[2])) {
				struct rbcp_com lcom;
					lcom.host = host;
					lcom.port = port;

					lcom.address = std::stoi(words[1], nullptr, 16);
					for (unsigned int i = 2 ; i < words.size() ; i++) {
						unsigned int val = std::stoi(words[i], nullptr, 16);
						char cval = static_cast<char>(val & 0xff);
						lcom.data.push_back(cval);
					}
					com.push_back(lcom);
			} else {
				std::cerr << "wrong line :";
				for (unsigned int i = 0 ; i < words.size() ; i++) {
					std::cerr << " " << words[i];
				}
				std::cerr << std::endl;
			}
		}
	

	} else {
		return -1;
	}

	return 0;
}

int nodelist(char *filename,
	std::vector<struct nodeprop> &nodes,
	std::vector<struct rbcp_com> &rcoms)
{
	std::ifstream ifs(filename);

	if (! ifs.is_open()) {
		std::cerr << filename << " : file open error" <<std::endl;
		return -1;
	}

	std::string line;
	while (getline(ifs, line)) {

		std::vector <std::string> words;
		words.clear();
		words.resize(0);
		int offset = 0;
		while (true) {
			std::string::size_type pos = line.find(SEPARATOR, offset);
			if (pos == std::string::npos) {
				if (line.substr(offset) != "") {
					words.push_back(line.substr(offset));
				}
				break;
			}
			std::string oneword(line.substr(offset, pos - offset));
			if (oneword != "") {
				words.push_back(oneword);
			}
			offset = pos + 1;
		}

		if (words.size() < 1) continue;

		#if 0
		std::cout << "#D " << words.size() << " ; " << line << std::endl;
		#endif

		if (words[0].front() == '#') continue;
		if ((words[0] == "rbcp") || (words[0] == "RBCP")) {
			process_rbcp(words, rcoms);
			continue;
		}
		if (words.size() >= 3) {
			struct nodeprop nprop;
			nprop.is_dummy = false;

			if (isNumber(words[0])) {
				std::istringstream iss(words[0]);
				iss >> nprop.id;
			} else {
				std::cerr << "Err. : " << words[0]
				<< " in " << line << std::endl;
				continue;
			}

			nprop.host = words[1];

			if (isNumber(words[2])) {
				std::istringstream iss(words[2]);
				iss >> nprop.port;
			} else {
				std::cerr << "Err. : " << words[2]
				<< " in " << line << std::endl;
				continue;
			}

			if (words.size() > 3) {
				if ((words[3] == "dummy") 
					|| (words[3] == "DUMMY")) {
					nprop.is_dummy = true;
				}
			}

			#if 0
			std::cout << "id: " << nprop.id
				<< " host: "  << nprop.host
				<< " port: "  << nprop.port <<std::endl;
			#endif
			nodes.push_back(nprop);

		} else {
			std::cerr << "Err. : " << line << std::endl;
			continue;
		}

	}


	ifs.close();

	return 0;
}

#ifdef TEST_MAIN
int main(int argc, char* argv[])
{

	std::vector<struct nodeprop> nodes;
	std::vector<struct rbcp_com> rcommands;
	nodelist(argv[1], nodes, rcommands);

	for (auto i : nodes) {
		std::cout << "id: " << std::dec << i.id
			<< "  host: "  <<i.host
			<< "  port: "  << i.port
			<< "  is_dummy: "  << i.is_dummy <<std::endl;
	}

	for (auto i : rcommands) {
		std::cout << "host: " << i.host
			<< " port: " << std::dec << i.port
			<< " address: 0x" << std::hex << std::setfill('0') << std::setw(4)
			<< i.address
			<< " :";
		for (auto j : i.data) {
			std::cout << " " << std::hex << std::setfill('0') << std::setw(2)
				<< (static_cast<unsigned int>(j) & 0xff);
		}
		std::cout << std::endl;
	}

	return 0;
}
#endif
