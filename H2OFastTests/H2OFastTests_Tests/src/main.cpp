#include "H2OFastTests_Tests.h"

#include <limits>
#include <iostream>


int main(int /*argc*/, char* /*argv[]*/){
	auto observer = std::make_shared<H2OFastTests::ConsoleIO_Observer>();
	register_observer(H2OFastTests_Tests, H2OFastTests::ConsoleIO_Observer, observer);
	run_scenario(H2OFastTests_Tests);
	print_result(H2OFastTests_Tests, true);
	std::cout << "Press enter to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}