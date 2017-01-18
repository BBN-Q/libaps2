#ifndef VERSION_H
#define VERSION_H

#include <string>
using std::string;

const string LIBAPS2_VERSION = "@GIT_DESCRIBE@";
string get_driver_version() {
	return LIBAPS2_VERSION;
}

#endif
