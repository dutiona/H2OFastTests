#pragma once

#ifndef UNITTESTTOOL_H
#define UNITTESTTOOL_H

namespace UnitTestTool{

	class Assert
	{
	public:

#if 0
		// Verify that two objects are equal.
		template<typename T> static void AreEqual(const T& expected, const T& actual, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(expected == actual, EQUALS_MESSAGE(expected, actual, message), pLineInfo);
		}


		// double equality comparison:
		static void AreEqual(double expected, double actual, double tolerance, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			double diff = expected - actual;
			FailOnCondition(fabs(diff) <= fabs(tolerance), EQUALS_MESSAGE(expected, actual, message), pLineInfo);
		}

		// float equality comparison:
		static void AreEqual(float expected, float actual, float tolerance, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			float diff = expected - actual;
			FailOnCondition(fabs(diff) <= fabs(tolerance), EQUALS_MESSAGE(expected, actual, message), pLineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const char* expected, const char* actual, const char* message, const __LineInfo* pLineInfo = NULL)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, ToString(message).c_str(), pLineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const char* expected, const char* actual, const wchar_t* message, const __LineInfo* pLineInfo = NULL)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, message, pLineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const char* expected, const char* actual, bool ignoreCase = false, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(CppUnitStrCmpA(expected, actual, ignoreCase), EQUALS_MESSAGE(expected, actual, message), pLineInfo);
		}


		// wchar_t* string equality comparison:
		static void AreEqual(const wchar_t* expected, const wchar_t* actual, const char* message, const __LineInfo* pLineInfo = NULL)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, ToString(message).c_str(), pLineInfo);
		}

		// wchar_t* string equality comparison:
		static void AreEqual(const wchar_t* expected, const wchar_t* actual, const wchar_t* message, const __LineInfo* pLineInfo = NULL)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, message, pLineInfo);
		}

		// wchar_t* string equality comparison:
		static void AreEqual(const wchar_t* expected, const wchar_t* actual, bool ignoreCase = false, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(CppUnitStrCmpW(reinterpret_cast<const unsigned short*>(expected), reinterpret_cast<const unsigned short*>(actual), ignoreCase), EQUALS_MESSAGE(expected, actual, message), pLineInfo);
		}

		// Verify that two references refer to the same object instance (identity):
		template<typename T> static void AreSame(const T& expected, const T& actual, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(&expected == &actual, EQUALS_MESSAGE(expected, actual, message), pLineInfo);
		}

		// Generic AreNotEqual comparison:
		template<typename T> static void AreNotEqual(const T& notExpected, const T& actual, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(!(notExpected == actual), NOT_EQUALS_MESSAGE(notExpected, actual, message), pLineInfo);
		}

		// double AreNotEqual comparison:
		static void AreNotEqual(double notExpected, double actual, double tolerance, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			double diff = notExpected - actual;
			FailOnCondition(fabs(diff) > fabs(tolerance), NOT_EQUALS_MESSAGE(notExpected, actual, message), pLineInfo);
		}

		// float AreNotEqual comparison:
		static void AreNotEqual(float notExpected, float actual, float tolerance, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			float diff = notExpected - actual;
			FailOnCondition(fabs(diff) > fabs(tolerance), NOT_EQUALS_MESSAGE(notExpected, actual, message), pLineInfo);
		}

		// char* string AreNotEqual comparison with char* message:
		static void AreNotEqual(const char* notExpected, const char* actual, const char* message, const __LineInfo* pLineInfo = NULL)
		{
			AreNotEqual(notExpected, actual, DEFAULT_IGNORECASE, ToString(message).c_str(), pLineInfo);
		}

		// char* string AreNotEqual comparison with wchar_t*  message:
		static void AreNotEqual(const char* notExpected, const char* actual, const wchar_t* message, const __LineInfo* pLineInfo = NULL)
		{
			AreNotEqual(notExpected, actual, DEFAULT_IGNORECASE, message, pLineInfo);
		}

		// char* string AreNotEqual comparison:
		static void AreNotEqual(const char* notExpected, const char* actual, bool ignoreCase = false, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(!CppUnitStrCmpA(notExpected, actual, ignoreCase), NOT_EQUALS_MESSAGE(notExpected, actual, message), pLineInfo);
		}

		// wchar_t* string AreNotEqual comparison with char* message:
		static void AreNotEqual(const wchar_t* notExpected, const wchar_t* actual, const char* message, const __LineInfo* pLineInfo = NULL)
		{
			AreNotEqual(notExpected, actual, DEFAULT_IGNORECASE, ToString(message).c_str(), pLineInfo);
		}

		// wchar_t* string AreNotEqual comparison with wchar_t* message:
		static void AreNotEqual(const wchar_t* notExpected, const wchar_t* actual, const wchar_t* message, const __LineInfo* pLineInfo = NULL)
		{
			AreNotEqual(notExpected, actual, DEFAULT_IGNORECASE, message, pLineInfo);
		}

		// wchar_t* string AreNotEqual comparison:
		static void AreNotEqual(const wchar_t* notExpected, const wchar_t* actual, bool ignoreCase = false, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(!CppUnitStrCmpW(reinterpret_cast<const unsigned short*>(notExpected), reinterpret_cast<const unsigned short*>(actual), ignoreCase), NOT_EQUALS_MESSAGE(notExpected, actual, message), pLineInfo);
		}

		// Verify that two references do not refer to the same object instance (identity):
		template<typename T> static void AreNotSame(const T& notExpected, const T& actual, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(!(&notExpected == &actual), NOT_EQUALS_MESSAGE(notExpected, actual, message), pLineInfo);
		}

		// Verify that a pointer is NULL:
		template<typename T> static void IsNull(const T* actual, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(actual == NULL, reinterpret_cast<const unsigned short*>(message), pLineInfo);
		}

		// Verify that a pointer is not NULL:
		template<typename T> static void IsNotNull(const T* actual, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(!(actual == NULL), reinterpret_cast<const unsigned short*>(message), pLineInfo);
		}

		// Verify that a condition is true:
		static void IsTrue(bool condition, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(condition, reinterpret_cast<const unsigned short*>(message), pLineInfo);
		}

		// Verify that a conditon is false:
		static void IsFalse(bool condition, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(!condition, reinterpret_cast<const unsigned short*>(message), pLineInfo);
		}

		// Force the test case result to be Failed:
		static void Fail(const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailImpl(reinterpret_cast<const unsigned short*>(message), pLineInfo);
		}

