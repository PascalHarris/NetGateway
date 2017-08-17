//
//  pop_server.cpp
//  Mailgateway
//
//  Created by Pascal Harris in August 2017.
//  Copyright (c) 2017 Pascal Harris / 45RPM Software. Some rights reserved.
//
#include "pop_server.h"
#include "cpp_wangers.h"
#include "netgate_tools.h"

#define get_args char *pa,*pb; pa = strchr(request, ' '); pb = strchr(request,'\r')
#define copy_args(x,y,z) if ((x != NULL)&& (y != NULL)) { strncpy(z, x + 1, y-x - 1); } else { strncpy(z,"0",1); }
#define send_err_message     send_data(client_sockfd, format("%s\r\n",format(responses[ERR],"").c_str()).c_str())
#define send_empty_ok        send_data(client_sockfd, format("%s\r\n",format(responses[OK],"").c_str()).c_str())
#define send_no_such_message send_data(client_sockfd, format("%s\r\n",format(responses[ERR],popmessages[6]).c_str()).c_str())
#define send_only_n_messages send_data(client_sockfd, format("%s\r\n",format(responses[ERR],format(popmessages[7],popList.POPEntry.size()).c_str()).c_str()).c_str())
#define send_messages_octets send_data(client_sockfd, format("%s\r\n",format(responses[OK],format(popmessages[8],popList.POPEntry.size(),popList.totalSize).c_str()).c_str()).c_str())

#define OK  0
#define ERR 1
#define PER 2

const char responses[][10]={
    {"+OK%s"},   //0
    {"-ERR%s"},  //1
    {"."},       //2
};

const char popmessages[][128]={
    {" Welcome"},                                         //0
    {" User %s attempting to log in"},                    //1
    {" User %s authenticated by password"},               //2
    {" Attempt to log in by user %s rejected"},           //3
    {" message %d already deleted"},                      //4
    {" message %d deleted"},                              //5
    {" no such message"},                                 //6
    {" no such message, only %lu messages in maildrop"},  //7
    {" %lu messages (%lu octets)"},                       //8
    {" %lu octets"},                                      //9
    {" maildrop has %lu messages (%lu octets)"},          //10
    {" bad command"},                                     //11
};


struct thread_args {
    int client_sockfd;
    UserDictionary users;
};

using namespace std;

char pop_user[USERNAMELENGTH];
char pop_pass[PASSWORDLENGTH]; //maybe need the path here too?
char pop_path[MAXPATHLENGTH];
bool pop_logged_in;

//----------------Mail Events Processing

