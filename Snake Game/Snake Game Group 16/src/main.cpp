#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <EEPROM.h>
#include <stdlib.h> // For random numbers
#include <time.h>   

// Pin definitions
#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8
#define JOY_X_PIN  A0
#define JOY_Y_PIN  A1
#define BUZZER_PIN 3

const int buttonPin = 7;  // Pin where the middle button is connected

// Initialize tft
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const int Width = 32;
const int Height = 24;

// Game variables
int score = 0;
int level = 1;
int highScore = 0;
bool gameOver = false;

// Direction enumeration
enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

// Structure for Barrier Segment
struct BarrierSegment {
  int x, y;
};


const int MAX_BARRIERS = 100;
BarrierSegment barrier_1[MAX_BARRIERS];
//BarrierSegment barrier_2[MAX_BARRIERS];

void genBarrier(){
  for(int i=0; i<Width; i++){
    barrier_1[i].x = i;
    barrier_1[i].y = 0;
  }
  for(int i=Width; i<2*Width; i++){
    barrier_1[i].x = i-Width;
    barrier_1[i].y = Height-3;
  }
  /*for(int i=0; i<Height; i++){
    barrier_2[i].x = 0;
    barrier_2[i].y = i;
  }
  for(int i=Height; i<2*Height; i++){
    barrier_2[i].x = Width-1;
    barrier_2[i].y = i-Height;
  }*/
}

// Array to store active barriers

BarrierSegment activeBarriers[MAX_BARRIERS];
int activeBarrierCount = 0;

// Flag to ensure barrier is drawn only once
bool barrierDrawn = false;

// Current direction variable
Direction currentDirection = RIGHT; // Initial direction

// Structure for snake segment
struct SnakeSegment {
  int x, y;
};

// Array to store snake segments
SnakeSegment snake[50];
int snakeLength = 3;

// Food Type Enumeration and Structure
enum FoodType {
  GOOD,
  High_M
};

struct Food {
  int x, y;
  FoodType type;
};
//
// Current Food Instance
Food currentFood;
bool foodVisible = false;
unsigned long foodTimer = 0;
const unsigned long foodTimeLimit = 5000; // Food disappears after 5 seconds

// Countdown Timer Variables
unsigned long countdownStartTime = 0; // Time when the countdown started
const int COUNTDOWN_TIME = 5; // Countdown time in seconds
bool countdownActive = false; // Flag to track if countdown is active

bool gameStart = false;

//void levelUp(int level);
// Function to read joystick input and update direction
void updateDirection() {
  int xValue = analogRead(JOY_X_PIN);
  int yValue = analogRead(JOY_Y_PIN);

  if (xValue > 600 && currentDirection != RIGHT) {
    currentDirection = LEFT;
  }
  else if (xValue < 400 && currentDirection != LEFT) {
    currentDirection = RIGHT;
  }
  else if (yValue > 600 && currentDirection != DOWN) {
    currentDirection = UP;
  }
  else if (yValue < 400 && currentDirection != UP) {
    currentDirection = DOWN;
  }
}

// Function to erase the food from its current position on the tft
void eraseFood() {
  if (foodVisible) {
    tft.fillRect(currentFood.x * 10, currentFood.y * 10, 10, 10, ILI9341_BLACK);
    foodVisible = false; // Update the flag to indicate food is no longer visible
  }
}

// Function to generate new food coordinates and type
void generateFood() {
  eraseFood(); // Ensure the old food is removed from the tft

  bool collision;
  do {
    collision = false;
    currentFood.x = random(0, Width);
    currentFood.y = random(0, Height-2);

    // Check against snake
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == currentFood.x && snake[i].y == currentFood.y) {
        collision = true;
        break;
      }
    }

    // Check against barriers
    if (!collision) {
      for (int i = 0; i < activeBarrierCount; i++) {
        if (activeBarriers[i].x == currentFood.x && activeBarriers[i].y == currentFood.y) {
          collision = true;
          break;
        }
      }
    }

  } while (collision);

  // Set food type based on level
  int val = random(0,100);
  if (level >= 4){
    if(val%2==0){
      currentFood.type = High_M;
    } 
    else {
    currentFood.type = GOOD;
    }
  }

  foodVisible = true; // Set food as visible
  foodTimer = millis(); // Reset the food timer

  // Initialize countdown timer only if level is 3 or higher
  if (level >= 3) {
    countdownStartTime = millis(); // Start countdown
    countdownActive = true; // Activate countdown
  }
  else {
    countdownActive = false; // Ensure countdown is inactive for levels below 3
  }
}

