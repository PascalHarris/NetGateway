//
//  load_settings.cpp
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include "load_settings.h"

using namespace std;

//int load_IMAP_Settings(char* configLocation, IMAPSetting &imap) {
int load_User_Settings(char* configLocation, UserDictionary &users) {
    char configPath[MAXPATHLENGTH]; //buffer for alternative config filename
    
    strcpy( configPath, strlen(configLocation)==0?"/etc/mailserver.cfg":configLocation );
    
    FILE * fp;
    char buffer[MAXLINE];
    int count = 0 ;
    if((fp = fopen(configPath,"r")) != NULL){
        char paramname[15];
        char parameter[MAXLINE];
        signed account_number=-1; //-1 because an account hasn't been found yet
        
        while(fgets(buffer,MAXLINE,fp)!=NULL){
            count++;
            sscanf(buffer, "%s %s\n", &paramname[0], &parameter[0]);
            if (strcmp(paramname, "refresh") == 0) {
                //frequency with which to fetch new email
                if ((atoi(parameter)>0) && (atoi(parameter)<1440)) {
                    users.refreshFrequency = atoi(parameter);
                }
            } else if (strcmp(paramname, "[account]") == 0) {
                account_number++; //found a new account - time to increment
            } else if (strcmp(paramname, "username") == 0) {
                strcpy(users.user[account_number].imap.username, parameter);
                strcpy(users.user[account_number].pop.username, parameter); //may want to provide an optional separate pop account in future
                strcpy(users.user[account_number].smtp.username, parameter); //may want to provide an optional separate pop account in future
            } else if (strcmp(paramname, "servername") == 0) {
                string username = format("%s@%s",users.user[account_number].imap.username,parameter);
                strcpy(users.user[account_number].imap.username, username.c_str());
                strcpy(users.user[account_number].smtp.username, username.c_str()); //may want to provide an optional separate pop account in future
            } else if (strcmp(paramname, "password") == 0) {
                string password;
                string key;
                password.assign(parameter,3,strlen(parameter)-3);
                key.assign(parameter,0,3);
                password = encryptDecrypt(password,key.c_str());
                strcpy(users.user[account_number].imap.password, password.c_str());
                strcpy(users.user[account_number].pop.password, password.c_str()); //may want to provide an optional separate pop account in future
                strcpy(users.user[account_number].smtp.password, password.c_str()); //may want to provide an optional separate pop account in future
            } else if (strcmp(paramname, "imapurl") == 0) {
                strcpy(users.user[account_number].imap.url, parameter);
            } else if (strcmp(paramname, "smtpusername") == 0) {
                strcpy(users.user[account_number].smtp.username, parameter); //may want to provide an optional separate pop account in future
            } else if (strcmp(paramname, "smtpservername") == 0) {
                string username = format("%s@%s",users.user[account_number].smtp.username,parameter);
                strcpy(users.user[account_number].smtp.username, username.c_str()); //may want to provide an optional separate pop account in future
            } else if (strcmp(paramname, "smtppassword") == 0) {
                string password;
                string key;
                password.assign(parameter,3,strlen(parameter)-3);
                key.assign(parameter,0,3);
                password = encryptDecrypt(password,key.c_str());
                strcpy(users.user[account_number].smtp.password, password.c_str()); //may want to provide an optional separate pop account in future
            } else if (strcmp(paramname, "smtpurl") == 0) {
                strcpy(users.user[account_number].smtp.url, parameter);
            } else if (strcmp(paramname, "mailpath") == 0) { //destination for fetched email
                strcpy(users.user[account_number].mailpath, expandPath(parameter));
            } else if (strcmp(paramname, "#") == 0) {
                // we aren't interested - just a comment
            } else if (strcmp(paramname, "[account_end]") == 0) {
                // end of account setting, nothing to worry about
            } else {
                cout << "Error in line " << count << " of configuration" << endl;
                create_log_entry(APPNAME, format("Error in line %d of configuration", count));
            }
        }
        users.settingsCount = ++account_number;
        if ((users.refreshFrequency<1) || (users.refreshFrequency>1440)) {
            users.refreshFrequency = 60; // default value provided, so refresh every hour.
        }

        if(errno != 0){
           cout << "Configuration file could not be opened." << endl;
           create_log_entry(APPNAME, format("Configuration file could not be opened."));
           return(FUNCTION_FAILED);
        }
    } else {
        cout << "Configuration file could not be opened." << endl;
        create_log_entry(APPNAME, format("Configuration file could not be opened."));
        return(FUNCTION_FAILED);
    }
    return(FUNCTION_SUCCESS);
}
