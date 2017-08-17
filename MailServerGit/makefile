TARGET = mailgateway
ifeq ($(OS),Windows_NT)
	TARGET = mailgateway.exe
	CC = g++
	CFLAGS = -std=libstdc++
else
	UNAME = $(shell uname -s)
endif
ifeq ($(UNAME), Linux)
	CC = g++
	CFLAGS = -std=libstdc++
endif
ifeq ($(UNAME), Darwin)
	CC = clang++
	CFLAGS = -stdlib=libc++
endif

mailserver : netgate.o netgate_tools.o pop_server.o smtp_server.o curl_imap.o load_settings.o cpp_wangers.o
	$(CC) $(CFLAGS) netgate.o netgate_tools.o pop_server.o smtp_server.o curl_imap.o load_settings.o cpp_wangers.o -lcurl -o $(TARGET)
netgate.o : netgate.cpp
	$(CC) -c netgate.cpp
netgate_tools.o : netgate_tools.cpp
	$(CC) -c netgate_tools.cpp
curl_imap.o : curl_imap.cpp
	$(CC) -c curl_imap.cpp
pop_server.o : pop_server.cpp
	$(CC) -c pop_server.cpp
smtp_server.o : smtp_server.cpp
	$(CC) -c smtp_server.cpp
load_settings.o : load_settings.cpp
	$(CC) -c load_settings.cpp
cpp_wangers.o : cpp_wangers.cpp
	$(CC) -c cpp_wangers.cpp
clean :
	rm -f $(TARGET) *.o