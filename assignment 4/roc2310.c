#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h> 
#include <netdb.h> // socket
#include <sys/socket.h> // for inet_addr
#include <netinet/in.h> // for inet_addr
#include <arpa/inet.h> // for inet_addr
#include <pthread.h>

#define DEFAULT_SPLIT 2
#define DEFAULT_SIZE 512
#define DEFAULT_PORT 0

enum Err {
    NORMAL,
    ARGV,
    INPORT,
    REQMAP,
    CONNECTION,
    NOMAPENTRY,
    FAILCONNDEST
};

void quit(enum Err err) {
    
    const char *msg = "";
    
    switch (err) {
        case NORMAL:
            break;
        case ARGV:
            msg = "Usage: roc2310 id mapper {airports}\n";
            break;
        case INPORT:
            msg = "Invalid mapper port\n";
            break;
        case REQMAP:
            msg = "Mapper required\n";
            break;
        case CONNECTION:
            msg = "Failed to connect to mapper\n";
            break;
        case NOMAPENTRY:
            msg = "No map entry for destination\n";
            break;
        case FAILCONNDEST:
            msg = "Failed to connect to at least one destination\n";
            break;
    }
    
    fprintf(stderr, "%s", msg);
    exit(err);
}

static char *INFORMATIONS[DEFAULT_SIZE];
static int AMOUNT = 0;
static char *ID = NULL;
static int PORTS[DEFAULT_SIZE];
static int PAMOUNT;
static int MPORT = DEFAULT_PORT;
static bool FAILED = false;

bool valid_port_text(const char *text) {
    for (int i = 0; i < strlen(text); i++) {
        if (!isdigit(text[i])) {
            return false;
        }
    }
    return true;
}

bool valid_port(int port) {
    return port > 0 && port < 65535;
}


int connect_mapper(char *text) {

    int fd, fd2, port = DEFAULT_PORT;
    struct sockaddr_in sa;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(MPORT);
    int res = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
    
    if (res) {
        quit(CONNECTION);
    }
    fd2 = dup(fd);
    FILE *writeFile = fdopen(fd, "w");
    
    FILE *readFile = fdopen(fd2, "r");
    fprintf(writeFile, "?%s\n", text);
    fflush(writeFile);
    
    char *message = malloc(DEFAULT_SIZE);
    
    if (fgets(message, DEFAULT_SIZE, readFile)) {
        message[strlen(message) - 1] = '\0';
        port = atoi(message);
    }
    
    
    free(message);
    fclose(writeFile);
    fclose(readFile);
    close(fd);
    
    
    return port;
}

void check_parameters(int argc, char **argv) {
    
    if (argc < 3) {
        quit(ARGV);
    }
    
    bool gotMapper = strcmp("-", argv[2]);
    if (gotMapper) {
        if (!strcmp("", argv[2]) || !valid_port_text(argv[2])) {
            quit(INPORT);
        }
        MPORT = atoi(argv[2]);
        if (!valid_port(MPORT)) {
            quit(INPORT);
        }
    }
    PAMOUNT = argc - 3;
    
    for (int i = 0; i < PAMOUNT; i++) {
        char *text = argv[i + 3];
        
        if (valid_port_text(text)) {
            
            PORTS[i] = atoi(text);
        } else {
            if (!gotMapper) {
                quit(REQMAP);
            } else {
                
                PORTS[i] = connect_mapper(text);
            }
            
        }
    }
    
    ID = strdup(argv[1]);
}

void connect_airport(int port) {

    int fd, fd2;
    struct sockaddr_in sa;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    int res = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
    
    if (res) {
        FAILED = true;
        return;
    }
    fd2 = dup(fd);
    
    FILE *writeFile = fdopen(fd, "w");
    FILE *readFile = fdopen(fd2, "r");
    
    fprintf(writeFile, "%s\n", ID);
    fflush(writeFile);
    
    char *message = malloc(DEFAULT_SIZE);
    
    if (fgets(message, DEFAULT_SIZE, readFile)) {
        message[strlen(message) - 1] = '\0';
        INFORMATIONS[AMOUNT++] = message;
    } else {
        free(message);
        
    }

    fclose(writeFile);
    fclose(readFile);
    close(fd);
}


int main(int argc, char **argv) {
    
    check_parameters(argc, argv);
    for (int i = 0; i < PAMOUNT; i++) {
        connect_airport(PORTS[i]);
    }
    for (int i = 0; i < AMOUNT; i++) {
        printf("%s\n", INFORMATIONS[i]);
        fflush(stdout);
    }
    if (FAILED) {
        quit(FAILCONNDEST);
    }
    return 0;
}
