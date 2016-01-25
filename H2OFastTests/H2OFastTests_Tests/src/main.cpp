#include <iostream>

#include "../../inc/H2OFastTests.h"



class A{
public:

	A();

	static int add(int a, int b) { return a + b; }
	static int minus(int a, int b) { return a - b; }

};

using namespace H2OFastTests;




/*
register_tests(TEST1, "Test 1", [](){
	Assert::Fail("nooooo", LINE_INFO());
});
*/

int main(int /*argc*/, char* /*argv[]*/){

	/*
	run_tests(TEST1);
	print_result(TEST1, true);
	*/

	system("PAUSE");
	//std::cout << "Hello world !" << std::endl;
}