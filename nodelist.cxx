/*
 *
 *
 */

#include <iostream>
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

int process_node(std::vector<struct nodeprop> &nodes)
{

	return 0;
}

int process_rbcp(std::vector<struct rbcp_com> &com)
{

	return 0;
}

int nodelist(char *filename, std::vector<struct nodeprop> &nodes)
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

		if (words[0].front() == '#') continue;
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
	nodelist(argv[1], nodes);

	for (auto i : nodes) {
		std::cout << "id: " << i.id
			<< "  host: "  <<i.host
			<< "  port: "  << i.port
			<< "  is_dummy: "  << i.is_dummy <<std::endl;
	}

	return 0;
}
#endif
