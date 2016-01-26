#include "H2OFastTests.h"

#include <limits>
#include <iostream>
#include <stdexcept>

using namespace H2OFastTests;


register_scenario(TEST1, "Test 1",

	describe_test_label("fail !", [](){
		Assert::Fail("nooooo", line_info());
	}),
	
	skip_test(describe_test([](){
		Assert::IsTrue(true, "yeah !");
	})),

	describe_test_label("RUNTIMERROR_TEST", [](){
		throw std::runtime_error("fail this with an error man !");
	})
);


register_scenario(TEST2, "youhou", describe_test([](){}));

int main(int /*argc*/, char* /*argv[]*/){
	register_observer(TEST1, H2OFastTests::ConsoleIO_Observer);
	run_scenario(TEST1);
	print_result(TEST1, true);
	std::cout << "Press enter to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}