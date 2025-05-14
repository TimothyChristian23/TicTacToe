#include <stdio.h>
#include <stdlib.h>

const char* broker = "34.70.133.53";
const char* topic = "esp32/tictac";

int main() {
    char fullCommand[256];

    while (1) {
        printf("\n=== ESP32 TicTacToe Menu ===\n");
        printf("1: 1 Player (You vs Bash Bot)\n");
        printf("2: 2 Player (You vs Friend)\n");
        printf("3: AUTO Mode (C Bot vs Bash Bot)\n");
        printf("4: Exit\n");
        printf("Select a mode: ");

        int option;
        scanf("%d", &option);

        if (option >= 1 && option <= 3) {
            snprintf(fullCommand, sizeof(fullCommand), 
                     "mosquitto_pub -h %s -t %s -m \"%d\"", 
                     broker, topic, option);
            printf("Publishing mode %d...\n", option);
            system(fullCommand);

            if (option == 3) {
                printf("Starting C Bot locally and Bash Bot remotely...\n");

                // Start C bot locally (Player X)
                system("start c_bot.exe");


                // Start Bash bot remotely on GCP (Player O)
                system("ssh timothy_dave11@34.70.133.53 'bash /home/timothy.dave11/bashscripts/player1.sh' &");
            }
        } else if (option == 4) {
            printf("Exiting menu.\n");
            break;
        } else {
            printf("Invalid input. Try 1–4.\n");
        }
    }

    return 0;
}