// Function to draw the snake
void drawSnake() {
  for (int i = 0; i < snakeLength; i++) {
    tft.fillRect(snake[i].x * 10, snake[i].y * 10, 10, 10, ILI9341_GREEN);
  }
}

// Function to erase the last position of the snake segment
void eraseSnakeSegment(int x, int y) {
  tft.fillRect(x * 10, y * 10, 10, 10, ILI9341_BLACK);
}

// Function to draw food
void drawFood() {
  if (foodVisible) {
    if (currentFood.type == GOOD) {
      tft.fillRect(currentFood.x * 10, currentFood.y * 10, 10, 10, ILI9341_GREEN);
    } else {
      tft.fillRect(currentFood.x * 10, currentFood.y * 10, 10, 10, ILI9341_RED);
    }
  }
}

// Function to move the snake based on current direction
void moveSnake() {
  int dirX = 0, dirY = 0;

  // Set direction deltas based on current direction
  switch (currentDirection) {
    case UP:
      dirY = -1;
      break;
    case DOWN:
      dirY = 1;
      break;
    case LEFT:
      dirX = -1;
      break;
    case RIGHT:
      dirX = 1;
      break;
  }

  // Erase the last segment of the snake
  eraseSnakeSegment(snake[snakeLength - 1].x, snake[snakeLength - 1].y);

  // Move the snake body
  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  // Move the head
  snake[0].x += dirX;
  snake[0].y += dirY;

  // Check for collisions with itself
  for (int i = 1; i < snakeLength; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      gameOver = true; // Game over
    }
  }

  // Check for collisions with barriers
  for (int i = 0; i < activeBarrierCount; i++) {
    if (snake[0].x == activeBarriers[i].x && snake[0].y == activeBarriers[i].y) {
      gameOver = true; // Game over
      break;
    }
  }

  // Wrap around the screen edges
  if (snake[0].x < 0) snake[0].x = Width - 1;
  if (snake[0].x >= Width) snake[0].x = 0;
  if (snake[0].y < 0) snake[0].y = Height - 3;
  if (snake[0].y >= Height-2) snake[0].y = 0;
}

// Function to play sound
void playSound(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
}
// Function to check if the snake has eaten food
void displayStatus();

void checkFood() {
  if (foodVisible && snake[0].x == currentFood.x && snake[0].y == currentFood.y) {
    eraseFood(); // Remove the food from the tft

    if (currentFood.type == GOOD) {
      snakeLength++;
      score++;
      playSound(1000, 200); // Play a single mark sound
    }
    else {
      snakeLength++;
      score+=5;
      playSound(2000, 200); // Play a High mark sound
    }
     // Level up every 2 points
    if (score % 2 == 0){ 
      level++;
      //levelUp(level); // Handle level progression
    }
    displayStatus(); // Update score and level
    countdownActive = false; // Stop countdown since food is eaten
    generateFood(); // Generate new food
  }
}

// Function to show the game menu
void showMenu() {
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);
  tft.println("1. Start New Game");
  tft.println("2. View High Score");
}

// Function to save high score
void saveHighScore() {
  if (score > highScore) {
    EEPROM.write(0, score);  // Save score to EEPROM
    highScore = score;
  }
}

// Function to load high score
void loadHighScore() {
  highScore = EEPROM.read(0);  // Load score from EEPROM
}

// Function to play game over sound
void gameOverSound() {
  playSound(200, 500); // Example sound on game over
}

