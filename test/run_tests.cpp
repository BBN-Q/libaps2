#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <iostream>
#include <string>
using std::string;

string ip_addr;

int main( int argc, char* const argv[] )
{

  //Pull out IP address from first argument
  if ( argc < 2) {
    std::cerr << "Error! Need to specifiy an IP address to test." << std::endl;
    return -1;
  } else {
    ip_addr = string(argv[1]);
    memset(argv[1], 0, strlen(argv[1]));
  }

  // Pass onto Catch
  int result = Catch::Session().run( argc, argv );

  // global clean-up...

  return result;
}
