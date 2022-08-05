#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


/**
 * @brief global variable: myprog.
**/
char *myprog;

/**
 * @brief exits incase of an error.
 * @details exits and prints error message if any error accured.
 * @param msg char pointer that contains error message.
**/
static void error_exit(char* msg);

/**
 * @brief main method of programm
 * @details The client takes an URL as input, connects to the corresponding server and requests the file specified in the URL. 
 *          The transmitted content of that file is written to stdout or to a file.
 * @param argc argc count of arguments.
 * @param argv array contains argument values.
 * @return returns error if programm fails, otherwise returns 0.
**/
int main(int argc, char *argv[]){

    if(argc < 1)
        error_exit("The name of the program is not written.");
    myprog = argv[0];

    int opt;
    int p_flag = 0;
    int o_flag = 0;
    int d_flag = 0;

    char *p_arg = NULL;
    char *o_arg = NULL;
    char *d_arg = NULL;

    FILE *output = NULL;

    while ((opt = getopt(argc, argv, "p:o:d:")) != -1){
        switch (opt){
        case 'p':
            if(p_flag != 0)
                error_exit("The p option cannot be entered more than once.");
            p_flag = 1;
            p_arg = optarg;
            char *pntr = NULL;
            strtol(p_arg, &pntr, 10);
            if(strlen(pntr) != 0)
                error_exit("Enter the correct port number.");
            break;

        case 'o':
            if(o_flag != 0)
                error_exit("The o option cannot be entered more than once.");
            if(d_flag != 0)
                error_exit("The o option cannot be used with the d option.");
            o_flag = 1;
            o_arg = optarg;
            break;

        case 'd':
            if(d_flag != 0)
                error_exit("The d option cannot be entered more than once.");
            if(o_flag != 0)
                error_exit("The d option cannot be used with the o option.");
            d_flag = 1;
            d_arg = optarg;
            break;
        
        default:
            break;
        }
    }

    if((argc-optind) != 1)
        error_exit("URL must be entered");

    if(p_flag == 0)
        p_arg = "80";

    char *URL = argv[optind];

    char identifier[8];
    strncpy(identifier, URL, 7);
    if(strcmp(identifier, "http://") != 0)
        error_exit("Wrong identifier.");
    identifier[7] = '\0';

    char host_name[512];
    memset(host_name, '\0', sizeof(host_name));
    for(int i = 0; i < strlen(URL)-7; i++){
        if(URL[i+7] == ';' || URL[i+7] == '/' || URL[i+7] == '?' || URL[i+7] == ':' || URL[i+7] == '@' || URL[i+7] == '=' || URL[i+7] == '&')
            break;
        host_name[i] = URL[i+7];
    }
    host_name[strlen(host_name)] = '\0';
    
    int len = strlen(identifier)+strlen(host_name);
    char url_file_path[256];
    memset(url_file_path, '\0', sizeof(url_file_path));

    int k = 0;
    for(int i = 0; i  < strlen(URL)-len; i++){
        if(URL[i+len] == '/')
            break;
        k++;
    }
    int i = 0;
    while(URL[k+len] != '\0'){
        url_file_path[i++] = URL[k+len];
        k++;
    }
    //printf("HOST: %s -> PATH: %s -> IDENTIFIER: %s\n",host_name, url_file_path, identifier);
    char request[strlen(URL)+55];
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url_file_path, host_name);

    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int s, sfd;
    s = getaddrinfo(host_name, p_arg, &hints, &ai);
    if (s != 0){
        freeaddrinfo(ai);
        error_exit("getaddrinfo failed.");
    }

    sfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sfd == -1){
        freeaddrinfo(ai);
        error_exit("socket creation failed.");
    }
    
    if (connect(sfd, ai->ai_addr, ai->ai_addrlen) == -1){
        freeaddrinfo(ai);
        close(sfd);
        error_exit("connect failed.");
    }

    FILE *socket_file = fdopen(sfd, "r+");
    if(socket_file == NULL){
        freeaddrinfo(ai);
        close(sfd);
        error_exit("The socket_file cannot be opened.");
    }
    
    if(fputs(request, socket_file) == EOF){
        fclose(socket_file);
        freeaddrinfo(ai);
        close(sfd);
        error_exit("fputs failed.");
    }
    if(fflush(socket_file) == EOF){
        fclose(socket_file);
        freeaddrinfo(ai);
        close(sfd);
        error_exit("fflush failed.");
    }

    char buf[1024];
    char *tmp = NULL;
    
    if (fgets(buf, sizeof(buf), socket_file) != NULL){
        tmp = strdup(buf);
    }else{
        fclose(socket_file);
        freeaddrinfo(ai);
        close(sfd);
        error_exit("fget failed");
    }
    char *header = strtok(tmp, " ");
    char *status_code = strtok(NULL, " ");
    char *status_message = strtok(NULL, "\r\n");
    char *invalid_status_code = NULL;
    int response_status = (int)strtol(status_code, &invalid_status_code, 10);

    if(header == NULL || (strcmp(header, "HTTP/1.1") != 0) || (strcmp(invalid_status_code, "") != 0)){
        fclose(socket_file);
        freeaddrinfo(ai);
        close(sfd);
        free(tmp);
        fprintf(stderr, "[%s] ERROR: Protocol error!\n", myprog);
        exit(2);
    }

    if(response_status != 200){
        fclose(socket_file);
        freeaddrinfo(ai);
        close(sfd);
        free(tmp);
        fprintf(stderr, "[%s] ERROR: STATUS CODE->%d, MESSAGE->%s!\n", myprog, response_status, status_message);
        exit(3);
    }

    free(tmp);
    
    while (fgets(buf, sizeof(buf), socket_file) != NULL){
        if (strcmp(buf, "\r\n") == 0)
            break;
    }

    if(o_flag != 0){
        output = fopen(o_arg, "w");
        if(output == NULL){
            fclose(socket_file);
            freeaddrinfo(ai);
            close(sfd);
            error_exit("The output file cannot be opened.");
        }
    }
    
    if(d_flag != 0){
        char *file_name = "index.html";
        if(url_file_path[strlen(url_file_path)-1] != '/'){
            char *token = strtok(url_file_path, "/");   
            while( token != NULL) {
                file_name = token;
                token = strtok(NULL, "/");
            }
        }

        char d_arg_path[strlen(d_arg)+strlen(file_name)];
        strcpy(d_arg_path, d_arg);
        if(d_arg[strlen(d_arg)-1] != '/')
            strcat(d_arg_path, "/");
        strcat(d_arg_path, file_name);

        printf("DIRECTORY: %s\n", d_arg_path);

        output = fopen(d_arg_path, "w");
        if(output == NULL){
            fclose(socket_file);
            freeaddrinfo(ai);
            close(sfd);
            error_exit("The output file cannot be opened.");
        }
    }

    if(output == NULL)
        output = stdout;

    size_t size = 0;
    while(!feof(socket_file)){
        if((size = fread(buf, sizeof(char), 1, socket_file)) <= 0)
            break;
        if(fwrite(buf, sizeof(char), size, output) == 0){
            freeaddrinfo(ai);
            fclose(socket_file);
            fclose(output);
            close(sfd);
            error_exit("fwrite failed!");
        }
    }

    freeaddrinfo(ai);
    fclose(socket_file);
    fclose(output);
    close(sfd);
    return EXIT_SUCCESS;
}

static void error_exit(char *msg){
    fprintf(stderr, "[%s] ERROR: %s\n", myprog, msg);
	exit(EXIT_FAILURE);
}