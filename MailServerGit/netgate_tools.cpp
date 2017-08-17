//
//  netgate_tools.cpp
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include "netgate_tools.h"
#include "cpp_wangers.h"
#include <sys/stat.h>

using namespace std;

int createDirectories(UserDictionary users) {
    int returnValue = 0; // successful until proven otherwise
    
    for (int i=0; i<users.settingsCount; i++) {
        if (strlen(users.user[i].mailpath)>0) {
            string path = format("%s/%s/%s",users.user[i].mailpath,users.user[i].pop.username,INBOXNAME);
            makePath(path,0755);
            path = format("%s/%s/%s",users.user[i].mailpath,users.user[i].pop.username,TEMPNAME);
            makePath(path,0755);
        }
    }
    
    return returnValue;
}

//------------------------------Email tools
string getMessageID(string email) {
    istringstream email_content(email);
    string returnValue;
    
    for (string line; getline(email_content, line); ) {
        string key ("MESSAGE-ID:");
        string temp = line;
        transform(temp.begin(), temp.end(), temp.begin(), std::ptr_fun<int, int>(std::toupper));
        size_t found = temp.rfind(key); //find tag of interest
        if (found!=string::npos) {
            found = line.rfind(">"); //find closing mark
            temp = line.substr(0,found);
            if (found!=string::npos) {
                found = temp.rfind("<"); // find opening mark
                if (found!=string::npos) {
                    returnValue = line.substr(found+1,line.size()-found-3);
                    return returnValue;
                }
            }
        }
    }
    return returnValue;
}

//--------------------------------POP Tools
int getList(const char* mailDirectory, POPListing &poplist) {
    POPListing returnInfo;
    DIR *dir;
    struct dirent *ent;
    int count = 0, cumulative_size=0;
    if ((dir = opendir (mailDirectory)) != NULL) {
        // print all the files and directories within directory 
        while ((ent = readdir (dir)) != NULL) {
            if ((strncmp(ent->d_name, ".", 1) != 0)) {
                poplist.POPEntry.push_back(POPEnt()); //create new entry
                char filepath[MAXPATHLENGTH];
                strcpy(filepath, mailDirectory);
                strcat(filepath, "/");
                strcat(filepath, ent->d_name);
                
                strcpy(poplist.POPEntry[count].messagePath, filepath);
                struct stat stat_buf;
                int rc = stat(filepath, &stat_buf);
                poplist.POPEntry[count].messageSize = (rc == 0 ? stat_buf.st_size : 0);
                poplist.POPEntry[count].deleted = false;
                cumulative_size += poplist.POPEntry[count].messageSize;
                count++;
            }
        }
        poplist.totalSize = cumulative_size;
        closedir (dir);
        return FUNCTION_SUCCESS;
    } else {
        return FUNCTION_FAILED;
    }
}

string GetMailContents(POPEnt message) {
    ifstream file(message.messagePath);
    string returnValue((istreambuf_iterator<char>(file)),
                       istreambuf_iterator<char>());
    return returnValue;
}

void send_data(int sockfd, const char* data) {
    if (data != NULL) {
        send(sockfd, data, strlen(data), 0);
    }
}

//----------------User Authentication
signed check_user(char* username, UserDictionary users, char (&rcpt_path)[MAXPATHLENGTH]) {
    for (int i=0; i < users.settingsCount; i++) {
        if (strncmp(username, users.user[i].pop.username, strlen(username)) == 0) {
            string file_dir = format("%s/%s/%s",users.user[i].mailpath,users.user[i].pop.username,INBOXNAME);
            strncpy(rcpt_path, file_dir.c_str(),MAXPATHLENGTH); //file directory
            return 1;
        }
    }
    return 0;
}

int check_name_pass(char* username, char* pass, UserDictionary users, bool &logged_in) {
    string password(pass);
    password = clean_whitespace(password);
    for (int i=0; i < users.settingsCount; i++) {
        if (strncmp(username, users.user[i].pop.username, strlen(username)) == 0) {
            if (strncmp(password.c_str(), users.user[i].pop.password, strlen(password.c_str())) == 0) {
                logged_in = true;
                return 1;
            } else {
                break;
            }
        }
    }
    return 0;
}

