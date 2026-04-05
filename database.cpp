#include "database.h"
#include <iostream>
#include <unordered_map>
#include <string>

void Database::set(const std::string& key, const std::string& value){
    store[key]=value;
}

std::string Database::get(const std::string& key){
    if(store.find(key)!=store.end()) return store[key];
    return "";
}

bool Database::exists(const std::string& key){
    return store.find(key) != store.end();
}