void pop_respond(int client_sockfd, char* request, UserDictionary users) {
    char result[MAXLINE];
    
    if (strncmp(request, "USER", 4) == 0) {
        get_args;
        strncpy(pop_user, pa + 1, pb-pa - 1);
        if (check_user(pop_user, users, pop_path)) {
            send_empty_ok;
            create_log_entry(APPNAME, format(popmessages[1], pop_user));
        } else {
            send_err_message;
            create_log_entry(APPNAME, format(popmessages[3], pop_user));
        }
    } else if (strncmp(request, "PASS", 4) == 0) {
        get_args;
        strncpy(pop_pass, pa + 1, pb-pa - 1);
        if (check_name_pass(pop_user, pop_pass, users, pop_logged_in)) {
            send_empty_ok;
            create_log_entry(APPNAME, format(popmessages[2], pop_user));
        } else {
            send_err_message;
            create_log_entry(APPNAME, format(popmessages[3], pop_user));
        }
    } else if ((strncmp(request, "APOP", 4) == 0) && pop_logged_in) {
        send_empty_ok;
    } else if ((strncmp(request, "AUTH", 4) == 0) && pop_logged_in) {
        send_empty_ok;
    } else if ((strncmp(request, "CAPA", 4) == 0) && pop_logged_in) {
        send_empty_ok;
    } else if ((strncmp(request, "DELE", 4) == 0) && pop_logged_in) { //doesn't actually do the delete - the delete will happen at quittin time
        char param[10];
        get_args;
        copy_args(pa,pb,param);
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) {
            if ((atoi(param)>0) && (atoi(param)-1<popList.POPEntry.size())) {
                if (popList.POPEntry[atoi(param)-1].deleted) {
                    send_data(client_sockfd, format("%s\r\n",format(responses[ERR],
                                                                    format(popmessages[4],atoi(param)).c_str()).c_str()).c_str());
                } else {
                    popList.POPEntry[atoi(param)-1].deleted = true; //mark as deleted
                    send_data(client_sockfd, format("%s\r\n",format(responses[OK],
                                                                    format(popmessages[5],atoi(param)).c_str()).c_str()).c_str());
                }
            } else {
                send_no_such_message;
            }
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "LIST", 4) == 0) && pop_logged_in) { //don't list deleted messages
        char param[10];
        get_args;
        copy_args(pa,pb,param);
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) {
            if (atoi(param)>0) {
                if ((atoi(param)>0) && (atoi(param)-1<popList.POPEntry.size())) {
                    sprintf(result,"%s %d %lu\r\n",format(responses[OK],"").c_str(),atoi(param),popList.POPEntry[atoi(param)-1].messageSize);
                    send_data(client_sockfd, result);
                } else {
                    send_only_n_messages;
                }
            } else {
                send_messages_octets;
                for (int i =0;i<popList.POPEntry.size();i++) {
                    sprintf(result,"%d %lu\r\n",i+1,popList.POPEntry[i].messageSize);
                    send_data(client_sockfd, result);
                }
                send_data(client_sockfd, format("%s\r\n",format(responses[PER],"").c_str()).c_str());
            }
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "NOOP", 4) == 0) && pop_logged_in) {
        send_empty_ok;
    } else if (strncmp(request, "QUIT", 4) == 0) {
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) {
            for (int i=0; i<popList.POPEntry.size(); i++) {
                if (popList.POPEntry[i].deleted) {
                    remove(popList.POPEntry[i].messagePath); //delete all messages tagged for deletion
                }
            }
        }
        send_empty_ok;
        create_log_entry(APPNAME, format("POP Socket %d closed", client_sockfd));
        close(client_sockfd);
        pthread_exit((void*)1); 
    } else if ((strncmp(request, "RETR", 4) == 0) && pop_logged_in) {
        char param[10];
        get_args;
        copy_args(pa,pb,param);
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) {
            if ((atoi(param)>0) && (atoi(param)-1<popList.POPEntry.size())) {
                if (popList.POPEntry[atoi(param)-1].deleted) {
                    send_no_such_message;
                } else {
                    send_data(client_sockfd, format("%s\r\n",format(responses[OK],format(popmessages[9],popList.POPEntry[atoi(param)-1].messageSize).c_str()).c_str()).c_str());
                    string mailContent = GetMailContents(popList.POPEntry[atoi(param)-1]);
                    send_data(client_sockfd, mailContent.c_str());
                    send_data(client_sockfd, "\r\n");
                    send_data(client_sockfd, format("%s\r\n",format(responses[PER],"").c_str()).c_str());
                }
            } else {
                send_no_such_message;
            }
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "RSET", 4) == 0) && pop_logged_in) {
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) {
            for (int i=0; i<popList.POPEntry.size(); i++) {
                if (popList.POPEntry[i].deleted) {
                    popList.POPEntry[i].deleted = false; //untag the message as deleted
                }
            }
            send_data(client_sockfd, format("%s\r\n",format(responses[OK],format(popmessages[10],popList.POPEntry.size(),popList.totalSize).c_str()).c_str()).c_str());
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "STAT", 4) == 0) && pop_logged_in) { // don't include deleted messages
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) {
            sprintf(result,"%s %lu %lu\r\n",format(responses[OK],"").c_str(),popList.POPEntry.size(),popList.totalSize);
            send_data(client_sockfd, result);
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "STLS", 4) == 0) && pop_logged_in) {
        send_empty_ok;
    } else if ((strncmp(request, "TOP", 3) == 0) && pop_logged_in) { //need to implement
        char param[20];
        char message_no[10];
        char lines_co[10];
        get_args;
        copy_args(pa,pb,param);
        string s(param);
        size_t pos = s.find(" ");
        if (pos != string::npos) {
            strncpy(lines_co,s.substr(pos+1).c_str(),s.substr(pos+1).length());
            strncpy(message_no,s.substr(0,pos).c_str(),s.substr(0,pos).length());
        } else {
            send_err_message;
        }
        POPListing popList;
        int success = getList(pop_path, popList);
        if ((success == FUNCTION_SUCCESS) && ((atoi(lines_co)>0) && (atoi(lines_co)<200))) {
            if ((atoi(message_no)>0) && (atoi(message_no)-1<popList.POPEntry.size())) {
                if (popList.POPEntry[atoi(message_no)-1].deleted) {
                    send_no_such_message;
                } else {
                    string section_marker;
                    string mailContent;
                    bool mailStartFound = NO;
                    int lines_count = 0;
                    vector<string> lines_array = split(GetMailContents(popList.POPEntry[atoi(message_no)-1]),'\n');
                    for (int i=0; i<lines_array.size(); i++) {
                        if (lines_array[i].substr(0,2).compare("==") == 0) { //found a section_marker
                            if (mailStartFound) { // this is the second time we've seen this - so bail. -- might be better to check that the markers match!
                                break;
                            }
                            section_marker = lines_array[i];
                        }
                        if (lines_array[i].find("Content-Type: text/plain") != std::string::npos) {
                            mailStartFound = YES;
                        }
                        if (mailStartFound) {
                            lines_count++;
                        }
                        mailContent = mailContent + lines_array[i] + "\n";
                        if (lines_count > (atoi(lines_co))) {
                            break;
                        }
                    }
                    send_data(client_sockfd, mailContent.c_str());
                    send_data(client_sockfd, "\r\n");
                    send_data(client_sockfd, format("%s\r\n",format(responses[PER],"").c_str()).c_str());
                }
            } else {
                send_no_such_message;
            }
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "UIDL", 4) == 0) && pop_logged_in) { //need to implement
        char param[10];
        get_args;
        copy_args(pa,pb,param);
        POPListing popList;
        int success = getList(pop_path, popList);
        if (success == FUNCTION_SUCCESS) { // don't show deleted messages
            if (atoi(param)>0) {
                if ((atoi(param)>0) && (atoi(param)-1<popList.POPEntry.size())) {
                    string mid = getMessageID(GetMailContents(popList.POPEntry[atoi(param)-1]));
                    sprintf(result,"%s %d %s\r\n",format(responses[OK],"").c_str(),atoi(param),mid.c_str());
                    send_data(client_sockfd, result);
                } else {
                    send_only_n_messages;
                }
            } else {
                send_empty_ok;
                for (int i =0;i<popList.POPEntry.size();i++) {
                    string mid = getMessageID(GetMailContents(popList.POPEntry[i]));
                    sprintf(result,"%d %s\r\n",i+1,mid.c_str());
                    send_data(client_sockfd, result);
                }
                send_data(client_sockfd, format("%s\r\n",format(responses[PER],"").c_str()).c_str());
            }
        } else {
            send_err_message;
        }
    } else if ((strncmp(request, "XTND", 4) == 0) && pop_logged_in) {
        send_empty_ok;
    } else {
        send_data(client_sockfd, format("%s\r\n",format(responses[ERR],popmessages[11]).c_str()).c_str());
    }
}

