#include "database.h"
#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

void Database::check_and_compact(){
    pid_t pid = fork();
    if(pid==0){
        ofstream compact("data.aof.compact");
        for(auto& i : store){
            string resp = "*3\r\n$3\r\nSET\r\n$" + to_string(i.first.length()) + 
                  "\r\n" + i.first + "\r\n$" + to_string(i.second.length()) + 
                  "\r\n" + i.second + "\r\n";
            compact<<resp;
        }
        compact.close();
        exit(0);
    }
    else if(pid>0){
        waitpid(pid,nullptr,0);
        string backlog_buffer;
        {
            lock_guard<mutex> lock(buffer_lock);
            swap(backlog_buffer, write_buffer);
        }
        ofstream compact("data.aof.compact", ios::app);
        compact << backlog_buffer;
        compact.close();
        ::rename("data.aof.compact", "data.aof");
        aof_writer.close();
        aof_writer.open("data.aof", ios::app);
    }
    else{
        cerr << "Fork failed, compaction aborted.\n";
        return;
    }
}

int Database::get_aof_size(){
    aof_writer.flush();
    ifstream f("data.aof", ios::ate | ios::binary);     //open the file in binary so that it doesn't count \r\n as \n or similar things and open ate means at end which opens the file from the end
    return f.tellg();   //stands for tell get position which basically returns the position where the cursor is at.
}

void Database::write_to_disk(){
    while(running){
        this_thread::sleep_for(chrono::seconds(1));
        string local_buffer;
        {
        lock_guard<mutex> lock(buffer_lock);
        swap(local_buffer, write_buffer);
        }
        if(!local_buffer.empty()) {
            aof_writer << local_buffer;
            aof_writer.flush();
            int size = get_aof_size();
            if(size > COMPACTION_THRESHOLD && size > size_after_last_compaction * 2) {
                check_and_compact();
                size_after_last_compaction = get_aof_size();
            }
        }
    } 
}

void Database::set(const string& key, const string& value){
    store[key]=value;
    
    string resp = "*3\r\n$3\r\nSET\r\n$" + to_string(key.length()) + 
              "\r\n" + key + "\r\n$" + to_string(value.length()) + 
              "\r\n" + value + "\r\n";
    lock_guard<mutex> lock(buffer_lock);    //Automatically opens the lock on going out of
    write_buffer += resp;
}

bool Database::del(const string& key){
    if(store.find(key)!=store.end()){
        store.erase(key);
        string resp = "*2\r\n$3\r\nDEL\r\n$"+to_string(key.length()) +
        "\r\n" + key + "\r\n";
        lock_guard<mutex> lock(buffer_lock);
        write_buffer += resp;
        return true;
    }
    else return false;
}

bool Database::rename(const std::string& key, const std:: string& newKey){
    if(store.find(key)!=store.end()){
        store[newKey]=store[key];
        store.erase(key);
        string resp = "*3\r\n$6\r\nRENAME\r\n$" + to_string(key.length()) + 
        "\r\n" + key + "\r\n$"+ to_string(newKey.length()) + 
        "\r\n" + newKey + "\r\n";
        lock_guard<mutex> lock(buffer_lock);
        write_buffer += resp;
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
    string resp = "*2\r\n$4\r\nINCR\r\n$" + to_string(key.length()) + 
                  "\r\n" + key + "\r\n";
    lock_guard<mutex> lock(buffer_lock);
    write_buffer += resp;
    
    return result_val;
}

Database::Database() {
    running=true;

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
            string key = c.args[0];
            if(store.find(key) != store.end()){
                try {
                     size_t char_processed = 0;
                    long long num = stoll(store[key], &char_processed); 
                    if (char_processed == store[key].length()) {
                        store[key] = to_string(num + 1);
                    }
                   } catch(...) { /* Ignore invalid values during boot */ }
            }
            else store[key] = "1";
        }
    }


    aof_reader.close();

    aof_writer.open("data.aof", ios::app);

    if (!aof_writer.is_open()) {
        cerr << "Fatal Error: Could not open AOF file to disk!\n";
    }

    writer_thread = thread(&Database::write_to_disk,this);
}

Database::~Database() {
    running = false;
    writer_thread.join();   //Waits for the process to finish instead of stopping immediately like detach

    if(!write_buffer.empty()){
        aof_writer<<write_buffer;
        aof_writer.flush();
    }

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

int Database::size(){
    return store.size();
}