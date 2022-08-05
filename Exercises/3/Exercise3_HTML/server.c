#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

/**
 *@brief global variable: myprog program name.
**/
char *myprog;

/**
 *@brief global variable: quit for signal.
**/
volatile sig_atomic_t quit = 0;

/**
 * @brief exits incase of an error.
 * @details exits and prints error message if any error accured.
 * @param msg char pointer that contains error message.
**/
static void error_exit(char *msg);

/**
 * @brief handle signal
 * @details makes variable quit 1.
 * @param signal 
**/
static void handle_signal(int signal);

/**
 * @brief main method of programm
 * @details The server waits for connections from clients and transmits the requested files.
 * @param argc argc count of arguments.
 * @param argv array contains argument values.
 * @return returns error if programm fails, otherwise returns 0.
**/
int main(int argc, char *argv[])
{

    if (argc < 1)
        error_exit("The name of the program is not written.");
    myprog = argv[0];

    int opt;
    int p_flag = 0;
    int i_flag = 0;

    char *p_arg = NULL;
    char *i_arg = NULL;

    while ((opt = getopt(argc, argv, "p:i:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (p_flag != 0)
                error_exit("The p option cannot be entered more than once.");
            p_flag = 1;
            p_arg = optarg;
            char *pntr = NULL;
            strtol(p_arg, &pntr, 10);
            if (strlen(pntr) != 0)
                error_exit("Enter the correct port number.");
            break;

        case 'i':
            if (i_flag != 0)
                error_exit("The i option cannot be entered more than once.");
            i_flag = 1;
            i_arg = optarg;
            break;

        default:
            break;
        }
    }

    if ((argc - optind) != 1)
        error_exit("DOC_ROOT must be entered");
    if (p_flag == 0)
        p_arg = "8080";
    if (i_flag == 0)
        i_arg = "index.html";

    char *DOC_ROOT = argv[optind];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    struct addrinfo hints;
    struct addrinfo *ai = NULL;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s, sfd, connfd;
    s = getaddrinfo(NULL, p_arg, &hints, &ai);
    if (s != 0)
    {
        freeaddrinfo(ai);
        error_exit("getaddrinfo failed.");
    }

    sfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sfd == -1)
    {
        freeaddrinfo(ai);
        error_exit("socket creation failed.");
    }
    int optval = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(sfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        freeaddrinfo(ai);
        close(sfd);
        error_exit("socket bind failed.");
    }

    if (listen(sfd, 1) < 0)
    {
        freeaddrinfo(ai);
        close(sfd);
        error_exit("socket listen failed.");
    }

    FILE *filepath = NULL;
    FILE *socket_file = NULL;
    while (!quit)
    {
        connfd = accept(sfd, NULL, NULL);
        if (connfd < 0)
        {
            if (errno == EINTR)
                break;
            freeaddrinfo(ai);
            close(sfd);
            error_exit("socket accept failed.");
        }

        socket_file = fdopen(connfd, "r+");
        if (socket_file == NULL)
        {
            close(connfd);
            if (errno == EINTR)
                break;
            freeaddrinfo(ai);
            close(sfd);
            error_exit("The socket_file cannot be opened.");
        }

        char buf[1024];
        char *tmp = NULL;
        if (fgets(buf, sizeof(buf), socket_file) != NULL)
        {
            tmp = strdup(buf);
        }
        else
        {
            fclose(socket_file);
            close(connfd);
            if (errno == EINTR)
                break;
            freeaddrinfo(ai);
            close(sfd);
            error_exit("fget failed");
        }
        while (fgets(buf, sizeof(buf), socket_file) != NULL){
            if (strcmp(buf, "\r\n") == 0)
                break;
        }

        char *request_method = strtok(tmp, " ");
        char *requested_file_path = strtok(NULL, " ");
        char *identifier = strtok(NULL, "\r\n");

        char request_header[32];
        memset(request_header, '\0', sizeof(request_header));
        int bool = 1;
        if (request_method == NULL || requested_file_path == NULL || identifier == NULL || strcmp(identifier, "HTTP/1.1") != 0)
        {
            bool = 0;
            strcpy(request_header, "HTTP/1.1 400 Bad Request\r\n");
        }

        if (bool == 1 && strcmp(request_method, "GET") != 0)
        {
            bool = 0;
            strcpy(request_header, "HTTP/1.1 501 Not implemented\r\n");
        }

        char path[strlen(DOC_ROOT) + strlen(requested_file_path) + 1];
        memset(path, '\0', sizeof(path));

        strcpy(path, DOC_ROOT);
        if(DOC_ROOT[strlen(DOC_ROOT)-1] != '/' && requested_file_path[0] == '/')
            strcat(path, requested_file_path);
        if(DOC_ROOT[strlen(DOC_ROOT)-1] == '/' && requested_file_path[0] == '/')
            strcat(path, ++requested_file_path);
        if (path[strlen(path) - 1] == '/')
            strcat(path, i_arg);

        filepath = fopen(path, "r");
        if (bool == 1 && filepath == NULL)
        {
            bool = 0;
            strcpy(request_header, "HTTP/1.1 404 Not Found\r\n");
        }

        if (bool == 1)
        {
            strcpy(request_header, "HTTP/1.1 200 OK");
        }
        free(tmp);
        if (filepath != NULL && bool == 1)
        {
            char date[128];
            time_t now = time(NULL);
            strftime(date, sizeof(date), "%a, %d %b %y %T %Z", gmtime(&now));

            fseek(filepath, 0, SEEK_END);
            int size = (int)ftell(filepath);
            rewind(filepath);
            //fprintf(stdout, "%s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", request_header, date, size);
            
            fprintf(socket_file, "%s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", request_header, date, size);
            fflush(socket_file);

            size_t len = 0;
            char data[1024];
            while (!feof(socket_file))
            {
                if ((len = fread(data, sizeof(char), 1, filepath)) <= 0)
                    break;
                if (fwrite(data, sizeof(char), len, socket_file) == 0){
                    if (errno == EINTR)
                        break;
                    close(connfd);
                    fclose(socket_file);
                    fclose(filepath);
                    freeaddrinfo(ai);
                    close(sfd);
                    error_exit("fwrite failed!");
                }
            }
            fflush(socket_file);
            fclose(socket_file);
            fclose(filepath);
        }
        else
        {
            fprintf(socket_file, "%s\r\nConnection: close\r\n\r\n", request_header);
            fflush(socket_file);
            fclose(socket_file);
        }
        close(connfd);
    }

    freeaddrinfo(ai);
    close(sfd);
    return EXIT_SUCCESS;
    
}

static void error_exit(char *msg)
{
    fprintf(stderr, "[%s] ERROR: %s\n", myprog, msg);
    exit(EXIT_FAILURE);
}

static void handle_signal(int signal)
{
    quit = 1;
}