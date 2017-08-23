//
//  smtp_server.cpp
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
//  LibCURL is used according to the terms in the attached license file

#include "smtp_server.h"
#include "netgate_tools.h"
#include "cpp_wangers.h"
#include <curl/curl.h>

#define get_args(x) char *pa, *pb; pa = strchr(request, '<'); pb = strchr(request, '>'); strncpy(x, pa + 1, pb - pa - 1); send_empty_ok
#define send_bye                   send_data(client_sockfd, smtpmessages[1])
#define send_empty_ok              send_data(client_sockfd, smtpmessages[2])
#define send_error_not_implemented send_data(client_sockfd, smtpmessages[5])
#define send_error_bad_sequence    send_data(client_sockfd, smtpmessages[6])

#define state_loggedout       0
#define state_reset           1
#define state_initial         2
#define state_sender          3
#define state_recipient       4
#define state_i_authenticated 5
#define state_data_expected   6
#define state_ready_to_send   7

#define MAX_RCPT_USR          50
#define MAX_EMAIL_ADDRESS_LENGTH 128 // yes - I know it should be longer

const char smtpmessages[][128]={
    {"220 Ready\r\n"},                                               //0
    {"221 Bye\r\n"},                                                 //1
    {"250 OK\r\n"},                                                  //2
    {"354 Send from Rising mail proxy\r\n"},                         //3
    {"451 Requested action aborted: local error in processing\r\n"}, //4
    {"502 Error: command not implemented\r\n"},                      //5
    {"503 Error: bad sequence of commands\r\n"},                     //6
};

struct thread_args {
    int client_sockfd;
    UserDictionary users;
};

using namespace std;

int mail_stat;
int recipient_count = 0;
char recipient_list[MAX_RCPT_USR][MAX_EMAIL_ADDRESS_LENGTH] = {""};
char sender[MAX_EMAIL_ADDRESS_LENGTH] = "";

string payload_temp;

struct EmailData {
    const char *readptr;
    long sizeleft;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct EmailData *upload_email = (struct EmailData *)userp;
    
    if (size*nmemb < 1) {
        return 0;
    }
    
    if(upload_email->sizeleft) {
        *(char *)ptr = upload_email->readptr[0]; /* copy one single byte */
        upload_email->readptr++;                 /* advance pointer */
        upload_email->sizeleft--;                /* less data left */
        return 1;                        /* we return 1 byte at a time! */
    }
    
    return 0;                          /* no more data left to deliver */
}

int send_mail_data(UserDictionary users) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct EmailData upload_email;
    upload_email.readptr = payload_temp.c_str();
    upload_email.sizeleft = (long)strlen(payload_temp.c_str());
    
    int send_account = 0; // in the event that no valid account can be found, we'll send from the SMTP account associated with account 0
    for (int i=0; i<users.settingsCount; i++) {
        if (strncmp(sender, users.user[i].smtp.username, strlen(users.user[i].smtp.username))==0) {
            send_account = i;
            break;
        }
    }
    // defaults
    string s_username(users.user[send_account].smtp.username);
    string s_password(users.user[send_account].smtp.password);
    string s_url(users.user[send_account].smtp.url);
    
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, s_username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, s_password.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, s_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, sender);
        
        for (int i = 1; i < recipient_count; i++) {
            recipients = curl_slist_append(recipients, recipient_list[i]);
        }
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_email);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // debug only.
        res = curl_easy_perform(curl);
        
        /* Free the list of recipients */
        curl_slist_free_all(recipients);
        
        /* Always cleanup */
        curl_easy_cleanup(curl);
        
        curl_easy_reset(curl);
        
        if(res != CURLE_OK) {
            create_log_entry(APPNAME, format("SMTP Error forwarding to %s - %s",s_url.c_str(),curl_easy_strerror(res)));
            return FUNCTION_FAILED;
        } else {
            return FUNCTION_SUCCESS;
        }
    }
    return FUNCTION_FAILED;
}

