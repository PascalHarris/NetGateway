//
//  cpp_wangers.cpp
//  CPP Wangers
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include "cpp_wangers.h"
#include <stdlib.h>
#include <wordexp.h>
#include <sys/stat.h>
#include <assert.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <syslog.h>

using namespace std;

#define LOCALPATHLENGTH 1024

static char hex2int(int c) {
    if (isdigit(c)) {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return -1;
    }
}

unsigned char *quoted_printable_decode(const unsigned char *str, size_t length, size_t *ret_length, int replace_us_by_ws) {
    register unsigned int i;
    register unsigned const char *p1;
    register unsigned char *p2;
    register unsigned int h_nbl, l_nbl;
    
    size_t decoded_len, buf_size;
    unsigned char *retval;
    
    if (replace_us_by_ws) {
        replace_us_by_ws = '_';
    }
    
    i = length, p1 = str; buf_size = length;
    
    while (i > 1 && *p1 != '\0') {
        if (*p1 == '=') {
            buf_size -= 2;
            p1++;
            i--;
        }
        p1++;
        i--;
    }
    
    retval = (unsigned char*)malloc(buf_size + 1);
    i = length; p1 = str; p2 = retval;
    decoded_len = 0;
    
    while (i > 0 && *p1 != '\0') {
        if (*p1 == '=') {
            i--, p1++;
            if (i == 0 || *p1 == '\0') {
                break;
            }
            h_nbl = hexval_tbl[*p1];
            if (h_nbl < 16) {
                /* next char should be a hexadecimal digit */
                if ((--i) == 0 || (l_nbl = hexval_tbl[*(++p1)]) >= 16) {
                    free(retval);
                    return NULL;
                }
                *(p2++) = (h_nbl << 4) | l_nbl, decoded_len++;
                i--, p1++;
            } else if (h_nbl < 64) {
                /* soft line break */
                while (h_nbl == 32) {
                    if (--i == 0 || (h_nbl = hexval_tbl[*(++p1)]) == 64) {
                        free(retval);
                        return NULL;
                    }
                }
                if (p1[0] == '\r' && i >= 2 && p1[1] == '\n') {
                    i--, p1++;
                }
                i--, p1++;
            } else {
                free(retval);
                return NULL;
            }
        } else {
            *(p2++) = (replace_us_by_ws == *p1 ? '\x20': *p1);
            i--, p1++, decoded_len++;
        }
    }
    
    *p2 = '\0';
    *ret_length = decoded_len;
    return retval;
}

char *base64_decode(char *s) {
    char *p = s, *e, *r, *_ret;
    int len = strlen(s);
    unsigned char i, unit[4];
    
    e = s + len;
    len = len / 4 * 3 + 1;
    r = _ret = (char *) malloc(len);
    
    while (p < e) {
        memcpy(unit, p, 4);
        if (unit[3] == '=')
            unit[3] = 0;
        if (unit[2] == '=')
            unit[2] = 0;
        p += 4;
        
        for (i = 0; unit[0] != B64[i] && i < 64; i++)
            ;
        unit[0] = i == 64 ? 0 : i;
        for (i = 0; unit[1] != B64[i] && i < 64; i++)
            ;
        unit[1] = i == 64 ? 0 : i;
        for (i = 0; unit[2] != B64[i] && i < 64; i++)
            ;
        unit[2] = i == 64 ? 0 : i;
        for (i = 0; unit[3] != B64[i] && i < 64; i++)
            ;
        unit[3] = i == 64 ? 0 : i;
        *r++ = (unit[0] << 2) | (unit[1] >> 4);
        *r++ = (unit[1] << 4) | (unit[2] >> 2);
        *r++ = (unit[2] << 6) | unit[3];
    }
    *r = 0;
    
    return _ret;
}


//---------------UUID
char* uuid_generator(int length) { // 0 or values greater than 56 result in full length UUID
    srand(time(NULL));
    char* strUuid = new char[57];
    sprintf(strUuid, "%x%x-%x-%x-%x-%x%x%x",
            rand(), rand(),
            rand(),
            ((rand() & 0x0fff) | 0x4000),
            rand() % 0x3fff + 0x8000,
            rand(), rand(), rand());
    if ((length>0)&&(length<57)) {
        char* uuid2 = new char[length+1];
        strncpy(uuid2,strUuid,length);
        uuid2[length]='\0';
        return uuid2;
    }
    return strUuid;
}

//---------------Sort Functions
bool sortStringNumber (string i, string j) {
    return (std::stoi(i)<std::stoi(j));
}

