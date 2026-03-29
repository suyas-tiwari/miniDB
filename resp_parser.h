#pragma once
#include <string>
#include <vector>

struct Command {
    std::string name;
    std::vector<std::string> args;
};

Command parseRESP(const std::string& input);