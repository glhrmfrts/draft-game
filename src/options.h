#ifndef DRAFT_OPTIONS_H
#define DRAFT_OPTIONS_H

#include <nlohmann/json.hpp>

struct option
{
    using json = nlohmann::json;

    json::value_t Type;
    union
    {
        std::string String;
        float Float;
        int Int;
        bool Bool;
    };

	option() {}
	~option() {}
};

struct options
{
    std::unordered_map<std::string, option *> Values;
};

#endif