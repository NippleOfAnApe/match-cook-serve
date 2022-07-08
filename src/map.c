#include "raylib.h"
#include "mapObjects.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static Texture2D bgTexture = { 0 };
static Texture2D fgTexture = { 0 };
static Texture2D wallTexture = { 0 };
static Texture2D raspberryTexture = { 0 };
static Texture2D pineapleTexture = { 0 };
static Texture2D sushiTexture = { 0 };
static Texture2D pizzaTexture = { 0 };

Food fruits[FOOD_ITEMS] = { 0 };

//Map dimensions
const int mapWidth = 5000;
const int mapHeight = 1000;
int borderWidth = 40;
int offMapSize = 110; //how many blocks to fit outside the map in the screen when near borders

static float minusFoodLifetime = 8.0f;
static float bonusFoodLifetime = 10.0f;
static float regularFoodLifetime = 40.0f;
static int minusFruitPoints = 50;
static int bonusFruitPoints = 10;
static int regularFruitPoints = 2;
static float minusFruitScale = .8f;
static float bonusFruitScale = 1.2f;
static float regularFruitScale = .6f;
static int minusFruitTailIncrease = -5;
static int bonusFruitTailIncrease = 5;
static int regularFruitTailIncrease = 1;
static int theExtra = 0;    // extra space needed for drawing bg and fg

//----------------------------------------------------------------------------------
// Map related Functions Definition
//----------------------------------------------------------------------------------
void InitMap(void)
{
    // mapWidth = 5000;
    // mapHeight = 3000;
    for (int i = 0; i < FOOD_ITEMS; i++) fruits[i].active = false;

    bgTexture = LoadTexture("../resources/dirtSIZE.png");
    wallTexture = LoadTexture("../resources/stone480.png");
    fgTexture = LoadTexture("../resources/03grass1024.png");
    
    raspberryTexture = LoadTexture("../resources/raspberry64.png");
    pineapleTexture = LoadTexture("../resources/pineaple64.png");
    sushiTexture = LoadTexture("../resources/sushi64.png");
    pizzaTexture = LoadTexture("../resources/pizza64.png");

    theExtra = borderWidth * 2 + offMapSize * 2;
}

void CalcFruitPos(void)
{
    for (int i = 0; i < FOOD_ITEMS; i++)
    {
        if (fruits[i].lifetime <= 0) fruits[i].active = false;
        if (!fruits[i].active)
        {
            fruits[i].active = true;
            int randomValue = GetRandomValue(1, 40);
            //MinusFruit
            if (randomValue % 20 == 0)
            {
                fruits[i].scale = minusFruitScale;
                fruits[i].foodTexture = &pizzaTexture;
                fruits[i].position = (Vector2){ GetRandomValue(64, mapWidth - 64), GetRandomValue(64, (mapHeight - 64) - 2)};
                fruits[i].points = minusFruitPoints;
                fruits[i].tailIncreaseSize = minusFruitTailIncrease;
                fruits[i].lifetime = minusFoodLifetime;
            }
            //Fast fruit
            else if (randomValue % 10 == 0) 
            {
                fruits[i].scale = .5f;
                fruits[i].foodTexture = &sushiTexture;
                fruits[i].position = (Vector2){ GetRandomValue(64, mapWidth - 64), GetRandomValue(64, (mapHeight - 64) - 2)};
                fruits[i].points = bonusFruitPoints;
                fruits[i].tailIncreaseSize = bonusFruitTailIncrease + 5;
                fruits[i].lifetime = bonusFoodLifetime;
            }
            //Bonus fruit
            else if (randomValue % 5 == 0) 
            {
                fruits[i].scale = bonusFruitScale;
                fruits[i].foodTexture = &pineapleTexture;
                fruits[i].position = (Vector2){ GetRandomValue(64, mapWidth - 64), GetRandomValue(64, (mapHeight - 64) - 2)};
                fruits[i].points = bonusFruitPoints;
                fruits[i].tailIncreaseSize = bonusFruitTailIncrease;
                fruits[i].lifetime = bonusFoodLifetime;
            }
            //Main fruit
            else
            {
                fruits[i].scale = regularFruitScale;
                fruits[i].foodTexture = &raspberryTexture;
                fruits[i].position = (Vector2){ GetRandomValue(64, mapWidth - 64), GetRandomValue(64, (mapHeight - 64) - 2)};
                fruits[i].points = regularFruitPoints;
                fruits[i].tailIncreaseSize = regularFruitTailIncrease;
                fruits[i].lifetime = regularFoodLifetime + regularFoodLifetime * GetRandomValue(-10, 10) / 20;
            }
            
            if (FruitIsOnSnake(fruits[i]))
            fruits[i].position = (Vector2){ GetRandomValue(64, mapWidth - 64), GetRandomValue(64, (mapHeight - 64) - 2)};
            
        }
        fruits[i].lifetime -= GetFrameTime();
    }
}

