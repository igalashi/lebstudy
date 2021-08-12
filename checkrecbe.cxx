//
//
//

#include <iostream>
#include <iomanip>
#include <fstream>

#define uint32_t unsigned int

int decode_tdc(unsigned int *data)
{
	return 0;
}

int reading()
{

	int nread = 0;
#if 0
	while (! std::cin.eof()) {
		uint32_t data;
		std::cin.read(reinterpret_cast<char *>(&data), sizeof(data));
		nread += std::cin.gcount();

		int carry = 0;
		int ch;
		int tval;
		std::cout << std::hex << std::setw(8) << data << " : ";
		if ((data & 0xffffffff) == 0x12345678) {
			std::cout << "Header" << std::endl;
		} else
		if ((data & 0xff0000ff) == 0xff0000aa) {
			std::cout << "Gate Start" << std::endl;
		} else
		if ((data & 0xff0000ff) == 0xff000055) {
			std::cout << "Gate End" << std::endl;
		} else
		if ((data & 0xff000000) == 0xff000000) {
			carry = data & 0xff;
			std::cout << "Carry : " << std::dec << carry << std::endl;
		} else
		if ((data & 0xc0000000) == 0xc0000000) {
			ch = (data >> 24) & 0x1f;
			tval = data & 0x00ffffff;
			std::cout << "Ch: " << std::dec << std::setw(2) << ch
			<< " Data : " << std::setw(8) << tval
			<< " (0x" << std::hex << std::setw(6) << tval << ")"
			<< std::dec << std::endl;
		} else {
			std::cout << "# BAD data : "
				<< std::hex << data
				<< std::dec << std::endl;
		}
	}
#else
	
#endif

	uint32_t data;
	std::cin.read(reinterpret_cast<char *>(&data), sizeof(uint32_t));
	std::cout << "HEAD: " << std::hex << data << std::endl;

	uint32_t prev_data = 0x00000000;
	while (! std::cin.eof()) {
		char *cdata = reinterpret_cast<char *>(&data);
		std::cin.read(reinterpret_cast<char *>(&data), sizeof(uint32_t));
		if (std::cin.eof()) break;
		//if (std::cin.gcount() == 0) break;

		nread += std::cin.gcount();
		if (data == 0xffff5555) {
			std::cout << "Gate end, Nread: " << std::dec << nread
				<< ", GE: " << std::hex <<  data << std::endl;
			time_t rectime;
			std::cin.read(reinterpret_cast<char *>(&rectime), sizeof(time_t));
			std::cout << ctime(&rectime);
		} else if (
			   ((cdata[1] - cdata[0]) != 1)
			&& ((cdata[2] - cdata[1]) != 1)
			&& ((cdata[3] - cdata[2]) != 1)) {
			std::cout << "#E " << std::hex << data << std::endl;
			if (prev_data != 0x00000000) {
				int p = (prev_data >> 24) & 0xff;
				int n = data & 0xff;
				if ((n -p) != 1) {
					std::cout << "#E " << std::hex
						<< prev_data << " " << data
						<< std::endl;
				}
			}
		}
		prev_data = data;
	}

	return 0;
}


int main(int argc, char* argv[])
{

	reading();	
	
	return 0;
}

