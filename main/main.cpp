#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include "networking.h"

#define PATH "assets/"

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int bottom;
    int velocity;
    int jumpcount;
    bool jumping;
    bool walking;
} Character;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int velocity;
    bool jumping;
    int jumpcount;
    int walking;
    int bottom;
} Enemy;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int velocity;
} Knife;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int imageIndex;
} Platform;

typedef struct {
    int score;
    int highscore;
    float timer;
} UI;


Character character;
Enemy enemy;
bool isConnected = false; 
bool inMultiplayer = false;
bool inMultiplayerSetup = false;
bool inCharacterChoice = false;
bool inShowIP = false;
bool inJoinGame = false;
Knife* knives = new Knife[9999]; 
int knife_count = 0;
float last_knife_time = 0.0f;
Sound knifeSFX;
Texture2D knifeImage;


int onPlatform(Character character, Platform platforms[], int count) {
    Rectangle character_rec = { (float)character.x, (float)character.y, (float)character.width, (float)character.height };
    Rectangle ground_rec = { 0.0f, 900.0f, 1920.0f, (float)character.height };
    bool gameover = false;
    for (int i = 0; i < count; i++) {
        Rectangle platform_rec = { (float)platforms[i].x, (float)platforms[i].y, (float)(platforms[i].width - 80), (float)platforms[i].height };
        if (CheckCollisionRecs(character_rec, ground_rec)) {
            gameover = true;
            break;
        }
        if (CheckCollisionRecs(character_rec, platform_rec)) {
            return i; 
        }

    }
    return -1; 
}


int onPlatform(Enemy enemy, Platform platforms[], int count) {
    Rectangle enemy_rec = { (float)enemy.x, (float)enemy.y, (float)enemy.width, (float)enemy.height };
    for (int i = 0; i < count; i++) {
        Rectangle platform_rec = { (float)platforms[i].x, (float)platforms[i].y, (float)(platforms[i].width - 80), (float)platforms[i].height };
        if (CheckCollisionRecs(enemy_rec, platform_rec)) {
            return i; 
        }
    }
    return -1;
}

void ResetGame(Character& character, Enemy& enemy, UI& ui, int window_height) {
    character.x = 1100;
    character.y = window_height / 2;
    character.velocity = 0;
    character.jumping = false;
    character.walking = false;
    character.jumpcount = 0;

    enemy.x = 100;
    enemy.y = window_height / 2;
    enemy.velocity = 0;
    enemy.jumping = false;
    enemy.jumpcount = 0;

    ui.score = 0;
    ui.timer = 0.0f;

    knife_count = 0; 
    last_knife_time = 0.0f; 
}

void loadImagesAsync(std::vector<Image>& images, int start, int end, std::mutex& mtx) {
    for (int i = start; i <= end; i++) {
        std::stringstream ss;
        ss << PATH "frame_" << std::setw(5) << std::setfill('0') << i << ".png";
        std::string fileName = ss.str();

        if (FileExists(fileName.c_str())) {
            Image img = LoadImage(fileName.c_str());

            // Lock the mutex to safely insert the image in the correct order
            std::lock_guard<std::mutex> lock(mtx);
            images[i] = img;  // Place image in the correct position
        }
        else {
            std::cout << "File not found: " << fileName << std::endl;
        }
    }
}


std::string playerName;   // Stores the player's chosen character ("Ahmed" or "Samra")
std::string serverIP; // Stores the IP address of the host

void onMessageReceived(const std::string& message) {
    if (message == "Connection established.") {
        isConnected = true;
        if (inShowIP) {
            inShowIP = false; 
            inMultiplayer = true; 
        }
    }
    else if (message == "Ahmed" || message == "Samra") {
        playerName = (message == "Ahmed") ? "Samra" : "Ahmed";
    }
    else {
        if (message == "AHMED_JUMP") {
            if (playerName == "Samra") { 
                character.velocity = -30;
                character.jumping = true;
                character.jumpcount++;
            }
        }
        else if (message == "AHMED_DOWN") {
            if (playerName == "Samra") {
                character.velocity = 20;
                character.jumping = false;
            }
        }
        else if (message == "AHMED_RIGHT") {
            if (playerName == "Samra") {
                character.x += 12;
                character.walking = true;
            }
        }
        else if (message == "AHMED_LEFT") {
            if (playerName == "Samra") {
                character.x -= 12;
                character.walking = true;
            }
        }
        else if (message == "SAMRA_JUMP") {
            if (playerName == "Ahmed") { 
                enemy.velocity = -30;
                enemy.jumping = true;
                enemy.jumpcount++;
            }
        }
        else if (message == "SAMRA_DOWN") {
            if (playerName == "Ahmed") {
                enemy.velocity = 20;
                enemy.jumping = false;
            }
        }
        else if (message == "SAMRA_RIGHT") {
            if (playerName == "Ahmed") {
                enemy.x += 12;
                enemy.walking = true;
            }
        }
        else if (message == "SAMRA_LEFT") {
            if (playerName == "Ahmed") {
                enemy.x -= 12;
                enemy.walking = true;
            }
        } 
        else if (message == "SAMRA_KNIFE") {

            knives[knife_count].x = enemy.x + enemy.width;
            knives[knife_count].y = enemy.y + enemy.height / 2;
            knives[knife_count].width = knifeImage.width;
            knives[knife_count].height = knifeImage.height;
            knives[knife_count].velocity = 20; 
            knife_count++;
            last_knife_time = GetTime();


            PlaySound(knifeSFX);
        }
    }
}

