#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

#define PORT 8080

int main() {

    int sock = 0;
    struct sockaddr_in serv_addr;

    char buffer[4096];
    char input[100];
    char server_ip[50];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        printf("Socket creation failed.\n");
        return -1;
    }

    printf("Enter server IP: ");
    scanf("%s", server_ip);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid Address\n");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return -1;
    }

    printf("Connected to server!\n");

    while (1) {

        int valread = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (valread <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        buffer[valread] = '\0';

        printf("%s", buffer);

        // ENTER NAME
        if (strstr(buffer, "Enter your name")) {

            getchar();

            fgets(input, sizeof(input), stdin);

            send(sock, input, strlen(input), 0);
        }

        // READY / MENU
        else if (
            strstr(buffer, "Press [R] when ready") ||
            strstr(buffer, "NEXT ROUND") ||
            strstr(buffer, "PLAYER OPTIONS")
        ) {

            char choice;

            scanf(" %c", &choice);

            choice = toupper(choice);

            send(sock, &choice, 1, 0);
        }

        // GAME TURN
        else if (
            strstr(buffer, "YOUR TURN") ||
            strstr(buffer, "Invalid")
        ) {

            char action;

            scanf(" %c", &action);

            action = toupper(action);

            send(sock, &action, 1, 0);
        }
    }

    close(sock);

    return 0;
}
