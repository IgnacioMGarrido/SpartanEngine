#pragma once

// Version
#define sp_version "v0.33"

// Class
#define SPARTAN_CLASS
#if SPARTAN_RUNTIME_SHARED == 1
#ifdef SPARTAN_RUNTIME
#define SPARTAN_CLASS __declspec(dllexport)
#else
#define SPARTAN_CLASS __declspec(dllimport)
#endif
#endif

// Optimisation
#define SP_OPTIMISE_OFF __pragma(optimize("", off))
#define SP_OPTIMISE_ON __pragma(optimize("", on))

// Warnings
#define SP_WARNINGS_OFF __pragma(warning(push, 0))
#define SP_WARNINGS_ON __pragma(warning(pop))

// Debug break
#define SP_DEBUG_BREAK() __debugbreak()

// Assert
#ifdef DEBUG
#define SP_ASSERT(expression) assert(expression)
#else
#define SP_ASSERT(expression)           \
if (!(##expression))                    \
{                                       \
    Spartan::Log::m_log_to_file = true; \
    LOG_ERROR(#expression);             \
    SP_DEBUG_BREAK();                   \
}
#endif

// Delete
#define SP_DELETE(x) \
if (x)               \
{                    \
    delete x;        \
    x = nullptr;     \
}

// Fix windows macros
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

//= DISABLED WARNINGS ==============================================================================================================================
// identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable: 4251) // https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=vs-2019
// non � DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'
#pragma warning(disable: 4275) // https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=vs-2019
// no definition for inline function 'function'
#pragma warning(disable: 4506) // https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4506?view=vs-2017
//==================================================================================================================================================
