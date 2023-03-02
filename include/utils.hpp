#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "defines.hpp"


#define ALLOCTEST(x) if ((!x)) utils_out_of_memory(__FILE__, __LINE__)

void utils_out_of_memory(const char* filename, const int lineno);

void make_binary_file(const char* input_filename, const char* out_filename);

#endif //_UTILS_H_
