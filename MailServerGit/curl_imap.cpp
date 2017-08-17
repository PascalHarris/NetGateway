//
//  curl_imap.cpp
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
//  LibCURL is used according to the terms in the attached license file
#include "curl_imap.h"
#include "cpp_wangers.h"
#include "netgate_tools.h"
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace std;

string homedirectory;


// need code for regular fetch

// Commonly used functions
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string execute_curl_request(CURL* curl) {
    string readBuffer;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    
    if(res != CURLE_OK) { // Check for errors
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else { /* return the result of the request */
        return readBuffer;
    }
    
    return "";
}

// Curl tools
vector<string> curl_listroot(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_URL, homedirectory.c_str());
    
    string root_contents = execute_curl_request(curl);
    
    //return a vector
    vector<string> returnVector;
    istringstream root_entries(root_contents);
    for (string line; getline(root_entries, line); ) {
        string key ("\"");
        string temp;
        size_t found = line.rfind(key); //find closing quote
        if (found!=string::npos) {
            temp = line.substr(0,found);
        }
        found = temp.rfind(key); // find opening quote
        if (found!=string::npos) {
            temp = line.substr(found+1,line.size()-found-3);
            if (strncmp(temp.substr(0,1).c_str(), "[", 1) != 0) {
                find_and_replace(temp," ","%20");
                returnVector.push_back(temp); //add to vector
            }
        }
    }
    return returnVector;
}

string curl_list_subfolder(CURL* curl, string folder) {
    string listpath("LSUB \""+folder+"\" *");
    
    curl_easy_setopt(curl, CURLOPT_URL, homedirectory.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, listpath.c_str());
    
    return execute_curl_request(curl);
}

string curl_fetch(CURL* curl, string folder, int uid) {
    string searchpath(homedirectory+"/"+folder+"/;UID="+to_string(uid));
    
    curl_easy_setopt(curl, CURLOPT_URL, searchpath.c_str());
    
    return execute_curl_request(curl);
}

vector<int> curl_search(CURL* curl, string folder, string searchtype) {
    string searchpath(homedirectory+"/"+folder);
    
    string searchstring ("SEARCH "+searchtype);
    
    curl_easy_setopt(curl, CURLOPT_URL, searchpath.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, searchstring.c_str());
    
    string mail_contents = execute_curl_request(curl);
    
    //return a vector
    vector<int> returnVector;
    vector<string> tempVector = split(mail_contents,' ');
    for (int i=0; i<tempVector.size(); i++) {
        if (tempVector[i].find_first_not_of("1234567890") == string::npos) { // only add numeric entries
            returnVector.push_back(stoi(tempVector[i])); //add to vector
        }
    }
    return returnVector;
}

vector<int> curl_search(CURL* curl, string folder, vector<string> searchtypes) {
    vector<int> returnVector;
    for (int i=0; i<searchtypes.size(); i++) {
        vector<int> tempVector;
        tempVector = curl_search(curl,folder,searchtypes[i]);
        returnVector.insert(returnVector.end(),tempVector.begin(),tempVector.end());
        
        //now we need to sort and remove duplicates
        sort (returnVector.begin(), returnVector.end());//, sortStringNumber);
        returnVector.erase(unique(returnVector.begin(), returnVector.end()), returnVector.end());
    }
    return returnVector;
}

string curl_examine(CURL* curl, string folder) {
    string examinepath("EXAMINE \""+folder+"\"");
    
    curl_easy_setopt(curl, CURLOPT_URL, homedirectory.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, examinepath.c_str());
    
    return execute_curl_request(curl);
}

CURL* curl_login(char* username, char* password) {
    CURLcode res = CURLE_OK;
    
    CURL *curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    } else {
        return NULL;
    }
    return curl;
}

void curl_reset(CURL* curl, char* username, char* password) {
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
}

int write_file(string filename, string mail_item) {
    ofstream out(filename.c_str());
    if (!out) {
        return FUNCTION_FAILED;
    }
    out << mail_item << endl;
    out.flush();
    usleep(500000); //wait for half a second to give the OS time to output the file
    if (!out) {
        return FUNCTION_FAILED;
    }
    out.close();
    if (!out) {
        return FUNCTION_FAILED;
    }
    return FUNCTION_SUCCESS;
}

