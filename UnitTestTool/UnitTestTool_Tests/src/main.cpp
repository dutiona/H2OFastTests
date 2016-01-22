#include <iostream>

#include "../../inc/UnitTestTool.h"



class A{
public:

	A();

	static int add(int a, int b) { return a + b; }
	static int minus(int a, int b) { return a - b; }

};

using namespace UnitTestTool;


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


static UnitTestTool::detail::UnitTestTool_registrar UnitTestTool_registrar(
	UnitTestTool::unit_test("Test", {
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

		UnitTestTool::run_and_display(true);

		system("PAUSE");
		//std::cout << "Hello world !" << std::endl;
}