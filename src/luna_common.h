#ifndef LUNA_COMMON_H
#define LUNA_COMMON_H

#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

typedef vec4 color;

using std::string;
using std::vector;
using std::unordered_map;

#define Color_white color(1, 1, 1, 1)
#define Color_blue  color(0, 0, 0.5, 1)
#define Color_gray  color(0.5, 0.5, 0.5, 1)

struct rect
{
    float X, Y;
    float Width, Height;
};

#endif