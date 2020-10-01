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



static char *NAMES[DEFAULT_SIZE];
static int AMOUNT = 0;
static char *ID = NULL;
static char *INFORMATION = NULL;
static unsigned int PORT = 0;
// threads are locked while making changes in the names and ports
static pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER;

enum Err {
    NORMAL,
    ARGV,
    CHAR,
    INPORT,
    CONNECTION
};

void quit(enum Err err) {
    
    const char *msg = "";
    
    switch (err) {
        case NORMAL:
            break;
        case ARGV:
            msg = "Usage: control2310 id info [mapper]\n";
            break;
        case CHAR:
            msg = "Invalid char in parameter\n";
            break;
        case INPORT:
            msg = "Invalid port\n";
            break;
        case CONNECTION:
            msg = "Can not connect to map\n";
            break;
    }
    
    fprintf(stderr, "%s", msg);
    exit(err);
}


bool valid_char(char c) {
    return c != ':' && c != '\n' && c != '\r';
}

bool valid_text(const char *text) {
    for (int i = 0; i < strlen(text); i++) {
        if (!valid_char(text[i])) {
            return false;
        }
    }
    return true;
}

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



char **ordered_names(void) {
    // prep ordered list 
    char **result = malloc(sizeof(char *) * AMOUNT);
    for (int i = 0; i < AMOUNT; i++) {
        result[i] = NAMES[i];
    }
    bool finished = false;
    while (!finished) {
        finished = true;
        // iterate the strings and compare them to each other 
        for (int i = 1; i < AMOUNT; i++) {
            if (strcmp(result[i - 1], result[i]) > 0) {
                // make changes if a string supposed to be in wrong index
                char *order = result[i - 1];
                result[i - 1] = result[i];
                result[i] = order;
                finished = false;
            }
        }
    }
    return result;
}


void print_ordered_names(FILE *file) {
    if (!AMOUNT) {
        fputs(".\n", file);
        fflush(file);
        return;
    }
    char **strings = ordered_names();
    for (int i = 0; i < AMOUNT; i++) {
        
        fputs(strings[i], file);
        fputc('\n', file);
        fflush(file);
        free(strings[i]);
    }
    free(strings);
    fputs(".\n", file);
    fflush(file);
}


void *client_handler(void *arg) {
    
    
    int fd = *((int *)arg);
    int fd2 = dup(fd);
    FILE *readFile = fdopen(fd, "r");
    FILE *writeFile = fdopen(fd2, "w");
    char *clientMessage = malloc(sizeof(char) * DEFAULT_SIZE);
    
    
    if (fgets(clientMessage, DEFAULT_SIZE, readFile)) {
        clientMessage[strlen(clientMessage) - 1] = '\0';
        pthread_mutex_lock(&LOCK);
        
        if (!strcmp("log", clientMessage)) {
            
            print_ordered_names(writeFile);
        } else {
            if (strlen(clientMessage) &&
                    valid_text(clientMessage)) {
                        
                NAMES[AMOUNT++] = strdup(clientMessage);        
                fprintf(writeFile, "%s\n", INFORMATION);
                fflush(writeFile);
            }
        }
        pthread_mutex_unlock(&LOCK);
    }
    
    
    free(clientMessage);
    fclose(writeFile);
    fclose(readFile);
    close(fd2);
    close(fd);
    free(arg);
    return NULL;
}

int get_serv(void) {
    
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(& hints, 0, sizeof(struct addrinfo));
    hints.ai_family=AF_INET;        // IPv4  for generic could use AF_UNSPEC
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;  // Because we want to bind with it    
    getaddrinfo("localhost", DEFAULT_PORT, &hints, &ai);
    // create a socket and bind it to a port
    int serv = socket(AF_INET, SOCK_STREAM, 0); // 0 == use default protocol
    bind(serv, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr));
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    getsockname(serv, (struct sockaddr*)&ad, &len);
    printf("%u\n", ntohs(ad.sin_port));
    fflush(stdout);
    listen(serv, 10);
    PORT = ntohs(ad.sin_port);
    return serv;
}


void *register_control2310(void *arg) {
    
    int mapper = *((int *)arg);
    int fd;
    struct sockaddr_in sa;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(mapper);
    int res = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
    
    if (res) {
        quit(CONNECTION);
    }
    FILE *writeFile = fdopen(fd, "w");
    
    fprintf(writeFile, "!%s:%d\n", ID, PORT);
    fflush(writeFile);
    fclose(writeFile);
    close(fd);
    free(arg);
    
    return NULL;
}


int check_parameters(int argc, char **argv) {
    
    
    if (argc != 3 && argc != 4) {
        quit(ARGV);
    }
    
    if (!strcmp("", argv[1]) || !valid_text(argv[1]) ||
            !strcmp("", argv[2]) || !valid_text(argv[2])) {
        quit(CHAR);
    }
    int serv = get_serv();
    ID = strdup(argv[1]);
    INFORMATION = strdup(argv[2]);
    
    if (argc == 4) {
        
        if (!strcmp("", argv[3]) || !valid_port_text(argv[3])) {
            quit(INPORT);
        }
        int mapper = atoi(argv[3]);
        
        if (!valid_port(mapper)) {
            quit(INPORT);
        }
        
        int *arg = malloc(sizeof(int));
        *arg = mapper;
        pthread_t threadId;
        pthread_create(&threadId, 0, register_control2310, arg);
        pthread_detach(threadId);
    }
    return serv;
}

int main(int argc, char **argv) {
    
    
    int serv, fd;
    
    serv = check_parameters(argc, argv);
    
    while (true) {
        fd = accept(serv, 0, 0);
        if (fd < 0) {
            continue;
        }
        int *arg = malloc(sizeof(int));
        *arg = fd;
        pthread_t threadId;
        pthread_create(&threadId, 0, client_handler, arg);
        pthread_detach(threadId);
        
    }
    return 0;
}