void DrawMap(void)
{
    // BG and FG
    DrawTextureTiled(bgTexture, (Rectangle){0.0f, 0.0f, 1920.0f, 1280.0f}, (Rectangle){-offMapSize - borderWidth, -offMapSize - borderWidth, mapWidth + theExtra, mapHeight + theExtra}, (Vector2){0.0f, 0.0f}, 0.0f, 1.0f, WHITE);
    DrawTextureTiled(fgTexture, (Rectangle){0.0f, 0.0f, 1024.0f, 1024.0f}, (Rectangle){0.0f, 0.0f, mapWidth, mapHeight}, (Vector2){0.0f, 0.0f}, 0.0f, 1.6f, WHITE);

    // Borders
    DrawTextureTiled(wallTexture, (Rectangle){0.0f, 0.0f, 480.0f, 480.0f}, (Rectangle){-borderWidth, -borderWidth, mapWidth + borderWidth, borderWidth}, (Vector2){0.0f, 0.0f}, 0.0f, .5f, WHITE);
    DrawTextureTiled(wallTexture, (Rectangle){0.0f, 0.0f, 480.0f, 480.0f}, (Rectangle){-borderWidth, 0, borderWidth, mapHeight}, (Vector2){0.0f, 0.0f}, 0.0f, .5f, WHITE);
    DrawTextureTiled(wallTexture, (Rectangle){0.0f, 0.0f, 480.0f, 480.0f}, (Rectangle){-borderWidth, mapHeight, mapWidth + borderWidth, borderWidth}, (Vector2){0.0f, 0.0f}, 0.0f, .5f, WHITE);
    DrawTextureTiled(wallTexture, (Rectangle){0.0f, 0.0f, 480.0f, 480.0f}, (Rectangle){mapWidth, -borderWidth, borderWidth, mapHeight + borderWidth * 2}, (Vector2){0.0f, 0.0f}, 0.0f, .5f, WHITE);
    
    // Draw fruit to pick
    for (int i = 0; i < FOOD_ITEMS; i++)
    {
        DrawTextureEx(*fruits[i].foodTexture, (Vector2){fruits[i].position.x - 32 * fruits[i].scale, fruits[i].position.y - 32 * fruits[i].scale}, 0, fruits[i].scale, WHITE);
        DrawCircleLines(fruits[i].position.x, fruits[i].position.y, 32 * fruits[i].scale, RED);
    }
}

void UpdateCameraCenterInsideMap(Camera2D *camera, int screenWidth, int screenHeight)
{
    camera->target = snake[0].position;
    camera->offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    float minX = -borderWidth - offMapSize;
    float minY = -borderWidth - offMapSize;
    float maxX = mapWidth + borderWidth + offMapSize;
    float maxY = mapHeight + borderWidth + offMapSize;

    Vector2 max = GetWorldToScreen2D((Vector2){ maxX, maxY }, *camera);
    Vector2 min = GetWorldToScreen2D((Vector2){ minX, minY }, *camera);
    
    if (max.x < screenWidth) camera->offset.x = screenWidth - (max.x - screenWidth/2.0f);
    if (max.y < screenHeight) camera->offset.y = screenHeight - (max.y - screenHeight/2.0f);
    if (min.x > 0) camera->offset.x = screenWidth/2.0f - min.x;
    if (min.y > 0) camera->offset.y = screenHeight/2.0f - min.y;

    if (camera->zoom > 1.4f) camera->zoom = 1.4f;
    if (camera->zoom < .7f) camera->zoom = .7f;
}

void UnloadMap(void)
{
    UnloadTexture(bgTexture);
    UnloadTexture(fgTexture);
    UnloadTexture(wallTexture);
    UnloadTexture(raspberryTexture);
    UnloadTexture(pineapleTexture);
    UnloadTexture(sushiTexture);
    UnloadTexture(pizzaTexture);
}