int main() {
    // srand((unsigned int)time(NULL));
    srand(1545153321231487675);

    int window_width = 1920;
    int window_height = 800;
    int gravity = 2;
    int jumpcount = 0;
    bool gameover = false;
    bool inMenu = true;
    bool inCountdown = false;
    bool inGame = false;
    bool inSound = false;
    bool inDevs = false;
    bool inHowToPlay = false;
    bool isMuted = false;
    bool isIntro = true;
    int howToPlayImageIndex = 0; 
    float prevVolume = 1.0f; 
    float soundVolume = 1.0f; 
    int gameMode = 0;
    float countdownTime = 3.0f; 

    InitWindow(window_width, window_height, "El Shabah");
    InitAudioDevice();

    // Game Texture
    Texture2D theEndImage = LoadTexture(PATH "theEndImage.png");
    Texture2D background = LoadTexture(PATH "background.png");
	knifeImage = LoadTexture(PATH "knife.png");
    // Player Texture
    Texture2D ahmedImage = LoadTexture(PATH "ahmedstand.png");
    Texture2D ahmedjump = LoadTexture(PATH "ahmedjump.png");
    Texture2D ahmedwalk = LoadTexture(PATH "ahmedwalk.png");
    // Enemy Texture
    Texture2D samraImage = LoadTexture(PATH "samrastand.png");
    Texture2D samrajump = LoadTexture(PATH "samrajump.png");
    Texture2D samrawalk = LoadTexture(PATH "samrawalk.png");
    // Terrain Texture
    Image building1 = LoadImage(PATH "building1.png");
    Image building2 = LoadImage(PATH "building2.png");
    Image building3 = LoadImage(PATH "building3.png");
    Image building4 = LoadImage(PATH "building4.png");
    Image building5 = LoadImage(PATH "building5.png");
    // make them all 50% smaller

    float WidthResizeFactor = 0.8f;
    float HeightResizeFactor = 0.54f;
    ImageResize(&building1, building1.width * WidthResizeFactor, building1.height * HeightResizeFactor);
    ImageResize(&building2, building2.width * WidthResizeFactor, building2.height * HeightResizeFactor);
    ImageResize(&building3, building3.width * WidthResizeFactor, building3.height * HeightResizeFactor);
    ImageResize(&building4, building4.width * WidthResizeFactor, building4.height * HeightResizeFactor);
    ImageResize(&building5, building5.width * WidthResizeFactor, building5.height * HeightResizeFactor);
    // Load textures
    int terrainImageCount = 0;
    Texture2D terrainImages[5];
    terrainImages[terrainImageCount++] = LoadTextureFromImage(building1);
    terrainImages[terrainImageCount++] = LoadTextureFromImage(building2);
    terrainImages[terrainImageCount++] = LoadTextureFromImage(building3);
    terrainImages[terrainImageCount++] = LoadTextureFromImage(building4);
    terrainImages[terrainImageCount++] = LoadTextureFromImage(building5);

    // PFPs
    Image nouraImage = LoadImage(PATH "nourapfp.png");
    Image karimImage = LoadImage(PATH "karimpfp.png");
    Image zakiImage = LoadImage(PATH "zakipfp.png");
    Image omarImage = LoadImage(PATH "omarpfp.png");
    ImageResize(&nouraImage, 250, 250);
    ImageResize(&karimImage, 250, 250);
    ImageResize(&zakiImage, 250, 250);
    ImageResize(&omarImage, 250, 250);
    Texture2D noura = LoadTextureFromImage(nouraImage);
    Texture2D karim = LoadTextureFromImage(karimImage);
    Texture2D zaki = LoadTextureFromImage(zakiImage);
    Texture2D omar = LoadTextureFromImage(omarImage);
    // Songs
    Sound oghneya = LoadSound(PATH "oghneya.mp3");
    Sound oghneya_music = LoadSound(PATH "oghneyaMusic.mp3");
    // SFX
    Sound countdownAudio = LoadSound(PATH "countdownAudio.mp3");
    Sound clickSFX = LoadSound(PATH "clickSFX.mp3");
    Sound backgroundMusic = LoadSound(PATH "vidAudio.mp3");
    Sound yalaBina = LoadSound(PATH "yalaBina.mp3");
	knifeSFX = LoadSound(PATH "knifeSFX.mp3");
    // Extras
    Texture2D howToPlaySingle = LoadTexture(PATH "singleplayerGuide.png");
	Texture2D howToPlayMulti = LoadTexture(PATH "multiplayerGuide.png");
	Texture2D notesGuide = LoadTexture(PATH "notesGuide.png");

    ////////////////////////////////////////////////////// TRAILER //////////////////////////////////////////////////////


    // Define variables for loading images
    std::vector<Texture2D> frames;
    std::vector<Image> images(712);  // Reserve space for 712 images
    const int totalAssets = 712;  // Total assets (images) count

    // Mutex to protect shared data during multi-threaded access
    std::mutex mtx;

    // Start loading images and show progress
    SetTargetFPS(18);  // Set FPS for smooth loading screen

    // Create threads to load images in chunks for faster loading
    const int chunkSize = 100;  // Load 100 images at a time
    std::vector<std::thread> threads;
    for (int i = 0; i < totalAssets; i += chunkSize) {
        int end = std::min(i + chunkSize - 1, totalAssets - 1);
        threads.push_back(std::thread(loadImagesAsync, std::ref(images), i, end, std::ref(mtx)));
    }

    // Wait for all threads to finish loading images
    for (auto& t : threads) {
        t.join();
    }

    for (const Image& img : images) {
        frames.push_back(LoadTextureFromImage(img));
    }

    PlaySound(backgroundMusic);

    bool skipClicked = false;
    int currentFrame = 0;
    float videoDuration = 43.0f; 
    float elapsedTime = 0.0f;  

    // Main video loop
    while (!WindowShouldClose()) {
        elapsedTime += GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);

        if (!skipClicked) {
            if (currentFrame < frames.size()) {
                DrawTexture(frames[currentFrame], (window_width - frames[currentFrame].width) / 2, (window_height - frames[currentFrame].height) / 2, WHITE);
            }
        }

        const int buttonWidth = 100;
        const int buttonHeight = 30;
        const int buttonX = window_width - buttonWidth - 20;
        const int buttonY = 20;

        if (!skipClicked && elapsedTime < videoDuration) {
            DrawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, DARKGRAY);
            DrawText("Skip", buttonX + (buttonWidth - MeasureText("Skip", 20)) / 2, buttonY + (buttonHeight - 20) / 2, 20, WHITE);
        }

        if (CheckCollisionPointRec(GetMousePosition(), Rectangle{ (float)buttonX, (float)buttonY, (float)buttonWidth, (float)buttonHeight }) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            skipClicked = true;
        }

        if (skipClicked || elapsedTime >= videoDuration) {
            for (size_t i = 0; i < frames.size(); i++) {
                UnloadTexture(frames[i]);
                UnloadImage(images[i]);
            }
            frames.clear();
            images.clear();
            StopSound(backgroundMusic);

            isIntro = false;
            break;
        }

        EndDrawing();

        if (!skipClicked && currentFrame < frames.size() - 1) {
            currentFrame++;
        }

    }


    //////////////////////////////////////////// END /////////////////////////////////////////////





    character = { 1100, 200, ahmedImage.width, ahmedImage.height, 4, 0, false, false };
    enemy = { 100, 200, samraImage.width, samraImage.height, 0, false, 0 };

    Vector2 camera_offset = { 0 };
    Vector2 camera_target = { 0 };

    Camera2D camera = { camera_offset, camera_target, 0, 1 };
    int platform_count = 9999;
    Platform* platforms = (Platform*)malloc(platform_count * sizeof(Platform));
    int spacing = 300;
    if (platforms == NULL) {
        return -1; 
    }


    for (int i = 0; i < platform_count; i++) {
        int imageIndex = rand() % terrainImageCount;
        platforms[i].imageIndex = imageIndex;

        if (i == 0) {
            platforms[i].x = 0;
        }
        else {
            platforms[i].x = platforms[i - 1].x + platforms[i - 1].width + spacing;
        }

        platforms[i].y = window_height - terrainImages[imageIndex].height;
        platforms[i].width = terrainImages[imageIndex].width;
        platforms[i].height = terrainImages[imageIndex].height;
    }



    UI ui = { 0, 0.0f };

    if (!isIntro) {

        SetTargetFPS(60);
        while (!WindowShouldClose()) {

            if (!IsSoundPlaying(oghneya_music) && inMenu) {
                PlaySound(oghneya_music);
            }
            if (!IsSoundPlaying(oghneya) && !inMenu) {
                PlaySound(oghneya);
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(background, 0, 0, WHITE);

            if (inMenu) {
                DrawText("ElShabah", window_width / 2 - MeasureText("ElShabah", 60) / 2, window_height / 4, 60, BLACK);
                Rectangle singlePlayerButton = { window_width / 2 - 100, window_height / 2 - 100, 200, 50 };
                Rectangle multiplayerButton = { window_width / 2 - 100, window_height / 2 - 30, 200, 50 };
                Rectangle soundButton = { window_width / 2 - 100, window_height / 2 + 40, 200, 50 };
                Rectangle howToPlayButton = { window_width / 2 - 100, window_height / 2 + 110, 200, 50 };
                Rectangle devsButton = { window_width / 2 - 100, window_height / 2 + 180, 200, 50 };


                DrawRectangleRec(singlePlayerButton, LIGHTGRAY);
                DrawRectangleRec(multiplayerButton, LIGHTGRAY);
                DrawRectangleRec(soundButton, LIGHTGRAY);
                DrawRectangleRec(howToPlayButton, LIGHTGRAY);
                DrawRectangleRec(devsButton, LIGHTGRAY);


                DrawText("Single Player", window_width / 2 - MeasureText("Single Player", 20) / 2, window_height / 2 - 85, 20, BLACK);
                DrawText("Multiplayer", window_width / 2 - MeasureText("Multiplayer", 20) / 2, window_height / 2 - 15, 20, BLACK);
                DrawText("Sound", window_width / 2 - MeasureText("Sound", 20) / 2, window_height / 2 + 55, 20, BLACK);
                DrawText("How to play", window_width / 2 - MeasureText("How to play", 20) / 2, window_height / 2 + 125, 20, BLACK);
                DrawText("Devs", window_width / 2 - MeasureText("Devs", 20) / 2, window_height / 2 + 195, 20, BLACK);

                DrawText("Inspired by the movie El Shabah", window_width / 2 - MeasureText("Inspired by the movie El Shabah", 20) / 2, window_height - 40, 20, WHITE);

                // check if mouse hover over button
                Vector2 mousePoint = GetMousePosition();
                if (CheckCollisionPointRec(mousePoint, singlePlayerButton)) {
                    Color shade = { 111, 115, 120, 255 };
                    DrawRectangleRec(singlePlayerButton, shade);
                    DrawText("Single Player", window_width / 2 - MeasureText("Single Player", 20) / 2, window_height / 2 - 85, 20, BLACK);
                }
                if (CheckCollisionPointRec(mousePoint, multiplayerButton)) {
                    Color shade = { 111, 115, 120, 255 };
                    DrawRectangleRec(multiplayerButton, shade);
                    DrawText("Multiplayer", window_width / 2 - MeasureText("Multiplayer", 20) / 2, window_height / 2 - 15, 20, BLACK);
                }
                if (CheckCollisionPointRec(mousePoint, soundButton)) {
                    Color shade = { 111, 115, 120, 255 };
                    DrawRectangleRec(soundButton, shade);
                    DrawText("Sound", window_width / 2 - MeasureText("Sound", 20) / 2, window_height / 2 + 55, 20, BLACK);
                }
                if (CheckCollisionPointRec(mousePoint, howToPlayButton)) {
                    Color shade = { 111, 115, 120, 255 };
                    DrawRectangleRec(howToPlayButton, shade);
                    DrawText("How to play", window_width / 2 - MeasureText("How to play", 20) / 2, window_height / 2 + 125, 20, BLACK);
                }
                if (CheckCollisionPointRec(mousePoint, devsButton)) {
                    Color shade = { 111, 115, 120, 255 };
                    DrawRectangleRec(devsButton, shade);
                    DrawText("Devs", window_width / 2 - MeasureText("Devs", 20) / 2, window_height / 2 + 195, 20, BLACK);
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();
                    if (CheckCollisionPointRec(mousePoint, singlePlayerButton)) {
                        PlaySound(clickSFX);
                        PlaySound(yalaBina);
                        inSound = false;
                        inMenu = false;
                        inCountdown = true;
                        gameMode = 0;
                        StopSound(oghneya_music);
                        PlaySound(countdownAudio);

                        countdownTime = 3.0f; // Reset countdown time

                    }

                    if (CheckCollisionPointRec(mousePoint, multiplayerButton)) {
                        PlaySound(clickSFX);
                        inMenu = false;
                        inMultiplayerSetup = true;
                    }

                    if (CheckCollisionPointRec(mousePoint, soundButton)) {
                        PlaySound(clickSFX);
                        inMenu = false;
                        inSound = true;
                    }

                    if (CheckCollisionPointRec(mousePoint, devsButton)) {
                        PlaySound(clickSFX);
                        inMenu = false;
                        inDevs = true;
                    }

                    if (CheckCollisionPointRec(mousePoint, howToPlayButton)) {
                        PlaySound(clickSFX);
                        inMenu = false;
                        inHowToPlay = true;
                    }

                }
            }
            else if (inCountdown) {
                BeginMode2D(camera);

                DrawTexture(background, 0, 0, WHITE);

                for (int i = 0; i < platform_count; i++) {
                    int imageIndex = platforms[i].imageIndex;
                    DrawTexture(terrainImages[imageIndex], platforms[i].x, platforms[i].y, WHITE);
                }

                if (character.jumping) {
                    DrawTexture(ahmedjump, character.x, character.y, WHITE);
                }
                else if (character.walking) {
                    DrawTexture(ahmedwalk, character.x, character.y, WHITE);
                }
                else {
                    DrawTexture(ahmedImage, character.x, character.y, WHITE);
                }

                if (enemy.jumping) {
                    DrawTexture(samrajump, enemy.x, enemy.y, WHITE);
                }
                else if (enemy.walking) {
                    DrawTexture(samrawalk, enemy.x, enemy.y, WHITE);
                }
                else {
                    DrawTexture(samraImage, enemy.x, enemy.y, WHITE);
                }

                EndMode2D();

                countdownTime -= GetFrameTime();
                if (countdownTime <= 0) {
                    inCountdown = false;
                    gameMode == 0 ? inGame = true : inMultiplayer = true;
                    gameMode == 1 ? inGame = false : inMultiplayer = false;
                }
                else {
                    DrawText(TextFormat("%d", (int)ceil(countdownTime)), window_width / 2 - 20, window_height / 2 - 60, 140, WHITE);
                    PlaySound(oghneya);
                }
            }



            
            else if (inMultiplayerSetup) {
                StopSound(oghneya);
				DrawText("Multiplayer", window_width / 2 - MeasureText("Multiplayer", 40) / 2, window_height / 4, 40, BLACK);
                Rectangle hostButton = { window_width / 2 - 100, window_height / 2 - 100, 200, 50 };
                Rectangle joinButton = { window_width / 2 - 100, window_height / 2 - 30, 200, 50 };
				Rectangle backButton = { window_width / 2 - 100, window_height / 2 + 40, 200, 50 };

                DrawRectangleRec(hostButton, LIGHTGRAY);
                DrawText("Host", window_width / 2 - 30, window_height / 2 - 85, 20, BLACK);

                DrawRectangleRec(joinButton, LIGHTGRAY);
                DrawText("Join", window_width / 2 - 30, window_height / 2 - 15, 20, BLACK);

                DrawRectangleRec(backButton, LIGHTGRAY);
				DrawText("Back", window_width / 2 - 30, window_height / 2 + 55, 20, BLACK);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();
                    if (CheckCollisionPointRec(mousePoint, hostButton)) {
                        inMultiplayerSetup = false;
                        inCharacterChoice = true;
                    }
                    else if (CheckCollisionPointRec(mousePoint, joinButton)) {
                        inMultiplayerSetup = false;
                        inJoinGame = true;
                    }
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();
                    if (CheckCollisionPointRec(mousePoint, backButton)) {
                        inMultiplayerSetup = false;
                        inMenu = true;
                        PlaySound(clickSFX);
                    }
                }
            }

            else if (inCharacterChoice) {
                StopSound(oghneya);
				DrawText("Choose a character", window_width / 2 - MeasureText("Choose a character", 40) / 2, window_height / 4, 40, BLACK);
                Rectangle ahmedButton = { window_width / 2 - 100, window_height / 2 - 100, 200, 50 };
                Rectangle samraButton = { window_width / 2 - 100, window_height / 2 - 30, 200, 50 };

                DrawRectangleRec(ahmedButton, LIGHTGRAY);
                DrawText("Ahmed", window_width / 2 - 30, window_height / 2 - 85, 20, BLACK);

                DrawRectangleRec(samraButton, LIGHTGRAY);
                DrawText("Samra", window_width / 2 - 30, window_height / 2 - 15, 20, BLACK);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();
                    if (CheckCollisionPointRec(mousePoint, ahmedButton)) {
                        playerName = "Ahmed";
                        inCharacterChoice = false;
                        inShowIP = true; 
                        startServer(playerName, [](const std::string& msg) { onMessageReceived(msg); });
                    }
                    else if (CheckCollisionPointRec(mousePoint, samraButton)) {
                        playerName = "Samra";
                        inCharacterChoice = false;
                        inShowIP = true; 
                        startServer(playerName, [](const std::string& msg) { onMessageReceived(msg); });
                    }
                }
            }

            else if (inShowIP) {
                StopSound(oghneya);
                std::string ip = getLocalIP();
				DrawText("Your IP:", window_width / 2 - MeasureText("Your IP:", 40) / 2, window_height / 4, 40, WHITE);
				DrawText(ip.c_str(), window_width / 2 - MeasureText(ip.c_str(), 40) / 2, window_height / 4 + 50, 40, WHITE);
				DrawText("Waiting for another player to join...", window_width / 2 - MeasureText("Waiting for another player to join...", 20) / 2, window_height / 2, 20, WHITE);

                if (isConnected) {
                    inShowIP = false;
                    inMultiplayer = true;
                }
}

            else if (inJoinGame) {
                StopSound(oghneya);
				DrawText("Enter Host IP Address", window_width / 2 - MeasureText("Enter Host IP Address", 40) / 2, window_height / 4, 40, BLACK);
                Rectangle ipInputBox = { window_width / 2 - 100, window_height / 2 - 50, 200, 50 };
                Rectangle connectButton = { window_width / 2 - 100, window_height / 2 + 20, 200, 50 };
				Rectangle backButton = { window_width / 2 - 100, window_height / 2 + 90, 200, 50 };

                DrawRectangleRec(ipInputBox, LIGHTGRAY);
                DrawText(serverIP.c_str(), window_width / 2 - 90, window_height / 2 - 35, 20, BLACK);

				DrawRectangleRec(connectButton, LIGHTGRAY);
				DrawText("Connect", window_width / 2 - MeasureText("Connect", 20) / 2, window_height / 2 + 35, 20, BLACK);

				DrawRectangleRec(backButton, LIGHTGRAY);
				DrawText("Back", window_width / 2 - MeasureText("Back", 20) / 2, window_height / 2 + 105, 20, BLACK);


                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();
                    if (CheckCollisionPointRec(mousePoint, connectButton)) {
                        startClient(serverIP, [](const std::string& msg) { onMessageReceived(msg); });
                        inJoinGame = false;
                        inMultiplayer = true;
                    }
                }

				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					Vector2 mousePoint = GetMousePosition();
					if (CheckCollisionPointRec(mousePoint, backButton)) {
						inJoinGame = false;
						inMultiplayerSetup = true;
						PlaySound(clickSFX);
					}
				}

                int key = GetCharPressed();
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (!serverIP.empty()) {
                        serverIP.pop_back();
                    }
                }
                while (key > 0) {
                    if ((key >= '0' && key <= '9') || key == '.') { 
                        serverIP += static_cast<char>(key);
                    }
                    key = GetCharPressed();
                }

            }

            else if (inHowToPlay) {
                StopSound(oghneya);

                Texture2D guides[] = { howToPlaySingle, howToPlayMulti, notesGuide };
                int guideCount = 3; 

                Texture2D currentGuide = guides[howToPlayImageIndex];
                DrawTexture(currentGuide, (window_width - currentGuide.width) / 2, (window_height - currentGuide.height) / 2, WHITE);

                Rectangle backButton = { (float)(window_width / 2 - 100), (float)(window_height / 2 + 300), 200.0f, 50.0f };
                DrawRectangleRec(backButton, LIGHTGRAY);
                DrawText("Back", window_width / 2 - MeasureText("Back", 20) / 2, window_height / 2 + 315, 20, BLACK);

                Rectangle leftArrowButton = { 50, (float)(window_height / 2 - 25), 50, 50 };
                DrawRectangleRec(leftArrowButton, LIGHTGRAY);
                DrawText("<", leftArrowButton.x + 15, leftArrowButton.y + 10, 30, BLACK);

                Rectangle rightArrowButton = { (float)(window_width - 100), (float)(window_height / 2 - 25), 50, 50 };
                DrawRectangleRec(rightArrowButton, LIGHTGRAY);
                DrawText(">", rightArrowButton.x + 15, rightArrowButton.y + 10, 30, BLACK);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();

                    if (CheckCollisionPointRec(mousePoint, backButton)) {
                        PlaySound(clickSFX);
                        inHowToPlay = false;
                        inMenu = true;
                    }

                    if (CheckCollisionPointRec(mousePoint, leftArrowButton)) {
                        PlaySound(clickSFX);
                        howToPlayImageIndex--;
                        if (howToPlayImageIndex < 0) {
                            howToPlayImageIndex = guideCount - 1;
                        }
                    }

                    if (CheckCollisionPointRec(mousePoint, rightArrowButton)) {
                        PlaySound(clickSFX);
                        howToPlayImageIndex++;
                        if (howToPlayImageIndex >= guideCount) {
                            howToPlayImageIndex = 0; 
                        }
                    }
                }
            }

            else if (inDevs) {
                StopSound(oghneya);
                int pfp_size = 250;
                int spacing = 70;
                int start_x = (window_width - (4 * pfp_size + 3 * spacing)) / 2;
                int y_position = window_height / 2 - pfp_size / 2;

                DrawTexture(noura, start_x, y_position, WHITE);
                DrawTexture(karim, start_x + pfp_size + spacing, y_position, WHITE);
                DrawTexture(zaki, start_x + 2 * (pfp_size + spacing), y_position, WHITE);
                DrawTexture(omar, start_x + 3 * (pfp_size + spacing), y_position, WHITE);

                int text_offset = (pfp_size - MeasureText("Noura Hesham", 40)) / 2;
                DrawText("Noura Hesham", start_x + text_offset, y_position + pfp_size + 10, 40, WHITE);
                text_offset = (pfp_size - MeasureText("Karim Amr", 40)) / 2;
                DrawText("Karim Amr", start_x + pfp_size + spacing + text_offset, y_position + pfp_size + 10, 40, WHITE);
                text_offset = (pfp_size - MeasureText("Karim Zaki", 40)) / 2;
                DrawText("Karim Zaki", start_x + 2 * (pfp_size + spacing) + text_offset, y_position + pfp_size + 10, 40, WHITE);
                text_offset = (pfp_size - MeasureText("Omar Ahmed", 40)) / 2;
                DrawText("Omar Ahmed", start_x + 3 * (pfp_size + spacing) + text_offset, y_position + pfp_size + 10, 40, WHITE);

                Rectangle backButton = { (float)(window_width / 2 - 100), (float)(window_height / 2 + 300), 200.0f, 50.0f };
                DrawRectangleRec(backButton, LIGHTGRAY);
                DrawText("Back", window_width / 2 - MeasureText("Back", 20) / 2, window_height / 2 + 315, 20, BLACK);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();
                    if (CheckCollisionPointRec(mousePoint, backButton)) {
                        PlaySound(clickSFX);
                        inDevs = false;
                        inMenu = true;
                    }
                }
            }
            else if (inSound) {

                StopSound(oghneya);

                DrawText("Sound Options", window_width / 2 - MeasureText("Sound Options", 40) / 2, window_height / 4, 40, BLACK);
                Rectangle muteButton = { (float)(window_width / 2 - 100), (float)(window_height / 2 - 100), 200.0f, 50.0f };
                Rectangle volumeUpButton = { (float)(window_width / 2 + 60), (float)(window_height / 2 - 30), 50.0f, 50.0f };
                Rectangle volumeDownButton = { (float)(window_width / 2 - 110), (float)(window_height / 2 - 30), 50.0f, 50.0f };
                Rectangle backButton = { (float)(window_width / 2 - 100), (float)(window_height / 2 + 110), 200.0f, 50.0f };
                Rectangle volumeTextBox = { (float)(window_width / 2 - 50), (float)(window_height / 2 - 30), 100.0f, 50.0f };

                DrawRectangleRec(muteButton, LIGHTGRAY);
                DrawRectangleRec(volumeUpButton, LIGHTGRAY);
                DrawRectangleRec(volumeDownButton, LIGHTGRAY);
                DrawRectangleRec(backButton, LIGHTGRAY);
                DrawRectangleRec(volumeTextBox, LIGHTGRAY);

                DrawText(isMuted ? "Unmute" : "Mute", window_width / 2 - MeasureText(isMuted ? "Unmute" : "Mute", 20) / 2, window_height / 2 - 85, 20, BLACK);
                DrawText(" +", window_width / 2 + 75 - MeasureText("+", 20) / 2, window_height / 2 - 15, 20, BLACK);
                DrawText(" -", window_width / 2 - 95 - MeasureText("-", 20) / 2, window_height / 2 - 15, 20, BLACK);
                DrawText("Back", window_width / 2 - MeasureText("Back", 20) / 2, window_height / 2 + 125, 20, BLACK);

                char volumeText[20];
                sprintf_s(volumeText, "vol: %d%%", (int)(soundVolume * 100));
                DrawText(volumeText, window_width / 2 - MeasureText(volumeText, 20) / 2, window_height / 2 - 15, 20, BLACK);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePoint = GetMousePosition();

                    if (CheckCollisionPointRec(mousePoint, muteButton)) {
                        PlaySound(clickSFX);
                        isMuted = !isMuted;

                        if (isMuted) {
                            prevVolume = soundVolume;
                            soundVolume = 0.0f;
                            SetMasterVolume(0.0f);
                        }
                        else {
                            SetMasterVolume(prevVolume);
                            soundVolume = prevVolume;
                        }
                    }

                    if (CheckCollisionPointRec(mousePoint, volumeUpButton) && !isMuted) {
                        PlaySound(clickSFX);
                        if (soundVolume < 1.0f) {
                            soundVolume += 0.1f;
                            SetMasterVolume(soundVolume);
                        }
                    }

                    if (CheckCollisionPointRec(mousePoint, volumeDownButton) && !isMuted) {
                        PlaySound(clickSFX);
                        if (soundVolume > 0.0f) {
                            soundVolume -= 0.1f;
                            SetMasterVolume(soundVolume);
                        }
                    }

                    if (CheckCollisionPointRec(mousePoint, backButton)) {
                        PlaySound(clickSFX);
                        inSound = false;
                        inMenu = true;
                    }
                }
            }

            else if (inGame) {
                BeginMode2D(camera);

                if (!gameover) {
                    camera.offset.x -= 11;

                    character.y += character.velocity;
                    character.velocity += gravity;

                    enemy.y += enemy.velocity;
                    enemy.velocity += gravity;

                    int current_platform = onPlatform(character, platforms, platform_count);
                    if (current_platform != -1) {
                        if (character.velocity > 0) {
                            character.y = platforms[current_platform].y - character.height;
                            character.velocity = 0;
                            character.jumping = false;
                            character.walking = false;
                            character.jumpcount = 0;
                        }

                    }


                    int enemy_platform = onPlatform(enemy, platforms, platform_count);
                    if (enemy_platform != -1) {
                        if (enemy.velocity > 0) {
                            enemy.y = platforms[enemy_platform].y - enemy.height;
                            enemy.velocity = 0;
                            enemy.jumping = false;
                            enemy.jumpcount = 0;
                        }
                    }



                    if (character.y + character.height >= window_height) {
                        character.y = window_height - character.height;
                        character.velocity = 0;
                        gameover = true; 
                    }


                    if (enemy.y + enemy.height >= window_height) {
                        enemy.y = window_height - enemy.height;
                        enemy.velocity = 0;
                    }

                    if (IsKeyPressed(KEY_UP) && character.jumpcount < 3) {
                        character.velocity = -30;
                        character.jumping = true;
                        character.jumpcount++;
                    }
                    if (IsKeyPressed(KEY_DOWN)) {
                        character.velocity = 20;
                        character.jumping = false;

                    }

                    if (IsKeyDown(KEY_RIGHT)) {
                        character.x += 12; 
                        character.walking = true;

                        for (int i = 0; i < platform_count; i++) {
                            Rectangle platform_rec = { (float)platforms[i].x, (float)platforms[i].y, (float)(platforms[i].width - 80), (float)platforms[i].height };
                            Rectangle character_rec = { (float)character.x, (float)character.y, (float)character.width, (float)character.height };

                            if (CheckCollisionRecs(character_rec, platform_rec)) {
                                if (character.x + character.width > platforms[i].x && character.x < platforms[i].x) {
                                    character.x = platforms[i].x - character.width;
                                }
                            }
                        }
                    }

                    if (character.jumping) {
                        DrawTexture(ahmedjump, character.x, character.y, WHITE);
                    }
                    else if (character.walking) {
                        double time = GetTime() * 10;
                        if ((int)time % 2 == 0) {
                            DrawTexture(ahmedwalk, character.x, character.y, WHITE);
                        }
                        else {
                            DrawTexture(ahmedImage, character.x, character.y, WHITE);
                        }
                    }
                    else {
                        DrawTexture(ahmedImage, character.x, character.y, WHITE);
                    }

                    if (enemy.x < character.x) {
                        enemy.x += 11;
                    }
                    else if (enemy.x > character.x) {
                        gameover = true;
                    }

                    if (enemy_platform != -1) {
                        if (enemy.x + enemy.width >= platforms[enemy_platform].x + platforms[enemy_platform].width - 80 && enemy.jumpcount < 2) {
                            enemy.velocity = -30;
                            enemy.jumping = true;
                            enemy.jumpcount++;
                        }
                    }

                    if (enemy.jumping) {
                        DrawTexture(samrajump, enemy.x, enemy.y, WHITE);
                    }
                    else {
                        double time = GetTime() * 10;
                        if ((int)time % 2 == 0) {
                            DrawTexture(samrawalk, enemy.x, enemy.y, WHITE);
                        }
                        else {
                            DrawTexture(samraImage, enemy.x, enemy.y, WHITE);
                        }
                    }

                    if (sizeof(knives) > knife_count) {
                        float knifeInterval;
						if (ui.score > 500) knifeInterval = rand() % 3 + 1;
						else if (ui.score > 250) knifeInterval = rand() % 5 + 3;
						else if (ui.score > 100) knifeInterval = rand() % 10 + 5;
						else knifeInterval = 999;
                        if (GetTime() - last_knife_time > knifeInterval) {
                            if (enemy.x < character.x) {
								PlaySound(knifeSFX);
                                knives[knife_count].x = enemy.x + enemy.width;
                                knives[knife_count].y = enemy.y + enemy.height / 2;
                                knives[knife_count].width = knifeImage.width;
                                knives[knife_count].height = knifeImage.height;
                                knives[knife_count].velocity = 20; 
                                knife_count++;
                                last_knife_time = GetTime();
                            }
                        }
                        for (int i = 0; i < knife_count; i++) {
                            knives[i].x += knives[i].velocity;

                            Rectangle character_rec = { (float)character.x, (float)character.y, (float)character.width, (float)character.height };
                            Rectangle knife_rec = { (float)knives[i].x, (float)knives[i].y, (float)knives[i].width, (float)knives[i].height };
                            if (CheckCollisionRecs(knife_rec, character_rec)) {
                                gameover = true;
                            }

                            DrawTexture(knifeImage, knives[i].x, knives[i].y, WHITE);
                        }
                    }

                    for (int i = 0; i < platform_count; i++) {
                        int imageIndex = platforms[i].imageIndex; 
                        DrawTexture(terrainImages[imageIndex], platforms[i].x, platforms[i].y, WHITE);
                    }

                    ui.timer += GetFrameTime();
                    if (ui.timer >= 0.15f) {
                        ui.score++;
                        ui.timer = 0.0f;
                    }
                    EndMode2D();
                    DrawText(TextFormat("Score: %d", ui.score), 10, 10, 40, BLACK);
                }
                else {
                    if (ui.score > ui.highscore) ui.highscore = ui.score;

                    DrawRectangle(0, 0, window_width, window_height, BLACK);

                    static float fadeAlpha = 0.0f;
                    static bool fadeIn = true;
                    static float fadeTimer = 0.0f;

                    if (fadeIn) {
                        fadeAlpha += GetFrameTime() / 2.0f; 
                        if (fadeAlpha >= 1.0f) {

                            fadeAlpha = 1.0f;
                            fadeIn = false;
                        }
                    }
                    else {
                        fadeAlpha -= GetFrameTime() / 2.0f; 
                        if (fadeAlpha <= 0.0f) {
                            fadeAlpha = 0.0f;

                        }
                    }

                    DrawTexture(theEndImage, (window_width - theEndImage.width) / 2, (window_height - theEndImage.height) / 2, Fade(WHITE, fadeAlpha));

                    if (!fadeIn && fadeAlpha == 0.0f) {
                        StopSound(oghneya);

                        DrawText("Game Over", window_width / 2 - 100, window_height / 2 - 50, 40, RED);
                        DrawText(TextFormat("Score: %d", ui.score), window_width / 2 - 100, window_height / 2, 20, WHITE);
                        DrawText(TextFormat("High Score: %d", ui.highscore), window_width / 2 - 100, window_height / 2 + 25, 20, WHITE);

                        Rectangle playAgainButton = { window_width / 2 - 100, window_height / 2 + 70, 200, 50 };
                        DrawRectangleRounded(playAgainButton, 0.2f, 10, WHITE); 
                        DrawText("Play Again", window_width / 2 - 50, window_height / 2 + 85, 20, BLACK);

                        Rectangle menuButton = { window_width / 2 - 100, window_height / 2 + 140, 200, 50 };
                        DrawRectangleRounded(menuButton, 0.2f, 10, WHITE); 
                        DrawText("Menu", window_width / 2 - 30, window_height / 2 + 155, 20, BLACK);

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            Vector2 mousePoint = GetMousePosition();
                            if (CheckCollisionPointRec(mousePoint, playAgainButton)) {
                                PlaySound(yalaBina);
                                gameover = false;
                                ResetGame(character, enemy, ui, window_height);
                                fadeAlpha = 0.0f;
                                fadeIn = true;
                                countdownTime = 3.0f; 
                                inCountdown = true;
                            }


                            else if (inCountdown) {
                                countdownTime -= GetFrameTime();
                                if (countdownTime <= 0) {
                                    inCountdown = false;
                                    gameMode == 0 ? inGame = true : inMultiplayer = true;
                                    gameMode == 1 ? inGame = false : inMultiplayer = false;
                                }
                                else {

                                    DrawText(TextFormat("%d", (int)ceil(countdownTime)), window_width / 2 - 20, window_height / 2 - 60, 140, BLACK);

                                    PlaySound(oghneya);
                                }
                            }
                            else if (CheckCollisionPointRec(mousePoint, menuButton)) {
                                inMenu = true;
                                gameover = false;
                                StopSound(oghneya);
                                PlaySound(oghneya_music);
                                ResetGame(character, enemy, ui, window_height);
                            }
                        }
                    }

                    camera.offset.x = 0;
                }
            }
            else if (inMultiplayer) {
				PlaySound(oghneya);
                BeginMode2D(camera);

                if (!gameover) {
                    camera.offset.x -= 11; 

                    character.y += character.velocity;
                    character.velocity += gravity;

                    enemy.y += enemy.velocity;
                    enemy.velocity += gravity;

                    Rectangle character_rec = { (float)character.x, (float)character.y, (float)character.width, (float)character.height };
                    Rectangle ground_rec = { 0.0f, 500.0f, 1920.0f, 500.0f };
                    int current_platform = onPlatform(character, platforms, platform_count);
                    if (current_platform != -1) {
                        if (character.velocity > 0) {
                            character.y = platforms[current_platform].y - character.height;
                            character.velocity = 0;
                            character.jumping = false;
                            character.walking = false;
                            character.jumpcount = 0;
                        }
                    }

                    int enemy_platform = onPlatform(enemy, platforms, platform_count);
                    if (enemy_platform != -1) {
                        if (enemy.velocity > 0) {
                            enemy.y = platforms[enemy_platform].y - enemy.height;
                            enemy.velocity = 0;
                            enemy.walking = false;
                            enemy.jumping = false;
                            enemy.jumpcount = 0;
                        }
                    }

                    if (character.y + character.height >= window_height) {
                        character.y = window_height - character.height;
                        character.velocity = 0;
                        gameover = true;
                    }

                    if (enemy.y + enemy.height >= window_height) {
                        enemy.y = window_height - enemy.height;
                        enemy.velocity = 0;
                        gameover = true; 
                    }


                    if (playerName == "Ahmed") {
                        if (IsKeyPressed(KEY_UP) && character.jumpcount < 3) {
                            character.velocity = -30;
                            character.jumping = true;
                            character.jumpcount++;
                            sendMessage("AHMED_JUMP");
                        }
                        if (IsKeyPressed(KEY_DOWN)) {
                            character.velocity = 20;
                            character.jumping = false;
                            sendMessage("AHMED_DOWN");
                        }
                        if (IsKeyDown(KEY_RIGHT)) {
                            character.x += 12;
                            character.walking = true;
                            sendMessage("AHMED_RIGHT");

                            for (int i = 0; i < platform_count; i++) {
                                Rectangle platform_rec = { (float)platforms[i].x, (float)platforms[i].y, (float)(platforms[i].width - 80), (float)platforms[i].height };
                                Rectangle character_rec = { (float)character.x, (float)character.y, (float)character.width, (float)character.height };

                                if (CheckCollisionRecs(character_rec, platform_rec)) {
                                    if (character.x + character.width > platforms[i].x && character.x < platforms[i].x) {
                                        character.x = platforms[i].x - character.width;
                                    }
                                }
                            }
                        }
                    }
                    else if (playerName == "Samra") {
                        if (IsKeyPressed(KEY_UP) && enemy.jumpcount < 3) {
                            enemy.velocity = -30;
                            enemy.jumping = true;
                            enemy.jumpcount++;
                            sendMessage("SAMRA_JUMP");
                        }
                        if (IsKeyPressed(KEY_DOWN)) {
                            enemy.velocity = 20;
                            enemy.jumping = false;
                            sendMessage("SAMRA_DOWN");
                        }
                        if (IsKeyDown(KEY_RIGHT)) {
                            enemy.x += 12;
                            enemy.walking = true;
                            sendMessage("SAMRA_RIGHT");

                            for (int i = 0; i < platform_count; i++) {
                                Rectangle platform_rec = { (float)platforms[i].x, (float)platforms[i].y, (float)(platforms[i].width - 80), (float)platforms[i].height };
                                Rectangle character_rec = { (float)character.x, (float)character.y, (float)character.width, (float)character.height };

                                if (CheckCollisionRecs(character_rec, platform_rec)) {
                                    if (character.x + character.width > platforms[i].x && character.x < platforms[i].x) {
                                        character.x = platforms[i].x - character.width;
                                    }
                                }
                            }
                        }

                        if (IsKeyPressed(KEY_SPACE)) { 
                            if (enemy.x < character.x && GetTime() - last_knife_time > 5) {
                                knives[knife_count].x = enemy.x + enemy.width;
                                knives[knife_count].y = enemy.y + enemy.height / 2;
                                knives[knife_count].width = knifeImage.width;
                                knives[knife_count].height = knifeImage.height;
                                knives[knife_count].velocity = 20; 
                                knife_count++;
                                last_knife_time = GetTime();

                                PlaySound(knifeSFX);
								sendMessage("SAMRA_KNIFE");
                            }
                        }

                    }

                    if (enemy.x > character.x) {
                        gameover = true;
                    }

                    if (character.jumping) {
                        DrawTexture(ahmedjump, character.x, character.y, WHITE);
                    }
                    else if (character.walking) {
                        double time = GetTime() * 10;
                        if ((int)time % 2 == 0) {
                            DrawTexture(ahmedwalk, character.x, character.y, WHITE);
                        }
                        else {
                            DrawTexture(ahmedImage, character.x, character.y, WHITE);
                        }
                    }
                    else {
                        DrawTexture(ahmedImage, character.x, character.y, WHITE);
                    }

                    if (enemy.jumping) {
                        DrawTexture(samrajump, enemy.x, enemy.y, WHITE);
                    }
                    else if (enemy.walking) {
                        double time = GetTime() * 10;
                        if ((int)time % 2 == 0) {
                            DrawTexture(samrawalk, enemy.x, enemy.y, WHITE);
                        }
                        else {
                            DrawTexture(samraImage, enemy.x, enemy.y, WHITE);
                        }
                    }
                    else {
                        DrawTexture(samraImage, enemy.x, enemy.y, WHITE);
                    }


                    for (int i = 0; i < knife_count; i++) {
                        knives[i].x += knives[i].velocity;

                        Rectangle knife_rec = { (float)knives[i].x, (float)knives[i].y, (float)knives[i].width, (float)knives[i].height };
                        if (CheckCollisionRecs(knife_rec, character_rec)) {
                            gameover = true;
                        }

                        DrawTexture(knifeImage, knives[i].x, knives[i].y, WHITE);
                    }

                    for (int i = 0; i < platform_count; i++) {
                        int imageIndex = platforms[i].imageIndex;
                        DrawTexture(terrainImages[imageIndex], platforms[i].x, platforms[i].y, WHITE);
                    }

                    ui.timer += GetFrameTime();
                    if (ui.timer >= 0.15f) {
                        ui.score++;
                        ui.timer = 0.0f;
                    }
                    EndMode2D();
                    DrawText(TextFormat("Score: %d", ui.score), 10, 10, 40, BLACK);

                    Rectangle knifeRect = { 10, 50, knifeImage.width + 10, knifeImage.height + 10 };

                    float knifeX = knifeRect.x + (knifeRect.width - knifeImage.width) / 2;
                    float knifeY = knifeRect.y + (knifeRect.height - knifeImage.height) / 2;

                    if (GetTime() - last_knife_time > 5) {
                        DrawRectangleRec(knifeRect, GREEN);
                        DrawTexture(knifeImage, knifeX, knifeY, WHITE);
                    }
                    else {
                        DrawRectangleRec(knifeRect, RED);
                        DrawTexture(knifeImage, knifeX, knifeY, WHITE);
                    }
                }
                else {
                    if (ui.score > ui.highscore) ui.highscore = ui.score;

                    DrawRectangle(0, 0, window_width, window_height, BLACK);

                    static float fadeAlpha = 0.0f;
                    static bool fadeIn = true;
                    static float fadeTimer = 0.0f;

                    if (fadeIn) {
                        fadeAlpha += GetFrameTime() / 2.0f;
                        if (fadeAlpha >= 1.0f) {
                            fadeAlpha = 1.0f;
                            fadeIn = false;
                        }
                    }
                    else {
                        fadeAlpha -= GetFrameTime() / 2.0f; 
                        if (fadeAlpha <= 0.0f) {
                            fadeAlpha = 0.0f;
                        }
                    }

                    DrawTexture(theEndImage, (window_width - theEndImage.width) / 2, (window_height - theEndImage.height) / 2, Fade(WHITE, fadeAlpha));

                    if (!fadeIn && fadeAlpha == 0.0f) {
                        StopSound(oghneya);

                        DrawText("Game Over", window_width / 2 - 100, window_height / 2 - 50, 40, RED);
                        DrawText(TextFormat("Score: %d", ui.score), window_width / 2 - 100, window_height / 2, 20, WHITE);
                        DrawText(TextFormat("High Score: %d", ui.highscore), window_width / 2 - 100, window_height / 2 + 25, 20, WHITE);

                        Rectangle playAgainButton = { window_width / 2 - 100, window_height / 2 + 70, 200, 50 };
                        DrawRectangleRounded(playAgainButton, 0.2f, 10, WHITE);
                        DrawText("Play Again", window_width / 2 - 50, window_height / 2 + 85, 20, BLACK);

                        Rectangle menuButton = { window_width / 2 - 100, window_height / 2 + 140, 200, 50 };
                        DrawRectangleRounded(menuButton, 0.2f, 10, WHITE); 
                        DrawText("Menu", window_width / 2 - 30, window_height / 2 + 155, 20, BLACK);

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            Vector2 mousePoint = GetMousePosition();
                            if (CheckCollisionPointRec(mousePoint, playAgainButton)) {
                                gameover = false; 
                                ResetGame(character, enemy, ui, window_height); 
                                knife_count = 0; 
                                last_knife_time = 0.0f; 
                                if (!isMuted) PlaySound(clickSFX);
                                fadeAlpha = 0.0f;
                                fadeIn = true;
                                countdownTime = 3.0f; 
                            }
                            else if (CheckCollisionPointRec(mousePoint, menuButton)) {
                                if (!isMuted) PlaySound(clickSFX);
                                inSound = false;
                                inMenu = true;
                                gameover = false;
                                StopSound(oghneya);
                                PlaySound(oghneya_music);
                                ResetGame(character, enemy, ui, window_height);
                            }
                        }
                    }

                    camera.offset.x = 0;
                }
            }

            EndDrawing();
        }
    }

    for (int i = 0; i < terrainImageCount; i++) {
        UnloadTexture(terrainImages[i]);
    }
    UnloadTexture(background);
    UnloadTexture(ahmedImage);
    UnloadTexture(ahmedjump);
    UnloadTexture(ahmedwalk);
    UnloadTexture(theEndImage);
    UnloadTexture(samraImage);
    UnloadTexture(samrajump);
    UnloadTexture(samrawalk);
    UnloadTexture(knifeImage);
    UnloadTexture(noura);
    UnloadTexture(karim);
    UnloadTexture(zaki);
    UnloadTexture(omar);
    UnloadTexture(howToPlaySingle);
	UnloadTexture(howToPlayMulti);
    UnloadTexture(notesGuide);
    UnloadSound(yalaBina);
    UnloadSound(oghneya_music);
    UnloadSound(oghneya);
    UnloadSound(countdownAudio);
    UnloadSound(clickSFX);
	UnloadSound(knifeSFX);
    UnloadSound(backgroundMusic);
    UnloadImage(nouraImage);
    UnloadImage(karimImage);
    UnloadImage(zakiImage);
    UnloadImage(omarImage);
    UnloadImage(building1);
	UnloadImage(building2);
	UnloadImage(building3);
	UnloadImage(building4);
	UnloadImage(building5);
    free(platforms);
    free(knives);
    CloseWindow();
    return 0;
}