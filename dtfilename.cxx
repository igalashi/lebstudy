/*
 *
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

char *dtfilename(const char *name)
{
	static char fname[128];
	
	time_t now = time(NULL);
	struct tm *lt = localtime(&now);

	std::ostringstream ossdaytime;
	ossdaytime << std::setfill('0')
		<< std::setw(2) << lt->tm_mon + 1
		<< std::setw(2) << lt->tm_mday
		<< std::setw(2) << lt->tm_hour
		<< std::setw(2) << lt->tm_min
		<< std::setw(2) << lt->tm_sec;

	std::ostringstream oss;
	oss << name
		//<< std::setfill('0') << std::setw(4) << id << std::setfill(' ') 
		<< "_"
		<< (lt->tm_year + 1900) << ossdaytime.str();

	int fileno = 0;
	std::string fnamestr = oss.str();
	while (true) {
		//struct stat finfo;
		//if (stat(oss.str().c_str(), &finfo)) break;
		std::ifstream infile((fnamestr + ".dat").c_str());
		if (! infile.good()) {
			break;
		} else {
			std::ostringstream ioss;
			ioss << ++fileno;
			fnamestr = oss.str() + "_" + ioss.str();
		}
	}

	strncpy(fname, (fnamestr + ".dat").c_str(), 128);
	
	return fname;
}