// Start a new game
void newGame(){
  gameOver = false;
  tft.fillScreen(ILI9341_YELLOW);
  tft.setCursor(50,Height*5-20);
  tft.setTextColor(ILI9341_BLUE);
  tft.setTextSize(5);
  tft.println("New Game");
  tft.setCursor(0,Height*8);
  tft.setTextColor(ILI9341_DARKGREY);
  tft.setTextSize(2);
  tft.println("Press Middle Button" );
  tft.println("to Start");
  if(!gameStart){
    while(digitalRead(buttonPin) == HIGH);
    gameStart = true;
  }
  tft.fillScreen(ILI9341_BLACK);
}
// Function to draw barriers

void printGameOver(){
  tft.fillScreen(ILI9341_RED);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Game Over!");
  tft.print("Score: ");
  tft.println(score);
  tft.print("High Score: ");
  tft.println(highScore);
}
// Setup function
void setup() {
  tft.begin();
  tft.setRotation(3);  // Set rotation based on your need
  tft.fillScreen(ILI9341_BLACK);
  
  pinMode(BUZZER_PIN, OUTPUT);
  loadHighScore(); // Load high score at the start

  pinMode(buttonPin, INPUT_PULLUP);
  gameStart = false;
  genBarrier();

  randomSeed(analogRead(A2)); // Seed random number generator
  newGame();
  generateFood(); // Generate initial food

  // Initialize snake starting position
  snake[0].x = Width/2; // Center horizontally
  snake[0].y = Height/2; // Center vertically
  for (int i = 1; i < snakeLength; i++) {
    snake[i].x = snake[0].x - i;
    snake[i].y = snake[0].y;
  }
  drawSnake();
  displayStatus(); // tft initial score and level
}
void displayStatus() {
  tft.fillRect(0, Height*10-15, 320, 20, ILI9341_BLUE); // Clear previous score and level
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  // tft Score
  tft.setCursor(0, Height*10-15);
  tft.print("Score: ");
  tft.print(score);
  // tft Level
  tft.setCursor(120, Height*10-15);
  tft.print("Level: ");
  tft.print(level);
}

// Main loop
void loop() {

  if (gameOver) {
    gameOverSound();
    printGameOver();
    saveHighScore(); // Save high score if needed
    while (true); // Stop the loop here
  }
  updateDirection(); // Update direction based on joystick input
  moveSnake(); // Move snake based on current direction

  if (level >= 2) {
    // Add barrier segments for the number "8"
    for (int i = 0; i < 2*Width; i++) {
      if (activeBarrierCount < MAX_BARRIERS) {
        activeBarriers[activeBarrierCount++] = barrier_1[i];
        tft.fillRect(barrier_1[i].x * 10, barrier_1[i].y * 10, 10, 10, ILI9341_RED);
      }
    }
  }
 /* if (level >= 3) {
    // Add barrier segments for the number "8"
    for (int i = 0; i < 2*Height; i++) {
      if (activeBarrierCount < MAX_BARRIERS) {
        activeBarriers[activeBarrierCount++] = barrier_2[i];
        tft.fillRect(barrier_2[i].x * 10, barrier_2[i].y * 10, 10, 10, ILI9341_RED);
      }
    }
  }*/
  // Handle food visibility and timeout based on level
  if (level >= 3) {
    if (foodVisible) {
      unsigned long currentTime = millis();
      unsigned long elapsedTime = currentTime - foodTimer;
      
      // Update the countdown tft
      int remainingTime = COUNTDOWN_TIME - (elapsedTime / 1000);
      if (remainingTime < 0) remainingTime = 0;

      // tft the countdown timer
      tft.fillRect(240, Height*10-15, 80, 20, ILI9341_BLUE); // Clear previous score and level
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_WHITE);
      // tft Score
      tft.setCursor(220, Height*10-10);
      tft.print("Time Left: ");
      tft.print(remainingTime);
      tft.print("s");

      // Check if countdown has reached zero
      if (elapsedTime >= foodTimeLimit) {
        generateFood(); // Generate new food and reset timer
      }
    }
  }

  drawSnake();
  drawFood();
  checkFood(); // Check if food is eaten
  if(level == 5) delay(100); // Control the game speed
  else{
    delay(200);
  }
}
