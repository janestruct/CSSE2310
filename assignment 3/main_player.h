#ifndef MAINPLAYER
#define MAINPLAYER

#include "struct.h"

struct Player {
    int index; // index of the player aka id
    int money; // money starts from 7
    int points; // points which matters at the end
    int v1; // how many times player reached v1
    int v2; 
    int location; // current site of the player 
    int reachedLocationPosition;
    int cards[5]; // each index represents a card count
    
};


struct Game {
    int pcount;
    int id;
    int siteCount;
    struct Site *sites;
    struct Player *players;
};

int get_choice(struct Game *game);


#endif