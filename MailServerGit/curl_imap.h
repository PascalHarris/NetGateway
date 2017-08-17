//
//  curl_imap.h
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include <stdio.h>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include "common_definitions.h"

#define nothing_fetched 1

int fetch_imap_inbox(IMAPSetting &imap, std::string save_path);
void* imap_thread(void* user_arg);