#ifdef  __cplusplus_winrt


		// General equal comparison for WinRT pointers.
		template<typename T> static void AreEqual(T^ expected, T^ actual, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			bool condition = false;
			if (expected != nullptr && actual != nullptr)
			{
				condition = expected->Equals(actual);
			}
			else if (expected == nullptr && actual == nullptr)
			{
				condition = true;
			}

			FailOnCondition(condition, EQUALS_MESSAGE_WINRT(expected, actual, message), pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreEqual(Platform::String^ expected, Platform::String^ actual, char* message, const __LineInfo* pLineInfo = nullptr)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, ref new Platform::String(ToString(message).c_str()), pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreEqual(Platform::String^ expected, Platform::String^ actual, wchar_t* message, const __LineInfo* pLineInfo = nullptr)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, ref new Platform::String(message), pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreEqual(Platform::String^ expected, Platform::String^ actual, Platform::String^ message, const __LineInfo* pLineInfo = nullptr)
		{
			AreEqual(expected, actual, DEFAULT_IGNORECASE, message, pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreEqual(Platform::String^ expected, Platform::String^ actual, bool ignoreCase = false, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			FailOnCondition(CppUnitStrCmpW(UnboxString(expected), UnboxString(actual), ignoreCase), EQUALS_MESSAGE_WINRT(expected, actual, message), pLineInfo);
		}

		// Used to check if two WinRT references are referecing the same object.
		// Be noted not to use this one to check if 2 objects are equals.
		template<typename T> static void AreSame(T% expected, T% actual, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			FailOnCondition(%expected == %actual, EQUALS_MESSAGE_WINRT(%expected, %actual, message), pLineInfo);
		}


		// General AreNotEqual comparison for WinRT pointers.
		template<typename T> static void AreNotEqual(T^ notExpected, T^ actual, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			bool condition = true;
			if (notExpected != nullptr && actual != nullptr)
			{
				condition = !notExpected->Equals(actual);
			}
			else if (notExpected == nullptr && actual == nullptr)
			{
				condition = false;
			}

			FailOnCondition(condition, NOT_EQUALS_MESSAGE_WINRT(notExpected, actual, message), pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreNotEqual(Platform::String^ expected, Platform::String^ actual, char* message, const __LineInfo* pLineInfo = nullptr)
		{
			AreNotEqual(expected, actual, DEFAULT_IGNORECASE, ref new Platform::String(ToString(message).c_str()), pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreNotEqual(Platform::String^ expected, Platform::String^ actual, wchar_t* message, const __LineInfo* pLineInfo = nullptr)
		{
			AreNotEqual(expected, actual, DEFAULT_IGNORECASE, ref new Platform::String(message), pLineInfo);
		}

		// Platform::String^ string equality comparison.
		static void AreNotEqual(Platform::String^ expected, Platform::String^ actual, Platform::String^ message, const __LineInfo* pLineInfo = nullptr)
		{
			AreNotEqual(expected, actual, DEFAULT_IGNORECASE, message, pLineInfo);
		}

		// Platform::String^ string AreNotEqual comparison:
		static void AreNotEqual(Platform::String^ notExpected, Platform::String^ actual, bool ignoreCase = false, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			FailOnCondition(!CppUnitStrCmpW(UnboxString(notExpected), UnboxString(actual), ignoreCase), NOT_EQUALS_MESSAGE_WINRT(notExpected, actual, message), pLineInfo);
		}

		// Used to check if two WinRT references are not referencing the same object.
		// Be noted not to use it to do AreNotEqual comparsion for 2 objects.
		template<typename T> static void AreNotSame(T% notExpected, T% actual, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			FailOnCondition(!(%notExpected == %actual), NOT_EQUALS_MESSAGE_WINRT(%notExpected, %actual, message), pLineInfo);
		}

		// Check if a WinRT pointer is nullptr.
		template<typename T> static void IsNull(T^ actual, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			FailOnCondition(actual == nullptr, UnboxString(message), pLineInfo);
		}

		// Check if a WinRT pointer is not nullptr
		template<typename T> static void IsNotNull(T^ actual, Platform::String^ message = nullptr, const __LineInfo* pLineInfo = nullptr)
		{
			FailOnCondition(!(actual == nullptr), UnboxString(message), pLineInfo);
		}

#endif


#ifdef _CPPUNWIND
		// Verify that a function raises an exception:
		template<typename _EXPECTEDEXCEPTION, typename _FUNCTOR> static void ExpectException(_FUNCTOR functor, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			try
			{
				functor();
			}
			catch (_EXPECTEDEXCEPTION)
			{
				return;
			}
			catch (...)
			{
				Internal_SetExpectedExceptionMessage(reinterpret_cast<const unsigned short *>(message));
				throw;
			}

			Assert::Fail(message, pLineInfo);
		}

		template<typename _EXPECTEDEXCEPTION, typename _RETURNTYPE> static void ExpectException(_RETURNTYPE(*func)(), const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
		{
			FailOnCondition(func != NULL, reinterpret_cast<const unsigned short*>(message), pLineInfo);
			try
			{
				func();
			}
			catch (_EXPECTEDEXCEPTION)
			{
				return;
			}
			catch (...)
			{
				Internal_SetExpectedExceptionMessage(reinterpret_cast<const unsigned short *>(message));
				throw;
			}

			Assert::Fail(message, pLineInfo);
		}
#endif

#ifndef __INTELLISENSE__ 
	public:
		__declspec(dllexport) static unsigned short* __stdcall Internal_GetExpectedExceptionMessage();
		__declspec(dllexport) static void            __stdcall Internal_SetExpectedExceptionMessage(const unsigned short *message);
#endif

	private:
		__declspec(dllexport) static void __stdcall FailOnCondition(bool condition, const unsigned short* message, const __LineInfo* pLineInfo);
		__declspec(dllexport) static bool __stdcall CppUnitStrCmpA(const char* str1, const char* str2, bool ignoreCase);
		__declspec(dllexport) static bool __stdcall CppUnitStrCmpW(const unsigned short* str1, const unsigned short* str2, bool ignoreCase);
		__declspec(dllexport) static void __stdcall FailImpl(const unsigned short* message, const __LineInfo* pLineInfo);
		__declspec(dllexport) static void __stdcall GetAssertMessage(bool equality, const unsigned short *expected, const unsigned short *actual, const unsigned short *userMessage, unsigned short* assertMessageBuffer, size_t bufferSize);

		// return a formated message for equality based asserts.
		static std::wstring GetAssertMessage(bool equality, const std::wstring& expected, const std::wstring& actual, const wchar_t *message)
		{
			wchar_t assertMessage[MS_CPP_UNITTESTFRAMEWORK_MAX_BUF_LENGTH];
			memset(assertMessage, 0, MS_CPP_UNITTESTFRAMEWORK_MAX_BUF_LENGTH * sizeof(wchar_t));
			const unsigned short* expectedPtr = reinterpret_cast<const unsigned short *>(expected.c_str());
			const unsigned short* actualPtr = reinterpret_cast<const unsigned short *>(actual.c_str());
			const unsigned short* messagePtr = reinterpret_cast<const unsigned short *>(message);
			unsigned short *assertMessagePtr = reinterpret_cast<unsigned short *>(assertMessage);

			GetAssertMessage(equality, expectedPtr, actualPtr, messagePtr, assertMessagePtr, MS_CPP_UNITTESTFRAMEWORK_MAX_BUF_LENGTH);
			return std::wstring(assertMessage);
		}


#ifdef  __cplusplus_winrt
	private:
		static const unsigned short* UnboxString(Platform::String^ str)
		{
			if (str == nullptr)
				return nullptr;

			return reinterpret_cast<const unsigned short*>(str->Data());
		}

		template<typename T> static std::wstring ToWideString(T^ value)
		{
			if (nullptr == value)
			{
				return nullptr;
			}
			return std::wstring(value->ToString()->Data());
		}

#endif
#endif

	};

}

#endif