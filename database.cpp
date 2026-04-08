#include "database.h"
#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

void Database::set(const string& key, const string& value){
    store[key]=value;
    if(aof_writer.is_open()){
        aof_writer << "*3\r\n"
                      << "$3\r\nSET\r\n"
                      << "$" << key.length() << "\r\n" << key << "\r\n"
                      << "$" << value.length() << "\r\n" << value << "\r\n";
        aof_writer.flush();
    }
}

bool Database::del(const string& key){
    if(store.find(key)!=store.end()){
        store.erase(key);
        aof_writer << "*2\r\n"
                    << "$3\r\nDEL\r\n"
                    <<"$"<<key.length()<<"\r\n"<<key<<"\r\n";
        aof_writer.flush();
        return true;
    }
    else return false;
}

bool Database::rename(const std::string& key, const std:: string& newKey){
    if(store.find(key)!=store.end()){
        store[newKey]=store[key];
        store.erase(key);
        aof_writer << "*3\r\n"
                    << "$6\r\nRENAME\r\n"
                    <<"$"<<key.length()<<"\r\n"<<key<<"\r\n"
                    <<"$"<<newKey.length()<<"\r\n"<<newKey<<"\r\n";
        aof_writer.flush();
        return true;
    }
    else return false;
}

string Database::incr(const std::string& key){
    string result_val;
    if(store.find(key) != store.end()){
        try{
            size_t char_processed=0;
            long long num = stoll(store[key], &char_processed); 
            if (char_processed != store[key].length()) {
                return "ERR"; 
            }
            else{
                num += 1;
                result_val = to_string(num);
                store[key] = result_val;
            }
        }
        catch(...){
            return "ERR";
        }
    }
    else{
        result_val = "1";
        store[key] = result_val;
    }
    if(aof_writer.is_open()){
        aof_writer << "*2\r\n"
                   << "$4\r\nINCR\r\n"
                   << "$" << key.length() << "\r\n" << key << "\r\n";
        aof_writer.flush();
    }
    return result_val;
}

Database::Database() {
    aof_reader.open("data.aof");
    
    if (!aof_reader.is_open()) {
        cerr << "Fatal Error: Could not open AOF file to disk!\n";
    }

    string array_header="";


    while(getline(aof_reader,array_header)){

        if (array_header.empty()) continue;

        if (array_header.back() == '\r') array_header.pop_back();

        string current_line="";

        string raw_resp_string=array_header;

        raw_resp_string+="\r\n";

        int lines_to_read = 2*stoi(array_header.substr(1));

        for(int i=0;i<lines_to_read;i++){
            getline(aof_reader,current_line);
            if (!current_line.empty() && current_line.back() == '\r') current_line.pop_back();
            raw_resp_string+=current_line+"\r\n";
        }
        
        Command c = parseRESP(raw_resp_string);

        if (c.name == "SET" && c.args.size() >= 2) {
            store[c.args[0]]=c.args[1];
        }
        else if (c.name == "DEL" && !c.args.empty()){
            string key = c.args[0];
            if(store.find(key)!=store.end()){
                store.erase(key);
            }
        }
        else if (c.name == "RENAME" && c.args.size()>=2){
            store[c.args[1]]=store[c.args[0]];
            store.erase(c.args[0]);
        }
        else if (c.name == "INCR" && !c.args.empty()){
            incr(c.args[0]);
        }
    }


    aof_reader.close();

    aof_writer.open("data.aof", ios::app);

    if (!aof_writer.is_open()) {
        cerr << "Fatal Error: Could not open AOF file to disk!\n";
    }
}

Database::~Database() {
    if (aof_writer.is_open()) {
        aof_writer.close();
        cout << "Database safely flushed and disconnected from disk.\n";
    }
}



string Database::get(const string& key){
    if(store.find(key)!=store.end()) return store[key];
    return "";
}

bool Database::exists(const string& key){
    return store.find(key) != store.end();
}