int fetch_imap_inbox(IMAPSetting imap, string save_path, vector<string> searchtypes) {
    homedirectory.assign(imap.url,strlen(imap.url));
    
    CURLcode res = CURLE_OK;
    CURL *curl = curl_easy_init();
    
    curl = curl_login(imap.username, imap.password);
    if (curl) {
        vector<string> directory = curl_listroot(curl);
        if (directory.size()==0) {
            return nothing_fetched;
        }
        
        for (int i=0; i<directory.size(); i++) {
            vector<int> mail_list = curl_search(curl,directory[i],searchtypes);
            create_log_entry(APPNAME, format("Attempting to fetch %ld emails from %s", mail_list.size(), directory[i].c_str()));
            unsigned long skipped_files = 0;
            for (int j=0; j<mail_list.size(); j++) {
                curl_reset(curl, imap.username, imap.password);
                string mail_item = curl_fetch(curl,directory[i],mail_list[j]);
                if (mail_item.compare("") != 0) {
                    string m_id = getMessageID(mail_item); //might be better to md5 file to get its unique name
                    string filename;
                    size_t found = m_id.rfind("@"); // find opening quote
                    if (found!=string::npos) {
                        filename = clean_filename(m_id.substr(0,found)) + ".eml";
                    } else {
                        filename = clean_filename(m_id) + ".eml";
                    }
                    if (filename.compare(".eml")==0) {
                        create_log_entry(APPNAME, format("Unable to get filename"));
                        skipped_files++;
                        continue; //didn't get a name - so continue.  Might want to handle this properly in future.
                    }
                    
                    filename = save_path+"/"+TEMPNAME+"/"+filename; //save empty marker to the tempbox so we can check that we're not saving the same item twice. We could use a database for this - but the filesystem is probably just as good (and a deal more convenient)
                    if (!file_exists(filename)) {
                        string uuid_filename(uuid_generator(27));
                        uuid_filename = save_path+"/"+INBOXNAME+"/"+uuid_filename+".eml";
                        int success = write_file(uuid_filename, mail_item);
                        if (success == FUNCTION_SUCCESS) {
                            write_file(filename, ""); 
                        } else {
                            create_log_entry(APPNAME, format("Failed to write email %s", filename.c_str()));
                            skipped_files++;
                        }
                    } else {
                        skipped_files++;
                    }
                }
            }
            create_log_entry(APPNAME, format("Successfully fetched %ld emails from %s", mail_list.size()-skipped_files, directory[i].c_str()));
            if (skipped_files>0) {
                create_log_entry(APPNAME, format("Skipped %ld emails from %s", skipped_files, directory[i].c_str()));
            }
        }
        
        curl_easy_cleanup(curl); // Always cleanup
    }
    
    return (int)res;
}

void get_inbox(UserDictionary &userDict, vector<string> searchtypes) {
    for (int i=0; i<userDict.settingsCount; i++) { //iterate through all set up IMAP accounts
        string filename = format("%s/%s",userDict.user[i].mailpath,userDict.user[i].pop.username);
        fetch_imap_inbox(userDict.user[i].imap, filename, searchtypes);
    }
}

void* imap_thread(void* user_arg) {
    UserDictionary *userarg = static_cast<UserDictionary*>(user_arg);

    //add code to do a full refresh each midnight

    //First run - do a full fetch
    create_log_entry(APPNAME, format("Performing initial email fetch"));
    get_inbox(*userarg, make_vector<string>() << "SEEN" << "RECENT" << "NEW" << "ANSWERED" << "FLAGGED");
    while (1) {
    //Event loop for fetching IMAP required.  Should be on a configurable basis (minutes) or when a POP request is recieved
    // refresh run - do a recent email refresh
        sleep(60*(*userarg).refreshFrequency);
        create_log_entry(APPNAME, format("Refreshing email"));
        get_inbox(*userarg, make_vector<string>() << "SEEN" << "RECENT" << "NEW" << "ANSWERED" << "FLAGGED"); //<< "RECENT" << "NEW" << "FLAGGED"
    }
    
    pthread_exit( NULL );
}
