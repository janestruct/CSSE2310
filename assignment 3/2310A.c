#include "main_player.h"
#include "struct.h"

/*
The player has money, and there is a Do site in front of them, go there.
• If the next site is Mo (and there is room), then go there.
• Pick the closest V1, V2 or ::, then go there.


*/
int get_choice(struct Game *game) {
    
    struct Player *this = &game->players[game->id];
    
    // he player has money, and there is a Do site in front of them, go there.
    if (this->money > 0) {
        
        for (int site = this->location + 1; site < game->siteCount; site++) {
            
            struct Site *current = &game->sites[site];
            
            if (current->type == Barrier) {
                
                break;
            }
            
            if (current->type == Do && current->players < current->capacity) {
                
                return site;
            }
            
            
        }
        
    }
    
    // If the next site is Mo (and there is room), then go there.
    struct Site *next = &game->sites[this->location + 1];
    if (next->type == Mo && next->players < next->capacity) {
        return (this->location + 1);
        
    }
    
    // Pick the closest V1, V2 or ::, then go there.
    for (int site = this->location + 1; site < game->siteCount; site++) {
            
        struct Site *current = &game->sites[site];
       
        
        if ((current->type == V1 || current->type == V2 || current->type == Barrier)
                && current->players < current->capacity) {
            
            return site;
        }
        
        
    }
    
    
    return 0;
}