//---------------String tools
string clean_whitespace(string source_string) {
    string returnValue = source_string;
    vector<string> bad_chars = make_vector<string>()<<" "<<"\n"<<"\t"<<"\b"<<"\v"<<"\r"<<"\e";
    for (int i=0; i<bad_chars.size(); i++) {
        find_and_replace(returnValue,bad_chars[i],"");
    }
    return returnValue;
}

string format(const char* fmt, ...) {
    int size = 512;
    char* buffer = 0;
    buffer = new char[size];
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    if (size<=nsize) {
        delete[] buffer;
        buffer = 0;
        buffer = new char[nsize+1];
        nsize = vsnprintf(buffer, size, fmt, vl);
    }
    string ret(buffer);
    va_end(vl);
    delete[] buffer;
    return ret;
}

void ci_find_and_replace(string& str,string oldStr,string newStr){
	string temp_string = str;
	transform(temp_string.begin(), temp_string.end(), temp_string.begin(), ::tolower);
	transform(oldStr.begin(), oldStr.end(), oldStr.begin(), ::tolower);
	int position=temp_string.find(oldStr);
	while(position!=string::npos){
		  temp_string.replace(position, oldStr.size(), newStr);
		  str.replace(position, oldStr.size(), newStr);
		  position=temp_string.find(oldStr);
	}
}

void find_and_replace(string& str,
                      const string& oldStr,
                      const string& newStr) {
    string::size_type pos = 0u;
    while((pos = str.find(oldStr, pos)) != std::string::npos) {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

vector<string> split(const string &text, char sep) {
    vector<string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != string::npos) {
        if (end != start) {
            tokens.push_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start) {
        tokens.push_back(text.substr(start));
    }
    return tokens;
}

char* generate_key(const int len) {
    static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
    
    char* returnValue = (char*)malloc(sizeof(char) * len);
    
    for (int i = 0; i < len; ++i) {
        returnValue[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    returnValue[len] = '\0';
    
    return returnValue;
}

string encryptDecrypt(string toEncrypt, const char* key) {
    for (int i=0;i<33;i++) {
        string s;
        char c = i;
        s.push_back(i);
        find_and_replace(toEncrypt,format("<%d>",i),s);
    }
    string output = toEncrypt;
    
    for (int i = 0; i < toEncrypt.size(); i++) {
        output[i] = toEncrypt[i] ^ key[i % (strlen(key) / sizeof(char))];
    }
    
    for (int i=0;i<33;i++) {
        string s;
        char c = i;
        s.push_back(i);
        find_and_replace(output,s,format("<%d>",i));
    }
    
    return output;
}

//------------------------------Path tools
char* expandPath(char* path) {
    wordexp_t exp_result;
    wordexp(path, &exp_result, 0);
    static char returnValue[LOCALPATHLENGTH];
    
    if (strlen(exp_result.we_wordv[0])<LOCALPATHLENGTH) {
        sprintf(returnValue,"%s",exp_result.we_wordv[0]);
    }
    wordfree(&exp_result);
    return returnValue;
}

signed makeDirectory(char* path, mode_t mode) {
    if (mkdir(path, mode) == -1) {
        if( errno == EEXIST ) {
            return 1;
        } else {
            printf ("Error creating folder %s\n",path);
        }
        return -1;
    }
    return 0;
}

int makePath(std::string s,mode_t mode) {
    size_t pre=0,pos;
    std::string dir;
    int mdret;
    
    if(s[s.size()-1]!='/'){
        s+='/';
    }
    
    while((pos=s.find_first_of('/',pre))!=std::string::npos){
        dir=s.substr(0,pos++);
        pre=pos;
        if(dir.size()==0) continue;
        if((mdret=mkdir(dir.c_str(),mode)) && errno!=EEXIST){
            return mdret;
        }
    }
    return mdret;
}

string clean_filename(string source_filename) {
    string returnValue = source_filename;
    vector<string> bad_chars = make_vector<string>()<<" "<<":"<<"&"<<"/"<<"-"<<"."<<"~"<<"%"<<"?"<<"*"<<"{"<<"}"<<"\\"<<"<"<<">"<<"+"<<"|"<<"_"<<"\"";
    for (int i=0; i<bad_chars.size(); i++) {
        find_and_replace(returnValue,bad_chars[i],"");
    }
    return returnValue;
}

bool file_exists (string filename) {
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}


//-----------------Logging
void create_log_entry(string programname, string logtext) {
    setlogmask (LOG_UPTO (LOG_NOTICE));
    
    openlog (programname.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    
    syslog (LOG_NOTICE, logtext.c_str(), getuid ());
    
    closelog ();
}
