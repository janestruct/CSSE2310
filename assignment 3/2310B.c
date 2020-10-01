#include "main_player.h"


int prime_choice(struct Game *game) {
    
    struct Player *player = &game->players[game->id];
    
    if (game->sites[player->location + 1].players <
            game->sites[player->location + 1].capacity) {
                
        for (int id = 0; id < game->pcount; id++) {
            if (game->id == id) {
                continue;
            }
            if (game->players[id].location <= 
                    player->location) {
                        
                return 0;
            }
        }
        
        return (player->location + 1);
    }
    return 0;
}

int beta_choice(struct Game *game) {
    struct Player *player = &game->players[game->id];
    
    if (player->money % 2 != 0) {
        for (int site = player->location + 1; site < game->siteCount; site++) {
            
            struct Site *current = &game->sites[site];
            
            if (current->type == Barrier) {
                
                break;
            }
            
            if (current->type == Mo 
                    && current->players < current->capacity) {
                
                return site;
            }
            
            
        }
        
    }
    
    return 0;
}

int player_card_counts(struct Player *player) {
    int count = 0;
    
    for (int i = 0; i < 5; i++) {
        count += player->cards[i];
    }
    return count;
}

int player_got_most(struct Game *game) {
    
    int playerCardCount = player_card_counts(&game->players[game->id]);
    for (int id = 0; id < game->pcount; id++) {
        if (game->id == id) {
            continue;
        }
        int count = player_card_counts(&game->players[id]);
        if (count >= playerCardCount) {
                    
            return 0;
        }
    }
    
    return 1;
}

int everyone_zero(struct Game *game) {
    for (int id = 0; id < game->pcount; id++) {
        if (player_card_counts(&game->players[id]) > 0) {
            return 0;
        }
    }
    return 1;
    
}

int card_choice(struct Game *game) {
    struct Player *player = &game->players[game->id];
    
    if (everyone_zero(game) || player_got_most(game)) {
        for (int site = player->location + 1; site < game->siteCount; site++) {
            
            struct Site *current = &game->sites[site];
            
            if (current->type == Barrier) {
                
                break;
            }
            
            if (current->type == Ri 
                    && current->players < current->capacity) {
                
                return site;
            }
            
            
        }
        
    }
    
    return 0;
}

int v2_choice(struct Game *game) {
    struct Player *player = &game->players[game->id];
    for (int site = player->location + 1; site < game->siteCount; site++) {
            
        struct Site *current = &game->sites[site];
        
        if (current->type == Barrier) {
            
            break;
        }
        
        if (current->type == V2 
                && current->players < current->capacity) {
            
            return site;
        }
    }
    return 0;
}


int final_choice(struct Game *game) {
    struct Player *player = &game->players[game->id];
    for (int site = player->location + 1; site < game->siteCount; site++) {
            
        struct Site *current = &game->sites[site];
        
        
        if (current->players < current->capacity) {
            
            return site;
        }
    }
    return 0;
}

/*
• If the next site is not full and all other players are on later sites than us, move forward one site.
• If we have an odd amount of money, and there is a Mo between us and the next barrier, then go there.
• If we have the most cards or if everyone has zero cards and there is a Ri between us and the next barrier,
then go there. Note: “most” means than noone else has as many cards as you do.
• If there is a V2 between us and the next barrier, then go there.
• Move forward to the earliest site which has room
*/
int get_choice(struct Game *game) {
    
    int choice;
    
    choice = prime_choice(game);
    
    if (!choice) {
        choice = beta_choice(game);
    }
    if (!choice) {
        choice = card_choice(game);
    }
    if (!choice) {
        choice = v2_choice(game);
    }
    if (!choice) {
        choice = final_choice(game);
    }
    
    
    return choice;
}