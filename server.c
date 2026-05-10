#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#define PORT 8080
#define MAX_HP 20
#define MAX_EP 10

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define RESET   "\033[0m"
#define BOLD    "\033[1m"

typedef struct {
    int hp;
    int ep;
    int defending;
    int wins;
    int heal_used;
} Player;

typedef struct {
    int attacks;
    int specials;
    int heals;
    int rests;
    int defends;
    int damage_dealt;
} Stats;

void add_log(char logs[3][128], const char *msg) {

    strcpy(logs[0], logs[1]);
    strcpy(logs[1], logs[2]);

    strncpy(logs[2], msg, 127);

    logs[2][127] = '\0';
}

void hp_bar(char *bar, int hp) {

    strcpy(bar, "HP [");

    for (int i = 0; i < MAX_HP; i++) {

        if (i < hp) {

            if (hp > 10)
                strcat(bar, GREEN "█" RESET);

            else if (hp > 5)
                strcat(bar, YELLOW "█" RESET);

            else
                strcat(bar, RED "█" RESET);
        }

        else {
            strcat(bar, "░");
        }
    }

    strcat(bar, "]");
}

void ep_bar(char *bar, int ep) {

    strcpy(bar, "EP [");

    for (int i = 0; i < MAX_EP; i++) {

        if (i < ep)
            strcat(bar, CYAN "■" RESET);

        else
            strcat(bar, "░");
    }

    strcat(bar, "]");
}

void send_screen(
    int sock,
    Player you,
    Player enemy,
    int yourTurn,
    char *msg,
    char *yourName,
    char *enemyName,
    int roundNum,
    char logs[3][128]
) {

    char buffer[4096];

    char hp1[256], hp2[256];
    char ep1[256], ep2[256];

    hp_bar(hp1, you.hp);
    hp_bar(hp2, enemy.hp);

    ep_bar(ep1, you.ep);
    ep_bar(ep2, enemy.ep);

    snprintf(buffer, sizeof(buffer),

        "\033[2J\033[H"

        CYAN BOLD
        "╔════════════════════════════════════════════╗\n"
        "║              STRIKE & SHIELD               ║\n"
        "╚════════════════════════════════════════════╝\n"
        RESET

        "\n"

        BOLD "ROUND %d\n" RESET
        "────────────────────────────────────────────\n"

        BOLD "SCOREBOARD\n" RESET
        "%s [%d]  VS  %s [%d]\n"
        "────────────────────────────────────────────\n\n"

        BOLD "PLAYER STATUS\n" RESET

        "%s\n"
        "%s\n"
        "%s\n"
        "Heal Status : %s\n"
        "%s\n\n"

        "%s\n"
        "%s\n"
        "%s\n"
        "Heal Status : %s\n"
        "%s\n\n"

        YELLOW BOLD "LAST ACTION\n" RESET
        "%s\n\n"

        BOLD "RECENT ACTIONS\n" RESET
        "1. %s\n"
        "2. %s\n"
        "3. %s\n\n"

        BOLD "GAME TIP\n" RESET
        "Use DEFEND when your opponent has enough EP for SPECIAL.\n\n",

        roundNum,

        yourName,
        you.wins,
        enemyName,
        enemy.wins,

        yourName,
        hp1,
        ep1,
        you.heal_used ? RED "USED" RESET : GREEN "READY" RESET,
        you.hp <= 5 ? RED BOLD "WARNING: LOW HP!" RESET : "",

        enemyName,
        hp2,
        ep2,
        enemy.heal_used ? RED "USED" RESET : GREEN "READY" RESET,
        enemy.hp <= 5 ? RED BOLD "WARNING: LOW HP!" RESET : "",

        msg,

        strlen(logs[0]) ? logs[0] : "-",
        strlen(logs[1]) ? logs[1] : "-",
        strlen(logs[2]) ? logs[2] : "-"
    );

    send(sock, buffer, strlen(buffer), 0);

    if (yourTurn) {

        char menu[] =

    GREEN BOLD
    "╔════════════════════════════════════════════╗\n"
    "║                 YOUR TURN                  ║\n"
    "╚════════════════════════════════════════════╝\n"
    RESET

    "\n"
    "[W] ATTACK   - Cost 2 EP | Deal 3 Damage\n"
    "[S] DEFEND   - Cost 2 EP | Reduce Damage\n"
    "[F] SPECIAL  - Cost 4 EP | Deal 5 Damage\n"
    "[C] HEAL     - Cost 5 EP | Recover 4 HP\n"
    "[X] REST     - Recover 1 EP\n\n"

    CYAN BOLD
    "PLAYER GUIDE\n"
    "────────────────────────────────────────────\n"
    RESET
    "Low HP?             Use [C] HEAL\n"
    "Low EP?             Use [X] REST\n"
    "Enemy has high EP?  Use [S] DEFEND\n"
    "Need to finish?     Use [F] SPECIAL\n"
    CYAN
    "────────────────────────────────────────────\n\n"
    RESET

    YELLOW BOLD
    "Choose your move:\n"
    RESET

    "> ";

        send(sock, menu, strlen(menu), 0);

    } else {

        char waitmsg[] =

            MAGENTA BOLD
            "╔════════════════════════════════════════════╗\n"
            "║          WAITING FOR OPPONENT MOVE         ║\n"
            "╚════════════════════════════════════════════╝\n"
            RESET;

        send(sock, waitmsg, strlen(waitmsg), 0);
    }
}

