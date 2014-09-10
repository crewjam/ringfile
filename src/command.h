#ifndef COMMAND_H_
#define COMMAND_H_

#include <iostream>

class Switches {
 public:
  enum { kModeUnspecified=0, kModeRead='r', kModeStat='S', kModeAppend='a' };

	Switches();
	bool Parse(int argc, char ** argv);
	
	std::ostream * error_stream;
	int mode;
	int verbose;
	long size;
	std::string path;
  std::string program;
};

int Main(int argc, char **argv);

#endif  // COMMAND_H_