#include <experimental/coroutine>
#include <experimental/generator>
#include <ppl.h>
#include <pplawait.h>

#include <iostream>
#include <cinttypes>
#include <csignal>
#include <Windows.h>


using concurrency::task;
using std::experimental::generator;

template<typename IntType = int>
generator<IntType> fibonacci(int n) {
   IntType i = 0;
   IntType j = 1;

   co_yield i;
   co_yield i + j;

   for (int cmpt = 2; cmpt < n; ++cmpt) {
      const auto next = i + j;
      co_yield next;
      i = j;
      j = next;
   }
}

template<typename IntType = int>
task<void> print_fib(int n) {
   for (const auto f : fibonacci<IntType>(n)) {
      std::cout << f << '\n';
   }
   co_return;
}

int main() {

   print_fib<uint64_t>(90).get();

   return 0;
}