/*
 *
 *  (C) Copyright 2016 Micha�l Roynard
 *
 *  Distributed under the MIT License, Version 1.0. (See accompanying
 *  file LICENSE or copy at https://opensource.org/licenses/MIT)
 *
 *  See https://github.com/dutiona/H2OFastTests for documentation.
 */

#pragma once

#ifndef H2OFASTTESTS_CONFIG_H
#define H2OFASTTESTS_CONFIG_H

#include <iostream>
#include <string>

// Determines the platform on which Google Test is compiled.
#ifdef __CYGWIN__
# define H2OFT_OS_CYGWIN 1
#elif defined __SYMBIAN32__
# define H2OFT_OS_SYMBIAN 1
#elif defined _WIN32
# define H2OFT_OS_WINDOWS 1
# ifdef _WIN32_WCE
#  define H2OFT_OS_WINDOWS_MOBILE 1
# elif defined(__MINGW__) || defined(__MINGW32__)
#  define H2OFT_OS_WINDOWS_MINGW 1
# elif defined(WINAPI_FAMILY)
#  include <winapifamily.h>
#  if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define H2OFT_OS_WINDOWS_DESKTOP 1
#  elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#   define H2OFT_OS_WINDOWS_PHONE 1
#  elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#   define H2OFT_OS_WINDOWS_RT 1
#  else
// WINAPI_FAMILY defined but no known partition matched.
// Default to desktop.
#   define H2OFT_OS_WINDOWS_DESKTOP 1
#  endif
# else
#  define H2OFT_OS_WINDOWS_DESKTOP 1
# endif  // _WIN32_WCE
#elif defined __APPLE__
# define H2OFT_OS_MAC 1
# if TARGET_OS_IPHONE
#  define H2OFT_OS_IOS 1
# endif
#elif defined __FreeBSD__
# define H2OFT_OS_FREEBSD 1
#elif defined __linux__
# define H2OFT_OS_LINUX 1
# if defined __ANDROID__
#  define H2OFT_OS_LINUX_ANDROID 1
# endif
#elif defined __MVS__
# define H2OFT_OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
# define H2OFT_OS_SOLARIS 1
#elif defined(_AIX)
# define H2OFT_OS_AIX 1
#elif defined(__hpux)
# define H2OFT_OS_HPUX 1
#elif defined __native_client__
# define H2OFT_OS_NACL 1
#elif defined __OpenBSD__
# define H2OFT_OS_OPENBSD 1
#elif defined __QNX__
# define H2OFT_OS_QNX 1
#endif  // __CYGWIN__

#if H2OFT_OS_LINUX

// TODO(kenton@google.com): Use autoconf to detect availability of
// gettimeofday().
# define H2OFT_HAS_GETTIMEOFDAY_ 1

# include <fcntl.h>  // NOLINT
# include <limits.h>  // NOLINT
# include <sched.h>  // NOLINT
// Declares vsnprintf().  This header is not available on Windows.
# include <strings.h>  // NOLINT
# include <sys/mman.h>  // NOLINT
# include <sys/time.h>  // NOLINT
# include <unistd.h>  // NOLINT
# include <string>

#elif H2OFT_OS_SYMBIAN
# define H2OFT_HAS_GETTIMEOFDAY_ 1
# include <sys/time.h>  // NOLINT

#elif H2OFT_OS_ZOS
# define H2OFT_HAS_GETTIMEOFDAY_ 1
# include <sys/time.h>  // NOLINT

// On z/OS we additionally need strings.h for strcasecmp.
# include <strings.h>  // NOLINT

#elif H2OFT_OS_WINDOWS_MOBILE  // We are on Windows CE.

# include <windows.h>  // NOLINT
# undef min
# undef max
# undef ERROR

#elif H2OFT_OS_WINDOWS  // We are on Windows proper.

# if H2OFT_OS_WINDOWS_MINGW
// MinGW has gettimeofday() but not _ftime64().
// TODO(kenton@google.com): Use autoconf to detect availability of
//   gettimeofday().
// TODO(kenton@google.com): There are other ways to get the time on
//   Windows, like GetTickCount() or GetSystemTimeAsFileTime().  MinGW
//   supports these.  consider using them instead.
#  define H2OFT_HAS_GETTIMEOFDAY_ 1
#  include <sys/time.h>  // NOLINT
# endif  // H2OFT_OS_WINDOWS_MINGW

// cpplint thinks that the header is already included, so we want to
// silence it.
# include <windows.h>  // NOLINT
# undef min
# undef max
# undef ERROR

#endif  // H2OFT_OS_LINUX

#if H2OFT_OS_WINDOWS
# define vsnprintf _vsnprintf
#endif  // H2OFT_OS_WINDOWS

