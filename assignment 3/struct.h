#ifndef STRUCT
#define STRUCT

typedef enum {
    Normal = 0,
    Arg = 1,
    Pcount = 2,
    PID = 3,
    Path = 4,
    Early = 5,
    Commu = 6
} ExitStatusPlayer;

typedef enum {
    DNormal = 0,
    DArg = 1,
    DDeck = 2,
    DPath = 3,
    DStart = 4,
    DCommu = 5
} ExitStatusDealer;

enum SiteTypes {
    Mo,
    Ri,
    Do,
    V1,
    V2,
    Barrier,
    Invalid //
};

struct Site {
    enum SiteTypes type;
    int capacity;
    int players;
};



#endif