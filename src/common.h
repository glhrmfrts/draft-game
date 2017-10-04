#ifndef DRAFT_COMMON_H
#define DRAFT_COMMON_H

#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

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

#ifdef DRAFT_DEBUG
#include <sstream>

inline string
ToString(vec3 v3)
{
    std::ostringstream stream;
    stream << "[";
    stream << v3.x << "," << v3.y << "," << v3.z;
    stream << "]";

    return stream.str();
}

inline string
ToString(ray Ray)
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
inline void
Println(T Arg)
{
    std::cout << Arg << std::endl;
}
#endif

inline float
Lerp(float A, float t, float B)
{
    return (1.0f - t)*A + t*B;
}

#endif
