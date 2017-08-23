//
//  common_definitions.h
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include <vector>

#ifndef COMMON_DEFS_LOADED
#define COMMON_DEFS_LOADED

#define MAXLINE 128
#define MAXPATHLENGTH 1024
#define MAXDIRECTORYSIZE 10240
#define MAXPERMITTEDACCOUNTS 8
#define USERNAMELENGTH 50
#define PASSWORDLENGTH 50
#define INBOXNAME "inbox"
#define TEMPNAME "temp"

#define APPNAME "Mailgateway"
#define VERSIONNUMBER 0.3
#define YEAR 2017

#define MAX_CLIENTS 32
#define BUF_SIZE 1024

#define POPPORT 110    // use port 110, need root privilege
#define SMTPPORT 25  // use port 25, need root privilege

struct POPEnt {
    unsigned long messageSize;
    char messagePath[MAXPATHLENGTH];
    bool deleted;
};

struct POPListing {
    std::vector<POPEnt> POPEntry;
    unsigned long totalSize;
};

struct IMAPSetting { // fetch from modern server
    char username[USERNAMELENGTH];
    char password[PASSWORDLENGTH];
    char url[100];
};

struct SMTPSetting { // fetch from modern server
    char username[USERNAMELENGTH];
    char password[PASSWORDLENGTH];
    char url[100];
};

struct POPSetting { // for delivery to retro computer
    char username[USERNAMELENGTH];
    char password[PASSWORDLENGTH];
};

struct MailSettings {
    IMAPSetting imap;
    SMTPSetting smtp;
    POPSetting pop;
    char mailpath[MAXPATHLENGTH];
};

struct UserDictionary {
    MailSettings user[MAXPERMITTEDACCOUNTS];
    int settingsCount;
    int refreshFrequency;
};

#endif
