#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "struct.h"
#include "main_player.h"



// Display error message for the error 
ExitStatusPlayer exit_program(ExitStatusPlayer status)
{
 const char *message = "";
    
    switch (status) {
        case Normal:
            break;
        case Arg:
            message = "Usage: player pcount ID\n";
            break;
        case Pcount:
            message = "Invalid player count\n";
            break;
        case PID:
            message = "Invalid ID\n";
            break;
        case Path:
            message = "Invalid path\n";
            break;
        case Early:
            message = "Early game over\n";
            break;
        case Commu:
            message = "Communications error\n";
            break;
        
    }
    
    fprintf(stderr, "%s", message);
    exit(status);
}

int convert_to_int(const char *string) {
    
    if (!strlen(string)) {
        
        return (-1);
    }
    
    for (int i = 0; i < strlen(string); i++) {
        
        if (!isdigit(string[i])) {
            
            return (-1);
        }
    }
    
    return atoi(string);
}


struct Game *start_new_game(int argc, char **argv) {
    
    if (argc != 3) {
        
        exit_program(Arg);
    }
    
    int pcount = convert_to_int(argv[1]);
    
    if (pcount < 1) {
        exit_program(Pcount);
    }
    
    int id = convert_to_int(argv[2]);
    
    if (id < 0 || id >= pcount) {
        exit_program(PID);
    }
    
    struct Game *res = malloc(sizeof(struct Game));
    
    res->pcount = pcount;
    res->id = id;
    
    res->players = malloc(sizeof(struct Player) * pcount);
    int reachedLocationPosition = pcount - 1;
    for (int p = 0; p < pcount; p++) {
        res->players[p].index = p;
        res->players[p].money = 7;
        res->players[p].points = 0;
        res->players[p].v1 = 0;
        res->players[p].v2 = 0;
        res->players[p].location = 0;
        res->players[p].reachedLocationPosition = reachedLocationPosition;
        reachedLocationPosition--;
        memset(res->players[p].cards, 0, sizeof(int) * 5);
        
    }
    
    return res;
}


enum SiteTypes get_type(char first, char second) {
    if (first == 'M' && second == 'o') {
        
        return Mo;
    }
    if (first == 'R' && second == 'i') {
        
        return Ri;
    }
    if (first == 'D' && second == 'o') {
        
        return Do;
    }
    if (first == 'V' && second == '1') {
        
        return V1;
    }
    if (first == 'V' && second == '2') {
        
        return V2;
    }
    if (first == ':' && second == ':') {
        
        return Barrier;
    }
    
    return Invalid;
}


void receive_path(struct Game *game) {
    // "4;::-V14Do3::-"
    
    char *path = malloc(80);
    
    if (!fgets(path, 79, stdin)) {
        // did not received path just got EOF
        exit_program(Path);
    }
    path[strlen(path) - 1] = '\0';
    
    int error;
    int siteCount;
    char sites[strlen(path)];
    
    error = sscanf(path, "%d;%s", &siteCount, sites);
    free(path);
    if (error != 2 || siteCount < 2 || strlen(sites) != siteCount * 3) {
         exit_program(Path);
        
    }
    
    game->siteCount = siteCount;
    game->sites = malloc(sizeof(struct Site) * siteCount);
    int pos = 0;
    // "::-V14Do3::-"
    for (int i = 0; i < strlen(sites); i += 3) {
        char first = sites[i];
        char second = sites[i + 1];
        char third = sites[i + 2];
        enum SiteTypes type = get_type(first, second);
        if (type == Invalid || third == '0' || (!isdigit(third) && third != '-')) {
            exit_program(Path);
        }
        int capacity = (isdigit(third)) ? ((int)(third - '0')) : game->pcount;
        
        game->sites[pos].type = type;
        game->sites[pos].capacity = capacity;
        game->sites[pos++].players = 0;
    }
    // first and last site must be barrier 
    
    if (game->sites[0].type != Barrier ||
            game->sites[siteCount - 1].type != Barrier) {
        
        exit_program(Path);
    }
    
    game->sites[0].players = game->pcount;
    
}


void print_site(enum SiteTypes type) {
    
    switch (type) {
        case Mo:
            fprintf(stderr, "Mo ");
            break;
        case Ri:
            fprintf(stderr, "Ri ");
            break;
        case Do:
            fprintf(stderr, "Do ");
            break;
        case V1:
            fprintf(stderr, "V1 ");
            break;
        case V2:
            fprintf(stderr, "V2 ");
            break;
        case Barrier:
            fprintf(stderr, ":: ");
            break;
        case Invalid:
            break;
    }
    
}

bool is_all_players_printed(bool *printed, int pcount) {
    
    for (int i = 0; i < pcount; i++) {
        if (!printed[i]) {
            
            return false;
        }
    }
    return true;
}

void print_site_names(struct Site *sites, int siteCount) {
    
    for (int i = 0; i < siteCount; i++) {
        print_site(sites[i].type);
    }
    fputc('\n', stderr);

}

void print_state(struct Game *game) {
    print_site_names(game->sites, game->siteCount);
     
    bool printed[game->pcount];
    memset(printed, 0, game->pcount * sizeof(bool));
    
    while (!is_all_players_printed(&printed[0], game->pcount)) {
        
        for (int i = 0; i < game->siteCount; i++) {
            int player = (-1);
            int reachedLocationPosition = game->pcount + 1;
            for (int j = 0; j < game->pcount; j++) {
                
                if (game->players[j].location == i && !printed[j])  {
                    if (game->players[j].reachedLocationPosition < reachedLocationPosition) {
                        
                        player = j;
                        reachedLocationPosition = game->players[j].reachedLocationPosition;
                        
                    }
                }
                
            }
            if (player >= 0) {
                fprintf(stderr, "%d  ", player);
                printed[player] = true;
            } else {
                
                fputs("   ", stderr);
            }
            
            
        }
        fputc('\n', stderr);
        
    }
    
    
}

