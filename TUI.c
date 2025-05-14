#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* broker = "34.70.133.53"; // Replace with your broker IP
const char* stateTopic = "ttt/state";
const char* inputTopic = "ttt/input";

void startViewer() {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "mosquitto_sub -h %s -t %s", broker, stateTopic);

    printf("\nListening to game state from ESP32...\n");
    printf("Press Ctrl+C to quit.\n\n");

    system(cmd);
}

void sendMove() {
    int row, col;
    printf("\nEnter your move (row col): ");
    scanf("%d %d", &row, &col);

    // Step 1: Write JSON to a temp file
    FILE *json = fopen("move.json", "w");
    if (!json) {
        perror("Failed to create move.json");
        return;
    }
    fprintf(json, "{\"row\":%d,\"col\":%d}\n", row, col);
    fclose(json);

    // Step 2: Write a batch file that sends the move
    FILE *bat = fopen("send_move.bat", "w");
    if (!bat) {
        perror("Failed to create send_move.bat");
        return;
    }
    fprintf(bat, "mosquitto_pub -h %s -t %s -f move.json\n", broker, inputTopic);
    fclose(bat);

    // Step 3: Run the batch file
    system("send_move.bat");

    printf("Move sent: row=%d, col=%d\n", row, col);
}



int main() {
    while (1) {
        printf("\n=== MQTT Tic-Tac-Toe TUI ===\n");
        printf("1: Watch Game\n");
        printf("2: Send Move (X or O)\n");
        printf("3: Exit\n");
        printf("Select an option: ");

        int choice;
        scanf("%d", &choice);
        if (choice == 1) startViewer();
        else if (choice == 2) sendMove();
        else if (choice == 3) break;
        else printf("Invalid choice. Try again.\n");
    }

    return 0;
}
