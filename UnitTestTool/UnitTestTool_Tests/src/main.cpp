#include <iostream>

#include "../../inc/UnitTestTool.h"



class A{
public:

	A();

	static int add(int a, int b) { return a + b; }
	static int minus(int a, int b) { return a - b; }

};

using namespace UnitTestTool;
TEST_CASE(Test_class_A {
		[](){
			Assert::AreEqual(A::add(1, 2), 3, "1+2 != 3 !!!!" LINE_INFO());
		}
	})



int main(int /*argc*/, char* /*argv[]*/){

	//std::cout << "Hello world !" << std::endl;
}