void intro_screen(int sock) {

    char intro[4096];

    snprintf(intro, sizeof(intro),

        "\033[2J\033[H"

        CYAN BOLD
        "╔══════════════════════════════════════╗\n"
        "║         STRIKE & SHIELD              ║\n"
        "╚══════════════════════════════════════╝\n"
        RESET

        "\n"

        BOLD "GAME INSTRUCTIONS\n\n" RESET

        "[W] ATTACK  -> Cost 2 EP | Deal 3 Damage\n"
        "[S] DEFEND  -> Cost 2 EP | Reduce Damage\n"
        "[F] SPECIAL -> Cost 4 EP | Deal 5 Damage\n"
        "[C] HEAL    -> Cost 5 EP | Recover 4 HP\n"
        "[X] REST    -> Recover 1 EP\n\n"

        BOLD "Press [R] when ready.\n\n" RESET

        "> "
    );

    send(sock, intro, strlen(intro), 0);
}

void ask_names(
    int p1_sock,
    int p2_sock,
    char *name1,
    char *name2
) {

    memset(name1, 0, 50);
    memset(name2, 0, 50);

    send(p1_sock,
        "\033[2J\033[HEnter your name: ",
        30,
        0);

    recv(p1_sock, name1, 49, 0);

    send(p2_sock,
        "\033[2J\033[HEnter your name: ",
        30,
        0);

    recv(p2_sock, name2, 49, 0);

    name1[strcspn(name1, "\n")] = 0;
    name2[strcspn(name2, "\n")] = 0;
}

void wait_ready(int p1_sock, int p2_sock) {

    intro_screen(p1_sock);
    intro_screen(p2_sock);

    char r;

    recv(p1_sock, &r, 1, 0);
    recv(p2_sock, &r, 1, 0);
}

char get_action(int sock, Player p) {

    char action;

    while (1) {

        if (recv(sock, &action, 1, 0) <= 0)
            return 'Q';

        action = toupper(action);

        if (
            (action == 'W' && p.ep >= 2) ||
            (action == 'S' && p.ep >= 2) ||
            (action == 'F' && p.ep >= 4) ||
            (action == 'X') ||
            (action == 'C' && p.ep >= 5 && !p.heal_used)
        ) {
            return action;
        }

        send(sock,
            "Invalid move!\n",
            15,
            0);
    }
}

