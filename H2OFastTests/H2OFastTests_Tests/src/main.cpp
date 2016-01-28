#include "H2OFastTests_Tests.h"

#include <limits>
#include <iostream>


int main(int /*argc*/, char* /*argv[]*/){
	run_scenario(H2OFastTests_Tests);
	print_result(H2OFastTests_Tests, true);

	std::cout << "Press enter to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}