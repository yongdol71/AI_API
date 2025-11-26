#pragma once

#include <string>
#include <vector>
#include <Windows.h>

// nlohmann/json 사용
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct CHATGPT_RESULT
{
    json o;  // jsonxx::Object 대신 json 사용
    std::string t;
    std::vector<char> data;
};
