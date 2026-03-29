#include "resp_parser.h"
#include <iostream>

using namespace std;

Command parseRESP(const std::string& input) {
    Command cmd;
    int pos = 0;
    
    //Ensure it's an array
    if(input.empty() || input[0] != '*') return cmd;
    pos++;
    
    while(pos < input.length() && input[pos] != '\r') pos++;
    
    std::string numStr = input.substr(1, pos - 1);
    int argumentCount = stoi(numStr);
    pos += 2; // Move past \r\n
    
    bool isCommand = true;
    
    // Parse each Bulk String
    while(argumentCount > 0 && pos < input.length()) {
        if(input[pos] == '$') pos++;
        int j = pos;
        
        while(pos < input.length() && input[pos] != '\r') pos++;
        if(j == pos) break; // Safety check
        
        int argumentSize = stoi(input.substr(j, pos - j));
        pos += 2; // Jump past \r\n to the start of the word
        
        if(isCommand) {
            cmd.name = input.substr(pos, argumentSize);
            isCommand = false;
        }
        else cmd.args.push_back(input.substr(pos, argumentSize));
        
        pos += argumentSize + 2; // Jump past the word AND its trailing \r\n
        argumentCount--;
    }
    
    return cmd;
}