// respond to request from the client
void smtp_respond(int client_sockfd, char* request, UserDictionary users) {
    char output[1024];
    memset(output, 0, sizeof(output));
    
    if (mail_stat == state_data_expected) { // aha!  The business end.  Email data to send
        string stringreq = clean_whitespace(format("%s",request));
        
        if (strncmp(stringreq.c_str(), ".", strlen(stringreq.c_str())) == 0) {
            int success = send_mail_data(users);
            if (success == FUNCTION_SUCCESS) {
                send_empty_ok;
            } else {
                send_data(client_sockfd, smtpmessages[4]);
            }
            mail_stat = state_initial; // might have to be state_reset
        } else {
            payload_temp=payload_temp+format("%s\n",request);
        }
    } else if (strncmp(request, "HELO", 4) == 0) {
        if (mail_stat == state_reset) {
            send_empty_ok;
            recipient_count = 0;
            memset(recipient_list, 0, sizeof(recipient_list));
            mail_stat = state_initial;
        } else {
            send_error_not_implemented;
        }
    } else if (strncmp(request, "MAIL FROM", 9) == 0) { // error if not surrounded by <> braces
        if (mail_stat == state_initial || mail_stat == state_i_authenticated) {
            get_args(sender);
            mail_stat = state_sender;
        } else {
            send_data(client_sockfd, "503 Error: send HELO/EHLO first\r\n");
        }
    } else if (strncmp(request, "RCPT TO", 7) == 0) { // error if not surrounded by <> braces
        if ((mail_stat == state_sender || mail_stat == state_recipient) && recipient_count < MAX_RCPT_USR) {
            get_args(recipient_list[recipient_count++]);
            mail_stat = state_recipient;
        } else {
            send_error_bad_sequence;
        }
    } else if (strncmp(request, "DATA", 4) == 0) {
        if (mail_stat == state_recipient) {
            send_data(client_sockfd, smtpmessages[3]);
            mail_stat = state_data_expected;
        } else {
            send_error_bad_sequence;
        }
    } else if (strncmp(request, "RSET", 4) == 0) {
        mail_stat = state_reset;
        send_empty_ok;
    } else if (strncmp(request, "NOOP", 4) == 0) {
        send_bye;
    } else if (strncmp(request, "QUIT", 4) == 0) {
        mail_stat = state_loggedout;
        send_bye;
        create_log_entry(APPNAME, format("SMTP Socket %d closed", client_sockfd));
        close(client_sockfd);
        pthread_exit((void*)1);
    } else if (strncmp(request, "EHLO", 4) == 0) { // this is all authentication stuff.  We don't care about this though - since this is for classic computers. //may need to care about this so we know which smtp account to send from!
        mail_stat = state_initial;
        send_empty_ok;
    } else if (strncmp(request, "AUTH LOGIN", 10) == 0) {
        send_empty_ok;
    } else if (strncmp(request, "AUTH LOGIN PLAIN", 10) == 0) {
        send_empty_ok;
    } else if (strncmp(request, "AUTH=LOGIN PLAIN", 10) == 0) {
        send_empty_ok;
    } else {
        send_error_not_implemented;
    }
}

// process mailing events
void *smtp_proc(void* t_params) {
    int client_sockfd, len;
    char buf[BUF_SIZE];
    
    thread_args *thread_params = static_cast<thread_args*>(t_params);
    UserDictionary userarg = thread_params->users;
    
    memset(buf, 0, sizeof(buf));
    client_sockfd = thread_params->client_sockfd;
    
    send_data(client_sockfd, smtpmessages[0]);
    mail_stat = state_reset;
    
    while (1) {
        memset(buf, 0, sizeof(buf));
        len = recv(client_sockfd, buf, sizeof(buf), 0);
        if (len > 0) {
            if (mail_stat != state_data_expected) {
                create_log_entry(APPNAME, format("SMTP Client made request: %s", buf));
            } else {
                create_log_entry(APPNAME, format("Client sending SMTP email data"));
            }
            smtp_respond(client_sockfd, buf, userarg);
        }
    }
    return NULL;
}

//----------------Mail Server
void* smtp_server_thread(void* user_arg) {
    
    signal(SIGPIPE, SIG_IGN);
    
    int server_sockfd, client_sockfd;
    socklen_t sin_size;
    struct sockaddr_in server_addr, client_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    //create socket
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("SMTP Socket create error\n");
        exit(1);
    }
    
    //set the socket's attributes
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SMTPPORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_addr.sin_zero), 8);
    
    //create a link
    if (bind(server_sockfd, (struct sockaddr *) &server_addr,
             sizeof(struct sockaddr)) == -1) {
        perror("SMTP bind error\n");
        exit(1);
    }
    // set to non-blocking to avoid lockout issue
    fcntl(server_sockfd, F_SETFL, fcntl(server_sockfd, F_GETFL, 0)|O_NONBLOCK);
    
    //listening requests from clients
    if (listen(server_sockfd, MAX_CLIENTS - 1) == -1) {
        perror("SMTP listen error\n");
        exit(1);
    }
    
    //accept requests from clients,loop and wait.
    sin_size = sizeof(client_addr);
    while (1) {
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
            sleep(1);
            continue;
        }
        
        struct thread_args thread_params;
        thread_params.client_sockfd = client_sockfd;
        thread_params.users = *(UserDictionary*)user_arg;
        
        pthread_t id;
        pthread_create(&id, NULL, smtp_proc, &thread_params);
        pthread_join(id, NULL);
    }
    close(client_sockfd);
}
