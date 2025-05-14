#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// WiFi and MQTT Settings
const char* ssid = "Hofalo";
const char* password = "aliciacia";
const char* mqtt_server = "34.70.133.53";  // e.g., "34.60.120.133"

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Game variables
char board[3][3];
char currentPlayer = 'X';
bool moveReceived = false;
int moveRow, moveCol;
int gameMode = 0;
bool gameInProgress = false;

// Score tracking
int xWins = 0, oWins = 0, draws = 0, gamesPlayed = 0;

void setupWiFi() {
  delay(10);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';  // Null-terminate

  if (String(topic) == "esp32/tictac") {
    gameMode = atoi((char*)payload);
    gameInProgress = true;
  }
  else if (String(topic) == "ttt/input") {
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("Failed to parse move: ");
      Serial.println(error.c_str());
      return;
    }

    moveRow = doc["row"];
    moveCol = doc["col"];
    moveReceived = true;

    Serial.printf("Move received: row=%d, col=%d\n", moveRow, moveCol);
  }
}


void setupMQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      client.subscribe("esp32/tictac");
      client.subscribe("ttt/input");
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void resetBoard() {
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      board[i][j] = ' ';
}

void printBoard() {
  Serial.println();
  for (int i = 0; i < 3; i++) {
    Serial.printf(" %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
    if (i < 2) Serial.println("---+---+---");
  }
  publishBoard();  // Also publish board to MQTT
}

void publishBoard() {
  char msg[128];
  snprintf(msg, sizeof(msg),
           "%c|%c|%c\n%c|%c|%c\n%c|%c|%c",
           board[0][0], board[0][1], board[0][2],
           board[1][0], board[1][1], board[1][2],
           board[2][0], board[2][1], board[2][2]);
  client.publish("ttt/state", msg);
}

bool isValidMove(int r, int c) {
  return r >= 0 && r < 3 && c >= 0 && c < 3 && board[r][c] == ' ';
}

char checkWinner() {
  for (int i = 0; i < 3; ++i) {
    if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
      return board[i][0];
    if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
      return board[0][i];
  }
  if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
    return board[0][0];
  if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
    return board[0][2];
  return ' ';
}

bool isDraw() {
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (board[i][j] == ' ') return false;
  return checkWinner() == ' ';
}

void playMove(char player) {
  while (true) {
    moveReceived = false;

    // For bot moves (Bash or C), trigger request via MQTT
    if ((gameMode == 1 && player == 'O') || gameMode == 3) {
      client.publish("ttt/request", String(player).c_str());
      while (!moveReceived) {
        client.loop();
      }
    } else {
      // Human moves are expected via MQTT too
      Serial.printf("Waiting for human (%c) move via MQTT...\n", player);
      while (!moveReceived) {
        client.loop();
      }
    }

    if (isValidMove(moveRow, moveCol)) {
      board[moveRow][moveCol] = player;
      break;  // Exit the loop once a valid move is placed
    } else {
      Serial.printf("Invalid move (%d,%d) received from %c — requesting again...\n", moveRow, moveCol, player);
      // Loop will retry the request
    }
  }
}

void updateLCDStats() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("X:%d O:%d", xWins, oWins);
  lcd.setCursor(0, 1);
  lcd.printf("D:%d G:%d", draws, gamesPlayed);
}

void loopGame() {
  resetBoard();
  currentPlayer = 'X';
  printBoard();

  while (true) {
    playMove(currentPlayer);
    printBoard();
    char winner = checkWinner();
    if (winner != ' ') {
      Serial.printf("Player %c wins!\n", winner);
      if (winner == 'X') xWins++;
      else if (winner == 'O') oWins++;
      break;
    } else if (isDraw()) {
      Serial.println("Draw!");
      draws++;
      break;
    }
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
  }

  gamesPlayed++;
  updateLCDStats();
  Serial.printf("Games Played: %d | X: %d | O: %d | D: %d\n\n", gamesPlayed, xWins, oWins, draws);

  gameInProgress = false;
}

void setup() {
  Serial.begin(115200);
  delay(100); // Let serial port initialize

  // Initialize I2C before LCD
  Wire.begin(14, 13);  // SDA = GPIO 14, SCL = GPIO 13 (adjust if needed)

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Tic-Tac-Toe Ready");

  setupWiFi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();  // Ensure MQTT connection
}


void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (gameInProgress) {
    Serial.printf("Starting game mode %d...\n", gameMode);
    loopGame();
  }
}

