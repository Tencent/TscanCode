
#include "tscexecutor.h"

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
static char exename[1024] = {0};
#endif


int main(int argc, char* argv[])
{
    // MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

    TscanCodeExecutor exec;
#ifdef _WIN32
    GetModuleFileNameA(NULL, exename, sizeof(exename)/sizeof(exename[0])-1);
    argv[0] = exename;
#endif

#ifdef NDEBUG
    try {
#endif
        return exec.check(argc, argv);
#ifdef NDEBUG
    } catch (const InternalError& e) {
        printf("%s\n", e.errorMessage.c_str());
    } catch (const std::exception& error) {
        printf("%s\n", error.what());
    } catch (...) {
        printf("Unknown exception\n");
    }
    return EXIT_FAILURE;
#endif
}


// Warn about deprecated compilers
#ifdef __clang__
#   if ( __clang_major__ < 2 || ( __clang_major__  == 2 && __clang_minor__ < 9))
#       warning "Using Clang 2.8 or earlier. Support for this version will be removed soon."
#   endif
#elif defined(__GNUC__)
#   if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4))
#       warning "Using GCC 4.3 or earlier. Support for this version will be removed soon."
#   endif
#elif defined(_MSC_VER)
#   if (_MSC_VER < 1600)
#       pragma message("WARNING: Using Visual Studio 2008 or earlier. Support for this version will be removed soon.")
#   endif
#endif

