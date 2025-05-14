#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() {
    const char* broker = "34.70.133.53";  // Your MQTT broker IP
    const char* subTopic = "ttt/request";
    const char* pubTopic = "ttt/input";

    // Seed randomness
    srand(time(NULL));

    // Build the command to listen to move requests
    char subCmd[256];
    snprintf(subCmd, sizeof(subCmd),
             "mosquitto_sub -h %s -t %s", broker, subTopic);

    // Open a pipe to read messages
    FILE *fp = popen(subCmd, "r");
    if (!fp) {
        perror("Failed to subscribe to MQTT topic");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Strip newline
        line[strcspn(line, "\r\n")] = 0;

        // Only respond if it's X's turn
        if (strcmp(line, "X") == 0) {
            int row = rand() % 3;
            int col = rand() % 3;

            char json[64];
            snprintf(json, sizeof(json), "{\"row\":%d,\"col\":%d}", row, col);

            char pubCmd[256];
            snprintf(pubCmd, sizeof(pubCmd),
                     "mosquitto_pub -h %s -t %s -m '%s'", broker, pubTopic, json);

            printf("C Bot move (X): %s\n", json);
            system(pubCmd);
        }
    }

    pclose(fp);
    return 0;
}
