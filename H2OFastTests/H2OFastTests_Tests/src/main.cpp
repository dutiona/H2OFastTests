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
register_test("Test", [](){
const std::string str = "1 + 2 = 3 !";
})
*/
/*
register_tests("Test", {
make_test([](){
const std::string str = "1 + 2 = 3 !";
Assert::Fail(":'(", LINE_INFO());
}),
make_test([](){
const bool b = true;
Assert::IsTrue(b);
})
}})
*/


static H2OFastTests::detail::UnitTestTool_registrar UnitTestTool_registrar(
	H2OFastTests::unit_test("Test", {
		make_test("hahah", [](){
			const std::string str = "1 + 2 = 3 !";
			Assert::Fail(":'(", LINE_INFO());
		}),
			make_test("hohoho", [](){
			const bool b = true;
			Assert::IsTrue(b);
		})
	}
));


int main(int /*argc*/, char* /*argv[]*/){

	H2OFastTests::run_and_display(true);

	system("PAUSE");
	//std::cout << "Hello world !" << std::endl;
}