void move_player_new_location(int player, int site, struct Game *game) {
    
    struct Player *p = &game->players[player];
    
    struct Site *current = &game->sites[p->location];
    
    struct Site *newSite = &game->sites[site];
    
    p->location = site;
    
    current->players -= 1;
    newSite->players += 1;
    
    p->reachedLocationPosition = newSite->players;
    
    
}


bool valid_pnsmc(struct Player *player, struct Site *site, int score,
        int money, int card) {
            
            
    return ((site->players == site->capacity) ||
            (score > 0 && site->type != Do) ||
            (player->money > 1 && score <= 0 && site->type == Do) ||
            (money < 0 && site->type != Do) ||
            (money > 0 && site->type != Mo) ||
            (money != 3 && site->type == Mo) ||
            (card < 0 || card > 5) ||
            (site->type == Ri && card == 0));        
}

int get_points_money(struct Player *p) {
    int money = p->money;
    if ((money % 2) != 0) {
        money -= 1;
    }
    int points = (money / 2);
    
    return points;
}

void hap_message_handler(struct Game *game, char *pnsmc) {
    
    int player, newSite, score, money, card;
    
    int error = sscanf(pnsmc, "%d,%d,%d,%d,%d", &player, &newSite,
            &score, &money, &card);
            
    if (error != 5 || player < 0 || player >= game->pcount) {
        exit_program(Commu);
    }
    struct Player *p = &game->players[player];
    
    if (newSite <= p->location || newSite > game->siteCount) {
        exit_program(Commu);
    }
    
    struct Site *s = &game->sites[newSite];
    
    
    if (valid_pnsmc(p, s, score, money, card)) {
        exit_program(Commu);
    }
    
    // here it looks like valid hap message
    
    move_player_new_location(player, newSite, game);
    p->money += money;
    p->points += score;
    
    switch (s->type) {
        case Mo:
            break;
        case Ri:
            p->cards[card - 1] += 1;
            break;
        case Do:
            break;
        case V1:
            p->v1 += 1;
            break;
        case V2:
            p->v2 += 1;
            break;
        case Barrier:
            break;
        case Invalid:
            break;
        
    }
    
    fprintf(stderr, "Player %d Money=%d V1=%d V2=%d Points=%d A=%d B=%d C=%d D=%d E=%d\n",
            player, p->money, p->v1, p->v2, p->points,
            p->cards[0], p->cards[1], p->cards[2], p->cards[3],
            p->cards[4]);
}


void game_loop(struct Game *game) {
    
    while (true) {
        char *dealerMessage = malloc(80);
        
        if (!fgets(dealerMessage, 79, stdin)) {
            free(dealerMessage);
            exit_program(Commu);
        }
        dealerMessage[strlen(dealerMessage) - 1] = '\0';
        
        if (!strcmp(dealerMessage, "EARLY")) {
            free(dealerMessage);
            exit_program(Early);
        } else if (!strcmp(dealerMessage, "DONE")) {
            free(dealerMessage);
            break;
        } else if (!strcmp(dealerMessage, "YT")) {
            int choice = get_choice(game);
            
            printf("DO%d\n", choice);
            fflush(stdout);
        } else if (!strncmp(dealerMessage, "HAP", 3)) {
            
            char *pnsmc = dealerMessage + 3;
            hap_message_handler(game, pnsmc);
            print_state(game);
            
            
        } else {
            
            exit_program(Commu);
        }
        
        free(dealerMessage);
    }
    
    
}


int get_set_of_cards(int *cards) {
    
    int set = 0;
    
    for (int i = 0; i < 5; i++) {
        
        if (cards[i] > 0) {
            
            cards[i] -= 1;
            set += 1;
        }
        
    }
    return set;
}


int calculate_card_score(struct Player *player) {
    int score = 0;
    while (true) {
        int set = get_set_of_cards(&player->cards[0]);
        
        if (set == 0) {
            break;
        } else if (set == 1) {
            score += 1;
        } else if (set == 2) {
            score += 3;
        } else if (set == 3) {
            score += 5;
        } else if (set == 4) {
            score += 7;
        } else {
            score += 10;
        } 
        
    }
    
    return score;
}

int calculate_vsite_score(struct Player *player) {
    int score = 0;
    score += player->v1;
    score += player->v2;
    
    return score;
    
}

int calculate_score2(struct Player *player) {
    
    int score = player->points;
    
    score += calculate_card_score(player);
    score += calculate_vsite_score(player);
    
    return score;
}

int calculate_score(struct Player *player) {
    int score = 0;
    
    score += player->points;
    score += player->v1;
    score += player->v2;
    
    while (true) {
        int set = get_set_of_cards(&player->cards[0]);
        
        if (set == 0) {
            break;
        } else if (set == 1) {
            score += 1;
        } else if (set == 2) {
            score += 3;
        } else if (set == 3) {
            score += 5;
        } else if (set == 4) {
            score += 7;
        } else {
            score += 10;
        } 
        
    }
    
    return score;
    
}

void print_scores(struct Game *game) {
    
    fprintf(stderr, "Scores: %d", calculate_score(&game->players[0]));
    
    for (int i = 1; i < game->pcount; i++) {
        fprintf(stderr, ",%d", calculate_score(&game->players[i]));
    }
    
    fputc('\n', stderr);
}

int main(int argc, char **argv) {
    // player pcount id
    struct Game *game = start_new_game(argc, argv);
    
    fputc('^', stdout);
    fflush(stdout);
    receive_path(game);    
    print_state(game);
    game_loop(game);
    print_scores(game);
    return Normal;
}


