//
//  netgate.cpp
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include "netgate.h"
#include <signal.h>
#include <pthread.h>

using namespace std;

#define display_info printf("%s v%.2f (c)%d Pascal Harris, 45RPM Software\n", APPNAME, VERSIONNUMBER, YEAR)

pthread_t imapthread;
pthread_t popthread;
pthread_t smtpthread;

void shutdown_netgate(int s){
    // code for shutdown
    printf("\nCaught signal %d - Shutting Down\n",s);
    pthread_join(imapthread,NULL); // get rid of the imap_thread
    pthread_join(popthread,NULL); // get rid of the pop_thread
    exit(1);
}

int main(int argc, char *argv[]) {
    string username, password;
    int ch;
    bool expectingpath = 0;
    bool expectingpassword = 0;
    char s[MAXPATHLENGTH]; //buffer for alternative config filename
    
    create_log_entry(APPNAME, format("%s v%.2f starting up", APPNAME, VERSIONNUMBER));
    
    printf("%s v%.2f (c)%d Pascal Harris, 45RPM Software\n", APPNAME, VERSIONNUMBER, YEAR);
    
    if ((getuid())&&(geteuid())) {
        cout << APPNAME << " needs to be run as root." << endl;
        exit (1);
    }
    
    // process command line parameters
    for( int n = 1; n < argc; n++ ) {
        if (expectingpath) {
            strcpy( s, &argv[n][0] );
            expectingpath = 0;
        } else if (expectingpassword) {
            char* key = generate_key(3);
            strcpy( s, &argv[n][0] );
            cout << "\n" << key << encryptDecrypt(s,key) << "\n" << endl;
            expectingpassword = 0;
            exit(0);
        } else {
            switch( (int)argv[n][0] ) { // Check for option character.
                case '-':
                case '/':
                    int l = strlen( argv[n] );
                    for (int m = 1; m < l; ++m) {
                        ch = (int)argv[n][m];
                        switch( ch )
                        {
                            case 'h':
                            case 'H': 
                                cout << "-c <path> loads alternative configuration" << endl;
                                cout << "-h shows this screen" << endl;
                                cout << "-p <password> generates password for configuration file" << endl;
                                exit(0);
                            case 'v':
                            case 'V': // set verbose logging option
                                break;
                            case 'c': // String parameter
                            case 'C': expectingpath = 1;
                                break;
                            case 'p': // String parameter
                            case 'P': expectingpassword = 1;
                                break;
                        }
                    }
                    break;
            }
        }
    }
   // POPListing popList;
    
    UserDictionary users;
    if (load_User_Settings(s, users)==FUNCTION_SUCCESS) {
        //Event loop for fetching IMAP required.  Should be on a configurable basis (minutes) or when a POP request is received.  Should be in a separate thread.
        //fetch_imap_inbox(users.user[0].imap);
        // Try to create output folder
        createDirectories(users);
        
        //trap ctrl-c so that we can shut down tidily
        struct sigaction sigIntHandler;
        sigIntHandler.sa_handler = shutdown_netgate;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;
        sigaction(SIGINT, &sigIntHandler, NULL);
        
        //create threads
        //set up fetch of IMAP
        pthread_create(&imapthread, NULL, imap_thread, (void*)&users);
        
        //set up pop thread
        pthread_create(&popthread, NULL, pop_server_thread, (void*)&users);
        
        //set up smtp thread
        pthread_create(&smtpthread, NULL, smtp_server_thread, (void*)&users);
        
        for(;;) { // Main event loop
            //do nothing
        }
    } else {
        cout << "Failed to load configuration" << endl;
    }
}
