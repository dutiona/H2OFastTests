#include <iostream>
#include <stdexcept>
#include "../../inc/H2OFastTests.h"


using namespace H2OFastTests;


register_scenario(TEST1, "Test 1",

	describe_test_label("fail !", [](){
		Assert::Fail("nooooo", line_info());
	}),
	
	describe_test([](){
		Assert::IsTrue(true, "yeah !");
	}),

	describe_test_label("RUNTIMERROR_TEST", [](){
		throw std::runtime_error("fail this with an error man !");
	})
);


int main(int /*argc*/, char* /*argv[]*/){

	
	run_scenario(TEST1);
	print_result(TEST1, true);
	
	system("PAUSE");
	//std::cout << "Hello world !" << std::endl;
}