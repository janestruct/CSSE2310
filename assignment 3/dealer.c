#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include "struct.h"
#include "dealer.h"

static int _null = 0;

// Display error message for the error 
ExitStatusDealer exit_program(ExitStatusDealer status)
{
 const char *message = "";
    
    switch (status) {
        case DNormal:
            break;
        case DArg:
            message = "Usage: 2310dealer deck path p1 {p2}\n";
            break;
        case DDeck:
            message = "Error reading deck\n";
            break;
        case DPath:
            message = "Error reading path\n";
            break;
        case DStart:
            message = "Error starting process\n";
            break;
        case DCommu:
            message = "Communications error\n";
            break;
        
    }
    
    fprintf(stderr, "%s", message);
    exit(status);
}

void handle_path(struct Game *game, const char *pathFileName) {
    
    FILE *pathFile = fopen(pathFileName, "r");
    if (pathFile == NULL) {
        exit_program(DPath);
    }
    
    char data[0xff];
    
    if (!fgets(data, 0xFF, pathFile)) {
        exit_program(DPath);
    }
    if (data[strlen(data) - 1] == '\n') {
        data[strlen(data) - 1] = 0;
    }
    game->path = strdup(&data[0]);
    
    fclose(pathFile);


}


void handle_deck(struct Game *game, const char *deckFileName) {
    
    FILE *deckFile = fopen(deckFileName, "r");
    if (deckFile == NULL) {
        exit_program(DDeck);
    }
    
    char data[0xff];
    
    if (!fgets(data, 0xFF, deckFile)) {
        exit_program(DDeck);
    }
    if (data[strlen(data) - 1] == '\n') {
        data[strlen(data) - 1] = 0;
    }
    
    int size;
    char deck[0xFF];
    
    if (sscanf(data, "%d%s", &size, deck) != 2 ||
            size < 4 || strlen(deck) != size) {
        exit_program(DDeck);
    }
    for (int card = 0; card < size; card++) {
        if (deck[card] < 'A' || deck[card] > 'E') {
            exit_program(DDeck);
        }
        
    }
    game->deck = strdup(&deck[0]);
    game->card = 0;
    fclose(deckFile);
}

struct Game *start_new_game(int argc, char **argv) {
    
    if (argc < 4) {
        exit_program(DArg);
    }
    int pcount = argc - 3;
    
    struct Game *res = malloc(sizeof(struct Game));
    
    res->pcount = pcount;
    
    res->froms = malloc(sizeof(int*) * pcount);
    res->tos = malloc(sizeof(int*) * pcount);
    res->players = malloc(sizeof(struct Player) * pcount);
    int reachedLocationPosition = pcount - 1;
    for (int p = 0; p < pcount; p++) {
        res->froms[p] = malloc(sizeof(int) * 2);
        res->tos[p] = malloc(sizeof(int) * 2);
        pipe(res->froms[p]);
        pipe(res->tos[p]);
        res->players[p].from = fdopen(res->froms[p][0], "r");
        res->players[p].to = fdopen(res->tos[p][1], "w");
        res->players[p].processId = 0; 
        res->players[p].programName = strdup(argv[p + 3]);
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
    
    handle_deck(res, argv[1]);
    handle_path(res, argv[2]);
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
    
    char *path = strdup(game->path);
    
    int error;
    int siteCount;
    char sites[strlen(path)];
    
    error = sscanf(path, "%d;%s", &siteCount, sites);
    free(path);
    if (error != 2 || siteCount < 2 || strlen(sites) != siteCount * 3) {
         exit_program(DPath);
        
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
            exit_program(DPath);
        }
        int capacity = (isdigit(third)) ? ((int)(third - '0')) : game->pcount;
        
        game->sites[pos].type = type;
        game->sites[pos].capacity = capacity;
        game->sites[pos++].players = 0;
    }
    // first and last site must be barrier 
    
    if (game->sites[0].type != Barrier ||
            game->sites[siteCount - 1].type != Barrier) {
        
        exit_program(DPath);
    }
    
    game->sites[0].players = game->pcount;
    
}

int run_player(struct Game *game, int p) {
    int processId = fork();
    
    if (processId == 0) {
        // child process
        close(game->tos[p][1]);
        close(game->froms[p][0]);
        dup2(game->tos[p][0], 0);
        dup2(game->froms[p][1], 1); 
        dup2(_null, 2);
        char *argv[4];
        memset(argv, 0, 4 * sizeof(char *));
        argv[0] = game->players[p].programName;
        argv[1] = malloc(0x16);
        argv[2] = malloc(0x16);
        sprintf(argv[1], "%d", game->pcount);
        sprintf(argv[2], "%d", p);
        execvp(argv[0], argv);
        
        exit_program(DStart);
        
    } 
    
    return processId;
    
}

