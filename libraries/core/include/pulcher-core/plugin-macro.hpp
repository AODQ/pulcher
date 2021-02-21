#pragma once

#if defined(__unix__) || defined(__APPLE__)
#define PUL_PLUGIN_DECL
#elif defined(_WIN32) || defined(_WIN64)
#define PUL_PLUGIN_DECL __declspec(dllexport)
#endif
