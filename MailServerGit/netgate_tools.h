//
//  netgate_tools.h
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include "common_definitions.h"
#include <dirent.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <net/if.h>

int createDirectories(UserDictionary users);
std::string getMessageID(std::string email);
int getList(const char* mailDirectory, POPListing &poplist);
std::string GetMailContents(POPEnt message);
void send_data(int sockfd, const char* data);
signed check_user(char* username, UserDictionary users, char (&rcpt_path)[MAXPATHLENGTH]);
int check_name_pass(char* username, char* pass, UserDictionary users, bool &logged_in);