void run_players(struct Game *game) {
    
    for (int p = 0; p < game->pcount; p++) {
        int pid = run_player(game, p);
        game->players[p].processId = pid;
        close(game->tos[p][0]);
        close(game->froms[p][1]);
        
        int chr = fgetc(game->players[p].from);
        if ((char)chr != '^') {
            int status;
            wait(&status);
            exit_program(DStart);
        }
       
        fputs(game->path, game->players[p].to);
        fputc('\n', game->players[p].to);
        fflush(game->players[p].to);
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
    
    fprintf(stdout, "Scores: %d", calculate_score(&game->players[0]));
    
    for (int i = 1; i < game->pcount; i++) {
        fprintf(stdout, ",%d", calculate_score(&game->players[i]));
    }
    
    fputc('\n', stdout);
}


void print_site(enum SiteTypes type) {
    
    switch (type) {
        case Mo:
            fprintf(stdout, "Mo ");
            break;
        case Ri:
            fprintf(stdout, "Ri ");
            break;
        case Do:
            fprintf(stdout, "Do ");
            break;
        case V1:
            fprintf(stdout, "V1 ");
            break;
        case V2:
            fprintf(stdout, "V2 ");
            break;
        case Barrier:
            fprintf(stdout, ":: ");
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
    fputc('\n', stdout);

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
                fprintf(stdout, "%d  ", player);
                printed[player] = true;
            } else {
                
                fputs("   ", stdout);
            }
            
            
        }
        fputc('\n', stdout);
        
    }
}


struct Player *who_should_play(struct Game *game) {
    int location = 0;
    
    for (int i = 0; i < game->siteCount; i++) {
        if (game->sites[i].players) {
            location = i;
            break;
        }
    }
    int reached = (-1);
    int id = 0;
    for (int p = 0; p < game->pcount; p++) {
        if (game->players[p].location == location && 
                game->players[p].reachedLocationPosition > 
                reached) {
            reached = game->players[p].reachedLocationPosition;
            id = p;
        }
    } 
    return &game->players[id];
    
}

bool game_ends(struct Game *game) {
    for (int p = 0; p < game->pcount; p++) {
        if (game->players[p].location != game->siteCount - 1) {
            return false;
        }
    }
    return true;
}

void early_game_over(struct Game *game) {
    for (int p = 0; p < game->pcount; p++) {
        fputs("EARLY\n", game->players[p].to);
        fflush(game->players[p].to);
    }
    exit_program(DCommu);
}

int communicate_choice(struct Player *player) {
    fputs("YT\n", player->to);
    fflush(player->to);
    
    char playerMessage[0xff];
    if (!fgets(playerMessage, 0xff, player->from)) {
        return 0;
    }
    playerMessage[strlen(playerMessage) - 1] = 0;
    
    if (strncmp("DO", playerMessage, 2)) {
        return 0;
    }
    
    int result;
    if (sscanf(playerMessage, "DO%d", &result) != 1) {
        return 0;
    }
    return result;
}

int card_index(char card) {
    if (card == 'A') {
        return 0;
    }
    if (card == 'B') {
        return 1;
    }
    if (card == 'C') {
        return 2;
    }
    if (card == 'D') {
        return 3;
    }
    if (card == 'E') {
        return 5;
    }
    return -1;
}

int pick_a_card(struct Game *game, struct Player *player) {
    
    char card = game->deck[game->card++];
    if (game->card == strlen(game->deck)) {
        game->card = 0;
    }
    int cardIndex = card_index(card);
    player->cards[cardIndex] += 1;
    
    return (cardIndex + 1);
    
}

int money_points(struct Player *player) {
    int m = player->money;
    if (m % 2 != 0) {
        m--;
    }
    return (m / 2);
    
}

void handle_choice(struct Game *game, struct Player *player, int choice) {
    int money = 0, points = 0, card = 0;
    
    struct Site *site = &game->sites[choice];
    
    switch (site->type) {
        case Mo:
            money += 3;
            player->money += 3;
            break;
        case Ri:
            card = pick_a_card(game, player);
            break;
        case Do:
            points = money_points(player);
            player->points += points;
            money = (-player->money);
            player->money = 0;
            break;
        case V1:
            player->v1 += 1;
            break;
        case V2:
            player->v2 += 1;
            break;
        case Barrier:
        case Invalid:
            break;
    }
    
    game->sites[player->location].players -= 1;
    player->location = choice;
    player->reachedLocationPosition = site->players;
    site->players += 1;
    
    char toPlayer[0xff];
    
    sprintf(toPlayer, "HAP%d,%d,%d,%d,%d\n", player->index,
            choice, points, money, card);
    
    for (int p = 0; p < game->pcount; p++) {
        fputs(toPlayer, game->players[p].to);
        fflush(game->players[p].to);
    }
    fprintf(stdout, "Player %d Money=%d V1=%d V2=%d Points=%d A=%d B=%d C=%d D=%d E=%d\n",
            player->index, player->money, player->v1, player->v2, player->points,
            player->cards[0], player->cards[1], player->cards[2], player->cards[3],
            player->cards[4]);
    
    
}

void game_lopp(struct Game *game) {
    
    do {
        struct Player *player = who_should_play(game);
        int choice = communicate_choice(player);
        if (!choice || choice <= player->location ||
                choice >= game->siteCount) {
            early_game_over(game);
        }
        for (int site = player->location + 1; site < choice; site++) {
            struct Site *current = &game->sites[site];
            if (current->type == Barrier) {
                early_game_over(game);
            }
        }
        handle_choice(game, player, choice);
        print_state(game);
    } while (!game_ends(game));
    
    
}

int main(int argc, char **argv) {
    struct Game *game = start_new_game(argc, argv);
    receive_path(game);
    _null = open("/dev/null", 2, "w");
    run_players(game);
    print_state(game);
    game_lopp(game);
    
    for (int p = 0; p < game->pcount; p++) {
        fputs("DONE\n", game->players[p].to);
        fflush(game->players[p].to);
    }
    
    print_scores(game);
    return 0;
}