// Brings in definitions for functions used in the testing::internal::posix
// namespace (read, write, close, chdir, isatty, stat). We do not currently
// use them on Windows Mobile.
#if H2OFT_OS_WINDOWS
# if !H2OFT_OS_WINDOWS_MOBILE
#  include <direct.h>
#  include <io.h>
# endif
// In order to avoid having to include <windows.h>, use forward declaration
// assuming CRITICAL_SECTION is a typedef of _RTL_CRITICAL_SECTION.
// This assumption is verified by
// WindowsTypesTest.CRITICAL_SECTIONIs_RTL_CRITICAL_SECTION.
struct _RTL_CRITICAL_SECTION;
#else
// This assumes that non-Windows OSes provide unistd.h. For OSes where this
// is not the case, we need to include headers that provide the functions
// mentioned above.
# include <unistd.h>
# include <strings.h>
#endif  // H2OFT_OS_WINDOWS

#if _MSC_VER >= 1500
# define H2OFT_DISABLE_MSC_WARNINGS_PUSH_(warnings) \
    __pragma(warning(push))                        \
    __pragma(warning(disable: warnings))
# define H2OFT_DISABLE_MSC_WARNINGS_POP_()          \
    __pragma(warning(pop))
#else
// Older versions of MSVC don't have __pragma.
# define H2OFT_DISABLE_MSC_WARNINGS_PUSH_(warnings)
# define H2OFT_DISABLE_MSC_WARNINGS_POP_()
#endif

namespace posix {
    // Functions with a different name on Windows.

#if H2OFT_OS_WINDOWS

# ifdef __BORLANDC__
    inline int IsATTY(int fd) { return isatty(fd); }
# else  // !__BORLANDC__
#  if H2OFT_OS_WINDOWS_MOBILE
    inline int IsATTY(int /* fd */) { return 0; }
#  else
    inline int IsATTY(int fd) { return _isatty(fd); }
#  endif  // H2OFT_OS_WINDOWS_MOBILE
# endif  // __BORLANDC__

# if H2OFT_OS_WINDOWS_MOBILE
    inline int FileNo(FILE* file) { return reinterpret_cast<int>(_fileno(file)); }
    // Stat(), RmDir(), and IsDir() are not needed on Windows CE at this
    // time and thus not defined there.
# else
    inline int FileNo(FILE* file) { return _fileno(file); }
# endif  // H2OFT_OS_WINDOWS_MOBILE

#else

    inline int FileNo(FILE* file) { return fileno(file); }
    inline int IsATTY(int fd) { return isatty(fd); }

#endif  // H2OFT_OS_WINDOWS

}  // namespace posix

#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.

enum H2OFTColor {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_PURPLE,
    COLOR_CYAN
};

#if H2OFT_OS_WINDOWS && !H2OFT_OS_WINDOWS_MOBILE && \
    !H2OFT_OS_WINDOWS_PHONE && !H2OFT_OS_WINDOWS_RT

// Returns the character attribute for the given color.
WORD GetForegroundColorAttribute(H2OFTColor color) {
    switch (color) {
    case COLOR_RED:    return FOREGROUND_RED;
    case COLOR_GREEN:  return FOREGROUND_GREEN;
    case COLOR_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
    case COLOR_BLUE:   return FOREGROUND_BLUE;
    case COLOR_PURPLE: return FOREGROUND_RED | FOREGROUND_BLUE;
    case COLOR_CYAN:   return FOREGROUND_BLUE | FOREGROUND_GREEN;
    default:           return 0;
    }
}

/*
// Returns the character attribute for the given color.
WORD GetBackgroundColorAttribute(H2OFTColor color) {
switch (color) {
case COLOR_RED:    return BACKGROUND_RED;
case COLOR_BLUE:   return BACKGROUND_BLUE;
case COLOR_GREEN:  return BACKGROUND_GREEN;
case COLOR_YELLOW: return BACKGROUND_RED | BACKGROUND_GREEN;
case COLOR_PURPLE: return BACKGROUND_RED | BACKGROUND_BLUE;
case COLOR_CYAN:   return BACKGROUND_BLUE | BACKGROUND_GREEN;
default:           return 0;
}
}
*/

#else
/*
Black       0;30     Dark Gray     1;30
Blue        0;34     Light Blue    1;34
Green       0;32     Light Green   1;32
Cyan        0;36     Light Cyan    1;36
Red         0;31     Light Red     1;31
Purple      0;35     Light Purple  1;35
Brown       0;33     Yellow        1;33
Light Gray  0;37     White         1;37
*/
// Returns the ANSI color code for the given color.
const char* GetAnsiColorCode(H2OFTColor color) {
    switch (color) {
    case COLOR_RED:     return "1";
    case COLOR_GREEN:   return "2";
    case COLOR_YELLOW:  return "3";
    case COLOR_BLUE:    return "4";
    case COLOR_PURPLE:  return "5";
    case COLOR_CYAN:    return "6";
    default:            return "7";
    };
}