void* pop_proc(void* t_params) {
    int client_sockfd, len;
    char buf[BUF_SIZE];
    
    pop_logged_in = false;
    
    thread_args *thread_params = static_cast<thread_args*>(t_params);
    UserDictionary userarg = thread_params->users;
    
    memset(buf, 0, sizeof(buf));
    client_sockfd = thread_params->client_sockfd;
    
    send_empty_ok;
    
    while (1) {
        memset(buf, 0, sizeof(buf));
        len = recv(client_sockfd, buf, sizeof(buf), 0);
        if (len > 0) {
            if (strncmp(buf, "PASS", 4) != 0) {
                create_log_entry(APPNAME, format("POP Client made request: %s", buf));
            } else {
                create_log_entry(APPNAME, format("POP Client supplied password"));
            }
            pop_respond(client_sockfd, buf, userarg);
        } /*else {
            create_log_entry(APPNAME, format("Client made empty request - exiting"));
            break;
        }*/
    }
    
    return NULL;
}

//----------------Mail Server

void* pop_server_thread(void* user_arg) {
    
    signal(SIGPIPE, SIG_IGN); //ignore pipe signal.For more see http://www.wlug.org.nz/SIGPIPE
    
    int server_sockfd, client_sockfd;
    socklen_t sin_size;
    struct sockaddr_in server_addr, client_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    //create socket
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("POP Socket create error\n");
        exit(1);
    }
    
    //set the socket's attributes
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(POPPORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_addr.sin_zero), 8);
    
    //create a link
    if (bind(server_sockfd, (struct sockaddr *) &server_addr,
             sizeof(struct sockaddr)) == -1) {
        perror("POP bind error\n");
        exit(1);
    }
    
    //listening requests from clients
    if (listen(server_sockfd, MAX_CLIENTS - 1) == -1) {
        perror("POP listen error\n");
        exit(1);
    }
    
    //accept requests from clients,loop and wait.
    while (1) {
        sin_size = sizeof(client_addr);
        if ((client_sockfd = accept(server_sockfd,
                                    (struct sockaddr *) &client_addr, &sin_size)) == -1) {
            //perror("POP accept request error\n");
            sleep(1);
            continue;
        }
        struct thread_args thread_params;
        thread_params.client_sockfd = client_sockfd;
        thread_params.users = *(UserDictionary*)user_arg;
        
        pthread_t id;
        
        pthread_create(&id, NULL, pop_proc, &thread_params);
        pthread_join(id, NULL);
        
        close(client_sockfd);
    }
}
