#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h> 
#include <netdb.h> // socket
#include <pthread.h>

#define DEFAULT_SPLIT 2
#define DEFAULT_SIZE 512
#define DEFAULT_PORT 0
#define ADD_PORT "!"
#define GET_PORT "?"
#define NO_PORT ";\n"
#define WITH_PORT "%d\n"
#define PORT_MAPPINGS "@"

// all the names and ports stored here globally shared by all the threads
static char *NAMES[DEFAULT_SIZE];
static int PORTS[DEFAULT_SIZE];
static int AMOUNT = 0;

// threads are locked while making changes in the names and ports
static pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER;


// adding name and port
void add_port(char *port, char *name) {
    PORTS[AMOUNT] = atoi(port);
    NAMES[AMOUNT] = name;
    AMOUNT += 1;
}

// returns the related port
int get_port(const char *name) {
    
    for (int i = 0; i < AMOUNT; i++) {
        
        if (!strcmp(name, NAMES[i])) {
            return PORTS[i];
        }
        
    }
    return DEFAULT_PORT;
}

// returns list of strings where each string is "name:port\n"
char **ordered_names_ports(void) {
    
    // prep ordered list 
    char *ordered[AMOUNT];
    for (int i = 0; i < AMOUNT; i++) {
        ordered[i] = NAMES[i];
    }
    
    // until finished loop
    bool finished = false;
    while (!finished) {
        finished = true;
        // iterate the strings and compare them to each other 
        for (int i = 1; i < AMOUNT; i++) {
            if (strcmp(ordered[i - 1], ordered[i]) > 0) {
                // make changes if a string supposed to be in wrong index
                char *order = ordered[i - 1];
                ordered[i - 1] = ordered[i];
                ordered[i] = order;
                finished = false;
            }
        }
    }
    // create the result strings "name:port\n"
    char **result = malloc(sizeof(char *) * AMOUNT);
    for (int i = 0; i < AMOUNT; i++) {
        result[i] = malloc(sizeof(char) * DEFAULT_SIZE);
        sprintf(result[i], "%s:%d\n", ordered[i], get_port(ordered[i]));
    }
    return result;
}


void print_ordered_names_ports(FILE *file) {
    if (!AMOUNT) {
        return;
    }
    char **strings = ordered_names_ports();
    for (int i = 0; i < AMOUNT; i++) {
        
        fputs(strings[i], file);
        fflush(file);
        free(strings[i]);
    }
    free(strings);
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

char **split_name_port(char *string, const char *regex) {
    
    if (!strlen(string)) {
        return NULL;
    }
    // count the ":"
    int count = 0;
    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == ':') {
            count++;
        }
    }
    if (count != 1) {
        // more then 1 or 0 ":"
        return NULL;
    }
    
    char *item = strtok(string, regex);
    
    if (!item || !strlen(item) || !valid_text(item)) {
        return NULL;
    }
    char *name = strdup(item);
    
    item = strtok(NULL, regex);
    
    int port = DEFAULT_PORT;
    if (!item || !strlen(item) || !valid_port_text(item) ||
            !(port = atoi(item)) || !valid_port(port)) {
        free(name);
        return NULL;
    }
    
    
    char **result = malloc(sizeof(char *) * DEFAULT_SPLIT);
    
    result[0] = name;
    result[1] = strdup(item);
    return result;
}

void add_name_port(char *string) {
    
    char **idPort = split_name_port(string, ":");
    if (idPort == NULL) {
        return;
    }
    
    int port = get_port(idPort[0]);
    if (port == DEFAULT_PORT) {
        add_port(idPort[1], idPort[0]);
        free(idPort[1]);
        free(idPort);
    } else {
        free(idPort[0]);
        free(idPort[1]);
        free(idPort);
    }

}

void *client_handler(void *arg) {
    
    
    int fd = *((int *)arg);
    int fd2 = dup(fd);
    FILE *readFile = fdopen(fd, "r");
    FILE *writeFile = fdopen(fd2, "w");
    char *clientMessage = malloc(sizeof(char) * DEFAULT_SIZE);
    int port;
    while (fgets(clientMessage, DEFAULT_SIZE, readFile)) {
        
        clientMessage[strlen(clientMessage) - 1] = '\0';
        pthread_mutex_lock(&LOCK);
        if (!strncmp(ADD_PORT, clientMessage, 1)) {
            // clientMessage == "!name:port"
            add_name_port(&clientMessage[1]);
        } else if (!strncmp(GET_PORT, clientMessage, 1)) {
            // clientMessage == "?name"
            port = get_port(&clientMessage[1]);
            if (port == DEFAULT_PORT) {
                fputs(NO_PORT, writeFile);
                fflush(writeFile);
            } else {
                fprintf(writeFile, WITH_PORT, port);
                fflush(writeFile);
            }
            
        } else if (!strcmp(PORT_MAPPINGS, clientMessage)) {
            // clientMessage == "@"
            print_ordered_names_ports(writeFile);
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



int main(int argc, char **argv) {
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
    
    int fd;
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