void victory_screen(
    int sock,
    int victory,
    char *winner,
    char *name1,
    char *name2,
    Player p1,
    Player p2,
    Stats s1,
    Stats s2
) {
    char buffer[8192];

    snprintf(buffer, sizeof(buffer),

        "\033[2J\033[H"

        "%s"
        "══════════════════════════════════════════════════\n"
        "                 %s\n"
        "══════════════════════════════════════════════════\n"
        RESET

        "\n"

        YELLOW BOLD
        "🏆 MATCH WINNER 🏆\n"
        RESET
        "Winner: " CYAN BOLD "%s\n\n" RESET

        CYAN BOLD
        "══════════════════════════════════════════════════\n"
        "                  FINAL SCORE\n"
        "══════════════════════════════════════════════════\n"
        RESET
        CYAN
        "%s [%d]  VS  %s [%d]\n"
        RESET
        CYAN BOLD
        "══════════════════════════════════════════════════\n\n"
        RESET

     

"══════════════════════════════════════════════════\n"
"                 MATCH SUMMARY\n"
"══════════════════════════════════════════════════\n\n"

RESET

"%s STATS\n"
RESET

"──────────────────────────────────────────────────\n"

"⚔  Attacks Used : %d\n"
"🛡  Defends Used : %d\n"
"💥 Special Used : %d\n"
"💚 Heals Used   : %d\n"
"⚡ Rests Used   : %d\n"
"🎯 Damage Dealt : %d\n"

RESET

"\n──────────────────────────────────────────────────\n\n"
RESET

"%s STATS\n"
RESET

"──────────────────────────────────────────────────\n"

"⚔  Attacks Used : %d\n"
"🛡  Defends Used : %d\n"
"💥 Special Used : %d\n"
"💚 Heals Used   : %d\n"
"⚡ Rests Used   : %d\n"
"🎯 Damage Dealt : %d\n"

RESET

"\n══════════════════════════════════════════════════\n"
RESET

        CYAN BOLD
        "\n══════════════════════════════════════════════════\n"
        "                 COMMAND GUIDE\n"
        "══════════════════════════════════════════════════\n"
        RESET
        "⚔  [W] ATTACK   - Normal Damage\n"
        "🛡  [S] DEFEND   - Reduce Damage\n"
        "💥 [F] SPECIAL  - Strong Attack\n"
        "💚 [C] HEAL     - Recover HP\n"
        "⚡ [X] REST     - Recover EP\n"
        CYAN BOLD
        "══════════════════════════════════════════════════\n"
        RESET

        YELLOW BOLD
        "\n══════════════════════════════════════════════════\n"
        "                 PLAYER OPTIONS\n"
        "══════════════════════════════════════════════════\n"
        RESET
        "🔁 [R] REMATCH     - Play Again\n"
        "👤 [N] NEW PLAYER  - Change Players\n"
        "🚪 [Q] QUIT        - Exit Game\n"
        YELLOW BOLD
        "══════════════════════════════════════════════════\n\n"
        RESET

        "> ",

        victory ? GREEN BOLD : RED BOLD,
        victory ? "🏆 VICTORY! 🏆" : "☠ DEFEAT ☠",

        winner,

        name1,
        p1.wins,
        name2,
        p2.wins,

        name1,
        s1.attacks,
        s1.defends,
        s1.specials,
        s1.heals,
        s1.rests,
        s1.damage_dealt,

        name2,
        s2.attacks,
        s2.defends,
        s2.specials,
        s2.heals,
        s2.rests,
        s2.damage_dealt
    );

    send(sock, buffer, strlen(buffer), 0);
}
int main() {

    int server_fd;
    int p1_sock;
    int p2_sock;

    struct sockaddr_in address;

    int addrlen = sizeof(address);

    srand(time(NULL));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(
        server_fd,
        (struct sockaddr *)&address,
        sizeof(address)
    );

    listen(server_fd, 2);

    printf("Server running on port %d\n", PORT);
    printf("Waiting for players...\n");

    p1_sock = accept(
        server_fd,
        (struct sockaddr *)&address,
        (socklen_t*)&addrlen
    );

    printf("Player 1 connected\n");

    p2_sock = accept(
        server_fd,
        (struct sockaddr *)&address,
        (socklen_t*)&addrlen
    );

    printf("Player 2 connected\n");

    char name1[50];
    char name2[50];

    ask_names(p1_sock, p2_sock, name1, name2);

    wait_ready(p1_sock, p2_sock);

    while (1) {

        Player p1 = {0};
        Player p2 = {0};

        Stats s1 = {0};
        Stats s2 = {0};

        p1.wins = 0;
        p2.wins = 0;

        int roundNum = 1;

        while (p1.wins < 3 && p2.wins < 3) {

            p1.hp = MAX_HP;
            p2.hp = MAX_HP;

            p1.ep = MAX_EP;
            p2.ep = MAX_EP;

            p1.heal_used = 0;
            p2.heal_used = 0;

            p1.defending = 0;
            p2.defending = 0;

            char msg[256] = "New Round Begins!";

            char logs[3][128] = {"", "", ""};

            int currentTurn = rand() % 2;
            
            char tossmsg[512];

if (currentTurn == 0) {
    sprintf(tossmsg,
        "\033[2J\033[H"
        CYAN BOLD
        "╔════════════════════════════════════════════╗\n"
        "║                 COIN TOSS                  ║\n"
        "╚════════════════════════════════════════════╝\n"
        RESET
        "\nROUND %d\n\n"
        "The coin has been tossed...\n\n"
        GREEN BOLD "%s goes first!\n" RESET,
        roundNum,
        name1
    );
} else {
    sprintf(tossmsg,
        "\033[2J\033[H"
        CYAN BOLD
        "╔════════════════════════════════════════════╗\n"
        "║                 COIN TOSS                  ║\n"
        "╚════════════════════════════════════════════╝\n"
        RESET
        "\nROUND %d\n\n"
        "The coin has been tossed...\n\n"
        GREEN BOLD "%s goes first!\n" RESET,
        roundNum,
        name2
    );
}

send(p1_sock, tossmsg, strlen(tossmsg), 0);
send(p2_sock, tossmsg, strlen(tossmsg), 0);

sleep(2);

            while (p1.hp > 0 && p2.hp > 0) {

                Player *atk;
                Player *def;

                Stats *stats;

                int atk_sock;
                int def_sock;

                char *atk_name;
                char *def_name;

                if (currentTurn == 0) {

                    atk = &p1;
                    def = &p2;

                    stats = &s1;

                    atk_sock = p1_sock;
                    def_sock = p2_sock;

                    atk_name = name1;
                    def_name = name2;
                }

                else {

                    atk = &p2;
                    def = &p1;

                    stats = &s2;

                    atk_sock = p2_sock;
                    def_sock = p1_sock;

                    atk_name = name2;
                    def_name = name1;
                }

                send_screen(
                    atk_sock,
                    *atk,
                    *def,
                    1,
                    msg,
                    atk_name,
                    def_name,
                    roundNum,
                    logs
                );

                send_screen(
                    def_sock,
                    *def,
                    *atk,
                    0,
                    msg,
                    def_name,
                    atk_name,
                    roundNum,
                    logs
                );

                char action = get_action(atk_sock, *atk);

                if (action == 'W') {

                    int dmg = def->defending ? 2 : 3;

                    def->hp -= dmg;

                    atk->ep -= 2;

                    stats->attacks++;
                    stats->damage_dealt += dmg;

                    sprintf(
                    msg,
                  "⚔️ SLASH! %s struck %s for %d damage!",
                  atk_name,
                  def_name,
                  dmg
);

                    add_log(logs, msg);

                    def->defending = 0;
                }

                else if (action == 'S') {

                    atk->defending = 1;

                    atk->ep -= 2;

                    stats->defends++;

                    sprintf(
    msg,
    "🛡️ SHIELD UP! %s is guarding against the next attack!",
    atk_name
);
}
                else if (action == 'F') {

                    int dmg = def->defending ? 3 : 5;

                    def->hp -= dmg;

                    atk->ep -= 4;

                    stats->specials++;
                    stats->damage_dealt += dmg;

                    sprintf(
    msg,
    "💥🔥 SPECIAL STRIKE! %s blasted %s with a powerful hit!",
    atk_name,
    def_name
);

                    add_log(logs, msg);

                    def->defending = 0;
                }

                else if (action == 'C') {

                    atk->hp += 4;

                    if (atk->hp > MAX_HP)
                        atk->hp = MAX_HP;

                    atk->ep -= 5;

                    atk->heal_used = 1;

                    stats->heals++;

                    sprintf(
    msg,
    "💚✨ HEALING LIGHT! %s recovered 4 HP!",
    atk_name
);

                    add_log(logs, msg);
                }

                else if (action == 'X') {

                    atk->ep++;

                    if (atk->ep > MAX_EP)
                        atk->ep = MAX_EP;

                    stats->rests++;

                    sprintf(
    msg,
    "⚡🔋 ENERGY CHARGE! %s recovered 1 EP!",
    atk_name
);

                    add_log(logs, msg);
                }

                currentTurn = !currentTurn;
            }

            if (p1.hp <= 0)
                p2.wins++;

            else
                p1.wins++;

            roundNum++;
        }

        char *winner =
            p1.wins == 3 ? name1 : name2;

        victory_screen(
            p1_sock,
            p1.wins == 3,
            winner,
            name1,
            name2,
            p1,
            p2,
            s1,
            s2
        );

        victory_screen(
            p2_sock,
            p2.wins == 3,
            winner,
            name1,
            name2,
            p1,
            p2,
            s1,
            s2
        );

        char c1;
        char c2;

        recv(p1_sock, &c1, 1, 0);
        recv(p2_sock, &c2, 1, 0);

        c1 = toupper(c1);
        c2 = toupper(c2);

        if (c1 == 'Q' || c2 == 'Q')
            break;

        if (c1 == 'N' || c2 == 'N') {

            ask_names(
                p1_sock,
                p2_sock,
                name1,
                name2
            );

            wait_ready(
                p1_sock,
                p2_sock
            );
        }
    }

    close(p1_sock);
    close(p2_sock);
    close(server_fd);

    return 0;
}
