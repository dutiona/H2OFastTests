#include <experimental/coroutine>
#include <experimental/generator>
#include <ppl.h>
#include <pplawait.h>

#include <iostream>
#include <cinttypes>
#include <csignal>
#include <csetjmp>
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


std::jmp_buf jump_buffer;


void signal_cb(int sig_num) {
   switch (sig_num) {
   case SIGINT: {
      std::cout << "SIGINT" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   case SIGILL: {
      std::cout << "SIGILL" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   case SIGFPE: {
      std::cout << "SIGFPE" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   case SIGSEGV: {
      std::cout << "SIGSEGV" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   case SIGTERM: {
      std::cout << "SIGTERM" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   case SIGBREAK: {
      std::cout << "SIGBREAK" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   case SIGABRT: {
      std::cout << "SIGABRT" << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }
   default: {
      std::cout << "default " << sig_num << std::endl;
      std::longjmp(jump_buffer, sig_num);
      break;
   }

   }
}

int main() {
   //print_fib<uint64_t>(90).get();

   signal(SIGINT, signal_cb);
   signal(SIGILL, signal_cb);
   signal(SIGFPE, signal_cb);
   signal(SIGSEGV, signal_cb);
   signal(SIGTERM, signal_cb);
   signal(SIGBREAK, signal_cb);
   signal(SIGABRT, signal_cb);
   volatile int sig_num = 0;

   if ((sig_num = std::setjmp(jump_buffer)) != 0) {
      std::cout << "CRASH" << std::endl;
      std::cout << sig_num << std::endl;
      return 1;
   }

   
   raise(SIGSEGV);

   return 0;
}