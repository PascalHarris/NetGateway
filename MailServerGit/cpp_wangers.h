//
//  cpp_wangers.h
//  CPP Wangers
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#define FUNCTION_SUCCESS 0
#define FUNCTION_FAILED 1

#define NO 0
#define YES 1

template <typename T>
class make_vector {
public:
    typedef make_vector<T> my_type;
    my_type& operator<< (const T& val) {
        data_.push_back(val);
        return *this;
    }
    operator std::vector<T>() const {
        return data_;
    }
private:
    std::vector<T> data_;
};

static char B64[64] = {
    'A','B','C','D','E','F','G',
    'H','I','J','K','L','M','N',
    'O','P','Q','R','S','T',
    'U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g',
    'h','i','j','k','l','m','n',
    'o','p','q','r','s','t',
    'u','v','w','x','y','z',
    '0','1','2','3','4','5','6',
    '7','8','9','+','/'
};

char *base64_decode(char *s);
char* uuid_generator(int length);
bool sortStringNumber (std::string i, std::string j);
std::string clean_whitespace(std::string source_string);
std::string format(const char* fmt, ...);
void find_and_replace(std::string& str,
                      const std::string& oldStr,
                      const std::string& newStr);
std::vector<std::string> split(const std::string &text, char sep);
char* generate_key(const int len);
std::string encryptDecrypt(std::string toEncrypt, const char* key);
char* expandPath(char* path);
signed makeDirectory(char* path, mode_t mode);
int makePath(std::string s,mode_t mode);
std::string clean_filename(std::string source_filename);
bool file_exists (std::string filename);
void create_log_entry(std::string programname, std::string logtext);
