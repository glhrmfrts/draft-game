#ifndef DRAFT_COMMON_H
#define DRAFT_COMMON_H

#include <algorithm>
#include <iostream>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <cctype>
#include <locale>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

#ifdef _WIN32
#define export_func __declspec(dllexport)
#else
#define export_func
#endif

#define ARRAY_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

struct ray
{
    vec3 Origin;
    vec3 Direction;
};

typedef vec4 color;

using std::string;
using std::vector;
using std::unordered_map;

#define ArrayCount(Arr) (sizeof(Arr)/sizeof(Arr[0]))

enum direction
{
    Direction_right,
    Direction_left,
};

#include <sstream>

inline string ToString(vec2 v2)
{
    std::ostringstream stream;
    stream << "[";
    stream << v2.x << "," << v2.y;
    stream << "]";

    return stream.str();
}

inline string ToString(vec3 v3)
{
    std::ostringstream stream;
    stream << "[";
    stream << v3.x << "," << v3.y << "," << v3.z;
    stream << "]";

    return stream.str();
}

inline string ToString(ray Ray)
{
    std::ostringstream stream;
    stream << "[";
    stream << ToString(Ray.Origin);
    stream << ",";
    stream << ToString(Ray.Direction);
    stream << "]";

    return stream.str();
}

template<typename T>
inline void Println(T Arg)
{
    std::cout << Arg << std::endl;
}

template<typename T>
inline T Lerp(T a, float t, T b)
{
    return (1.0f - t)*a + t*b;
}

inline float Square(float f)
{
    return f*f;
}

inline float Length2(const vec3 &v)
{
    return Square(v.x) + Square(v.y) + Square(v.z);
}

static inline int NextP2(int n)
{
	int Result = 1;
	while (Result < n)
	{
		Result <<= 1;
	}
	return Result;
}

static long int GetFileSize(FILE *handle)
{
	long int result = 0;

	fseek(handle, 0, SEEK_END);
	result = ftell(handle);
	rewind(handle);

	return result;
}

static FILE *OpenFile(const char *Filename, const char *Mode)
{
	FILE *Handle;
#ifdef _WIN32
	fopen_s(&Handle, Filename, Mode);
#else
	Handle = fopen(Filename, Mode);
#endif
	return Handle;
}

static const char *ReadFile(const char *filename)
{
	FILE *handle = OpenFile(filename, "r");
	long int length = GetFileSize(handle);
	char *buffer = new char[length + 1];
	int i = 0;
	char c = 0;
	while ((c = fgetc(handle)) != EOF) {
		buffer[i++] = c;
		if (i >= length) break;
	}

	buffer[i] = '\0';
	fclose(handle);

	return (const char *)buffer;
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
	rtrim(s);
	return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
	trim(s);
	return s;
}

#endif