#endif  // H2OFT_OS_WINDOWS && !H2OFT_OS_WINDOWS_MOBILE

// Returns true iff Google Test should use colors in the output.
bool ShouldUseColor(bool stdout_is_tty) {
    const std::string H2OFT_color = "auto";

    if (H2OFT_color == "auto") {
#if H2OFT_OS_WINDOWS
        // On Windows the TERM variable is usually not set, but the
        // console there does support colors.
        return stdout_is_tty;
#else
#   if H2OFT_OS_WINDOWS_MOBILE || H2OFT_OS_WINDOWS_PHONE | H2OFT_OS_WINDOWS_RT
        // We are on Windows CE, which has no environment variables.
        const std::string term = NULL;
#   elif defined(__BORLANDC__) || defined(__SunOS_5_8) || defined(__SunOS_5_9)
        // Environment variables which we programmatically clear will be set to the
        // empty string rather than unset (NULL).  Handle that case.
        const char* const env = getenv("TERM");
        const std::string term = std::string{ (env != NULL && env[0] != '\0') ? env : NULL };
#   else
        char* buffer = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&buffer, &sz, "TERM") == 0 && buffer == nullptr)
        {
            return stdout_is_tty;
        }
        const std::string term = std::string{ buffer };
#       define FREE_BUFFER free(buffer);
#   endif
        
        // On non-Windows platforms, we rely on the TERM variable.
        const bool term_supports_color =
            term == "xterm" ||
            term == "xterm-color" ||
            term == "xterm-256color" ||
            term == "screen" ||
            term == "screen-256color" ||
            term == "tmux" ||
            term == "tmux-256color" ||
            term == "rxvt-unicode" ||
            term == "rxvt-unicode-256color" ||
            term == "linux" ||
            term == "cygwin";
        
#       ifdef FREE_BUFFER
        FREE_BUFFER
#       undef FREE_BUFFER
#       endif


        return stdout_is_tty && term_supports_color;
#   endif  // H2OFT_OS_WINDOWS
    }

    return H2OFT_color == "yes" ||
        H2OFT_color == "true" ||
        H2OFT_color == "t" ||
        H2OFT_color == "1";
    // We take "yes", "true", "t", and "1" as meaning "yes".  If the
    // value is neither one of these nor "auto", we treat it as "no" to
    // be conservative.
}

// Helpers for printing colored strings to stdout. Note that on Windows, we
// cannot simply emit special characters and have the terminal change colors.
// This routine must actually emit the characters rather than return a string
// that would be colored when printed, as can be done on Linux.
void ColoredPrintf(H2OFTColor color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

#if H2OFT_OS_WINDOWS_MOBILE || H2OFT_OS_SYMBIAN || H2OFT_OS_ZOS || \
    H2OFT_OS_IOS || H2OFT_OS_WINDOWS_PHONE || H2OFT_OS_WINDOWS_RT
    const bool use_color = AlwaysFalse();
#else
    static const bool in_color_mode =
        ShouldUseColor(posix::IsATTY(posix::FileNo(stdout)) != 0);
    const bool use_color = in_color_mode && (color != COLOR_DEFAULT);
#endif  // H2OFT_OS_WINDOWS_MOBILE || H2OFT_OS_SYMBIAN || H2OFT_OS_ZOS
    // The '!= 0' comparison is necessary to satisfy MSVC 7.1.

    if (!use_color) {
        vprintf(fmt, args);
        va_end(args);
        return;
    }

#if H2OFT_OS_WINDOWS && !H2OFT_OS_WINDOWS_MOBILE && \
    !H2OFT_OS_WINDOWS_PHONE && !H2OFT_OS_WINDOWS_RT
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Gets the current text color.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;

    // We need to flush the stream buffers into the console before each
    // SetConsoleTextAttribute call lest it affect the text that is already
    // printed but has not yet reached the console.
    fflush(stdout);
    SetConsoleTextAttribute(stdout_handle,
        GetForegroundColorAttribute(color) | FOREGROUND_INTENSITY);
    vprintf(fmt, args);

    fflush(stdout);
    // Restores the text color.
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
    printf("\033[0;3%sm", GetAnsiColorCode(color));
    vprintf(fmt, args);
    printf("\033[m");  // Resets the terminal to default.
#endif  // H2OFT_OS_WINDOWS && !H2OFT_OS_WINDOWS_MOBILE
    va_end(args);
}

#endif
