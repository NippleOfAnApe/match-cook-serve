#include "include/raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
// Tile collision types
#define EMPTY   -1
#define BLOCK    0     // Start from zero, slopes can be added

// Defined map size
#define TILE_MAP_WIDTH  30
#define TILE_MAP_HEIGHT 12

#define MAX_COINS       11

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Object storing inputs for Entity
typedef struct {
    float right;
    float left;
    float up;
    float down;
    bool jump;
} Input;

// Physics body moving around
typedef struct {
    int width;
    int height;

    Vector2 position;
    float direction;
    float maxSpd;
    float acc;
    float dcc;
    float gravity;
    float jumpImpulse;
    float jumpRelease;
    Vector2 velocity;
    // Carry stored subpixel values
    float hsp;
    float vsp;

    bool isGrounded;
    bool isJumping;
    // Flags for detecting collision
    bool hitOnFloor;
    bool hitOnCeiling;
    bool hitOnWall;

    Input *control;
} Entity;

// Coin object
typedef struct {
    Vector2 position;
    bool visible;
} Coin;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
const int TILE_SIZE = 16;
const int TILE_SHIFT = 4;   // Used in bitshift  | bit of TILE_SIZE
const int TILE_ROUND = 15;  // Used in bitwise operation | TILE_SIZE - 1

float screenScale;
int screenWidth;
static int screenHeight;

static float delta;
static bool win = false;
static int score = 0;

static int tiles [TILE_MAP_WIDTH*TILE_MAP_HEIGHT];
static Entity player = { 0 };
static Input input = {false, false, false, false, false};
static Camera2D camera = {0};

// Create coin instances
static Coin coins[MAX_COINS] = {
    {(Vector2){1*16+6,7*16+6}, true},
    {(Vector2){3*16+6,5*16+6}, true},
    {(Vector2){5*16+6,5*16+6}, true},
    {(Vector2){8*16+6,3*16+6}, true},
    {(Vector2){9*16+6,3*16+6}, true},
    {(Vector2){13*16+6,4*16+6}, true},
    {(Vector2){14*16+6,4*16+6}, true},
    {(Vector2){15*16+6,4*16+6}, true},
    {(Vector2){19*16+6,5*16+6}, true},
    {(Vector2){20*16+6,5*16+6}, true},
    {(Vector2){25*16+6,3*16+6}, true},
};

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

//------------------------------------------------------------------------------------
// Movement Functions Declaration (local)
//------------------------------------------------------------------------------------
static void EntityMoveUpdate(Entity *intance);
static void GetDirection(Entity *instance);
static void GroundCheck(Entity *instance);
static void MoveCalc(Entity *instance);
static void GravityCalc(Entity *instance);
static void CollisionCheck(Entity *instance);
static void CollisionHorizontalBlocks(Entity *instance);
static void CollisionVerticalBlocks(Entity *instance);

//------------------------------------------------------------------------------------
// Tile Functions Declaration (local)
//------------------------------------------------------------------------------------
static int MapGetTile(int x, int y);
static int MapGetTileWorld(int x, int y);
static int TileHeight(int x, int y, int tile);

static void MapInit(void);
static void MapDraw(void);
static void PlayerInit(void);
static void InputUpdate(void);
static void PlayerUpdate(void);
static void PlayerDraw(void);
static void CoinInit(void);
static void CoinUpdate(void);
static void CoinDraw(void);

//------------------------------------------------------------------------------------
// Utility Functions Declaration (local)
//------------------------------------------------------------------------------------
int ttc_sign(float x);
float ttc_abs(float x);
float ttc_clamp(float value, float min, float max);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    screenScale = 4.0;
    //screenHeight = 720;
    screenWidth = 1280;

    //screenWidth = TILE_SIZE*TILE_MAP_WIDTH*(int)screenScale;
    screenHeight = TILE_SIZE*TILE_MAP_HEIGHT*(int)screenScale;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Platformer prototype");
    InitGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
        camera.target = player.position;
        //----------------------------------------------------------------------------------
    }
#endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame();         // Unload loaded data (textures, sounds, models...)

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------
// Initialize game variables
void InitGame(void)
{
    win = false;
    score = 0;
    
    camera.offset = (Vector2){0.0, 0.0};
    camera.target = player.position;
    camera.rotation = 0.0f;
    camera.zoom = screenScale;
    
    MapInit();
    PlayerInit();
    CoinInit();
}

// Update game (one frame)
void UpdateGame(void)
{
    // Get time since last frame
    delta = GetFrameTime();

    PlayerUpdate();
    CoinUpdate();

    // If all coins are collected
    if (win)
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
        }
    }
}

// Draw game (every frame)
void DrawGame(void)
{
    BeginDrawing();
    
        BeginMode2D(camera);
            ClearBackground(RAYWHITE);

            // Draw game
            MapDraw();
            CoinDraw();
            PlayerDraw();


        EndMode2D();
        
        DrawText(TextFormat("SCORE: %i", score), GetScreenWidth()/2 - MeasureText(TextFormat("SCORE: %i", score), 40)/2, 50, 40, BLACK);

        if (win) DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth()/2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20)/2, GetScreenHeight()/2 - 50, 20, GRAY);

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload required assets here
}

void MapInit(void)
{
    // Set tiles as borders
    for (int y = 0; y < TILE_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < TILE_MAP_WIDTH; x++)
        {
            // Solid tiles
            if (y == 0 || x == 0 || y == TILE_MAP_HEIGHT-1 || x == TILE_MAP_WIDTH-1)
            {
                tiles[x+y*TILE_MAP_WIDTH] = BLOCK;
            }
            else    // Empty tiles
            {
                tiles[x+y*TILE_MAP_WIDTH] = EMPTY;
            }
        }
    }

    // Manual cell population for platforms
    tiles[3 + 8*TILE_MAP_WIDTH] = BLOCK;
    tiles[4 + 8*TILE_MAP_WIDTH] = BLOCK;
    tiles[5 + 8*TILE_MAP_WIDTH] = BLOCK;

    tiles[8 + 6*TILE_MAP_WIDTH] = BLOCK;
    tiles[9 + 6*TILE_MAP_WIDTH] = BLOCK;
    tiles[10 + 6*TILE_MAP_WIDTH] = BLOCK;

    tiles[13 + 7*TILE_MAP_WIDTH] = BLOCK;
    tiles[14 + 7*TILE_MAP_WIDTH] = BLOCK;
    tiles[15 + 7*TILE_MAP_WIDTH] = BLOCK;

    tiles[19 + 8*TILE_MAP_WIDTH] = BLOCK;
    tiles[20 + 8*TILE_MAP_WIDTH] = BLOCK;

    tiles[23 + 6*TILE_MAP_WIDTH] = BLOCK;
    tiles[24 + 6*TILE_MAP_WIDTH] = BLOCK;
    tiles[25 + 6*TILE_MAP_WIDTH] = BLOCK;

    tiles[1 + 10*TILE_MAP_WIDTH] = BLOCK;
}

void MapDraw(void)
{
    // Parse through tile map and draw rectangles to visualize it
    for (int y = 0; y < TILE_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < TILE_MAP_WIDTH; x++)
        {
            // Draw tiles
            if ( tiles[x+y*TILE_MAP_WIDTH] > EMPTY)
            {
                DrawRectangle(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
            }
        }
    }
}

// Function to get tile index using world coordinates
int MapGetTileWorld(int x, int y)
{
    // Returns tile ID using world position
    x /= TILE_SIZE;
    y /= TILE_SIZE;
    
    if (x < 0 || x > TILE_MAP_WIDTH || y < 0 || y > TILE_MAP_HEIGHT) return EMPTY;

    return tiles[x+y*TILE_MAP_WIDTH];
}

// Function to get tile index using tile map coordinates
int MapGetTile(int x, int y)
{
    // Returns tile ID using tile position withing tile map
    if (x < 0 || x > TILE_MAP_WIDTH || y < 0 || y > TILE_MAP_HEIGHT) return EMPTY;

    return tiles[x+y*TILE_MAP_WIDTH];
}

// Returns one pixel above the tile in world coordinates
int TileHeight(int x, int y, int tile)
{
    // Returns one pixel above solid. Extendable for slopes.
    switch(tile)
    {
        case EMPTY: break;
        case BLOCK: y = (y & ~TILE_ROUND) -1; break;
    }
    
    return y;
}

// Update player controlled Input instance
void InputUpdate(void)
{
    input.right = (float)(IsKeyDown('D') || IsKeyDown(KEY_RIGHT));
    input.left = (float)(IsKeyDown('A') || IsKeyDown(KEY_LEFT));
    input.up = (float)(IsKeyDown('W') || IsKeyDown(KEY_UP));
    input.down = (float)(IsKeyDown('S') || IsKeyDown(KEY_DOWN));

    // For jumping button needs to be toggled - allows pre-jump buffered (if held, jumps as soon as lands)
    if (IsKeyPressed(KEY_SPACE)) input.jump = true;
    else if (IsKeyReleased(KEY_SPACE)) input.jump = false;
}

// Init player controlled instance
void PlayerInit(void)
{
    player.position.x = (float)(TILE_SIZE*TILE_MAP_WIDTH)*0.5;
    player.position.y = TILE_MAP_HEIGHT*TILE_SIZE - 16.0 -1;
    player.direction = 1.0;

    player.maxSpd = 1.5625f*60;
    player.acc = 0.218164f*60*60;
    player.dcc =  0.113281f*60*60;
    player.gravity = 0.363281f*60*60;
    player.jumpImpulse = -6.5625f*60;
    player.jumpRelease = player.jumpImpulse*0.2f;
    player.velocity = (Vector2){ 0.0, 0.0 };
    player.hsp = 0;
    player.vsp = 0;

    player.width = 12;
    player.height = 12;

    player.isGrounded = false;
    player.isJumping = false;

    // Assign Input instance used by player
    player.control = &input;
}

// Draw player representing rectangle so the position is at bottom middle
void PlayerDraw(void)
{
    DrawRectangle(player.position.x - player.width*0.5, player.position.y-player.height +1, player.width, player.height, MAROON);
}

// Player's instance update
void PlayerUpdate(void)
{
    InputUpdate();
    EntityMoveUpdate(&player);
}

// Reset coin visibility
void CoinInit(void)
{
    for (int i=0; i<MAX_COINS; i++) coins[i].visible = true;
}

// Draw each coin
void CoinDraw(void)
{
    for (int i=0; i<MAX_COINS; i++)
    {
        if (coins[i].visible) DrawRectangle((int)coins[i].position.x, (int)coins[i].position.y, 4.0, 4.0, GOLD);
    }
}

// Collision check each coin
void CoinUpdate(void)
{
    Rectangle playerRect = (Rectangle){ player.position.x - player.width*0.5, player.position.y-player.height +1, player.width, player.height };
    
    for (int i = 0; i < MAX_COINS; i++)
    {
        if (coins[i].visible)
        {
            Rectangle coinRect = (Rectangle){ coins[i].position.x, coins[i].position.y, 4.0, 4.0 };
            
            if (CheckCollisionRecs(playerRect, coinRect))
            {
                coins[i].visible = false;
                score += 1;
            }
        }
    }
    
    win = score == MAX_COINS;
}

//------------------------------------------------
// Physics functions
//------------------------------------------------

// Main Entity movement calculation
void EntityMoveUpdate(Entity *instance)
{
    GroundCheck(instance);
    GetDirection(instance);
    MoveCalc(instance);
    GravityCalc(instance);
    CollisionCheck(instance);

    // Horizontal velocity together including last frame sub-pixel value
    float xVel = instance->velocity.x*delta + instance->hsp;
    // Horizontal velocity in pixel values
    int xsp = (int)ttc_abs(xVel)*ttc_sign(xVel);
    // Save horizontal velocity sub-pixel value for next frame
    instance->hsp = instance->velocity.x*delta - xsp;

    // Vertical velocity together including last frame sub-pixel value
    float yVel = instance->velocity.y*delta + instance->vsp;
    // Vertical velocity in pixel values
    int ysp = (int)ttc_abs(yVel)*ttc_sign(yVel);
    // Save Vertical velocity sub-pixel value for next frame
    instance->vsp = instance->velocity.y*delta - ysp;

    // Add pixel value velocity to the position
    instance->position.x += xsp;
    instance->position.y += ysp;

    // Prototyping Safety net - keep in view
    instance->position.x = ttc_clamp(instance->position.x, 0.0, TILE_MAP_WIDTH*(float)TILE_SIZE);
    instance->position.y = ttc_clamp(instance->position.y, 0.0, TILE_MAP_HEIGHT*(float)TILE_SIZE);
}

// Read Input for horizontal movement direction
void GetDirection(Entity *instance)
{
    instance->direction = (instance->control->right - instance->control->left);
}

// Check pixel bellow to determine if Entity is grounded
void GroundCheck(Entity *instance)
{
    int x = (int)instance->position.x;
    int y = (int)instance->position.y + 1;
    instance->isGrounded = false;

    // Center point check
    int c = MapGetTile(x >> TILE_SHIFT, y >> TILE_SHIFT);
    
    if (c != EMPTY)
    {
        int h = TileHeight(x, y, c);
        instance->isGrounded = (y >= h);
    }
    
    if (!instance->isGrounded)
    {
        // Left bottom corner check
        int xl = (x - instance->width / 2);
        int l = MapGetTile(xl >> TILE_SHIFT, y >> TILE_SHIFT);
        
        if (l != EMPTY)
        {
            int h = TileHeight(xl, y, l);
            instance->isGrounded = (y >= h);
        }
        
        if (!instance->isGrounded)
        {
            // Right bottom corner check
            int xr = (x + instance->width / 2 - 1);
            int r = MapGetTile(xr >> TILE_SHIFT, y >> TILE_SHIFT);
            if (r != EMPTY)
            {
                int h = TileHeight(xr, y, r);
                instance->isGrounded = (y >= h);
            }
        }
    }
}

// Simplified horizontal acceleration / deacceleration logic
void MoveCalc(Entity *instance)
{
    // Check if direction value is above dead zone - direction is held
    float deadZone = 0.0;
    if (ttc_abs(instance->direction) > deadZone)
    {
        instance->velocity.x += instance->direction*instance->acc*delta;
        instance->velocity.x = ttc_clamp(instance->velocity.x, -instance->maxSpd, instance->maxSpd);
    }
    else
    {
        // No direction means deacceleration
        float xsp = instance->velocity.x;
        if (ttc_abs(0 - xsp) < instance->dcc*delta) instance->velocity.x = 0;
        else if (xsp > 0) instance->velocity.x -= instance->dcc*delta;
        else instance->velocity.x += instance->dcc*delta;
    }
}

// Set values when jump is activated
void Jump(Entity *instance)
{
    instance->velocity.y = instance->jumpImpulse;
    instance->isJumping = true;
    instance->isGrounded = false;
}

// Gravity calculation and Jump detection
void GravityCalc(Entity *instance)
{
    if (instance->isGrounded)
    {
        if (instance->isJumping)
        {
            instance->isJumping = false;
            instance->control->jump = false;    // Cancel input button
        }
        else if (!instance->isJumping && instance->control->jump)
        {
            Jump(instance);
        }
    }
    else
    {
        if (instance->isJumping)
        {
            if (!instance->control->jump)
            {
                instance->isJumping = false;
                
                if (instance->velocity.y < instance->jumpRelease)
                {
                    instance->velocity.y = instance->jumpRelease;
                }
            }
        }
    }
    
    // Add gravity
    instance->velocity.y += instance->gravity*delta;
    
    // Limit falling to negative jump value
    if (instance->velocity.y > -instance->jumpImpulse)
    {
        instance->velocity.y = -instance->jumpImpulse;
    }
}

// Main collision check function
void CollisionCheck(Entity *instance)
{
    CollisionHorizontalBlocks(instance);
    CollisionVerticalBlocks(instance);
}

// Detect and solve horizontal collision with block tiles
void CollisionHorizontalBlocks(Entity *instance)
{
    // Get horizontal speed in pixels
    float xVel = instance->velocity.x*delta + instance->hsp;
    int xsp = (int)ttc_abs(xVel)*ttc_sign(xVel);

    instance->hitOnWall = false;

    // Get bounding box side offset
    int side;
    if (xsp > 0) side = instance->width / 2 - 1;
    else if (xsp < 0) side = -instance->width / 2;
    else return;

    int x = (int)instance->position.x;
    int y = (int)instance->position.y;
    int mid = -instance->height / 2;
    int top = -instance->height + 1;

    // 3 point check
    int b = MapGetTile((x + side + xsp) >> TILE_SHIFT, y >> TILE_SHIFT) > EMPTY;
    int m = MapGetTile((x + side + xsp) >> TILE_SHIFT, (y + mid) >> TILE_SHIFT) > EMPTY;
    int t = MapGetTile((x + side + xsp) >> TILE_SHIFT, (y + top) >> TILE_SHIFT) > EMPTY;
    
    // If implementing slopes it's better to disable b and m, if (x,y) is in the slope tile
    if (b || m || t)
    {
        if (xsp > 0) x = ((x + side + xsp) & ~TILE_ROUND) - 1 - side;
        else x = ((x + side + xsp) & ~TILE_ROUND) + TILE_SIZE - side;

        instance->position.x = (float)x;
        instance->velocity.x = 0.0;
        instance->hsp = 0.0;

        instance->hitOnWall = true;
    }
}

// Detect and solve vertical collision with block tiles
void CollisionVerticalBlocks(Entity *instance)
{
    // Get vertical speed in pixels
    float yVel = instance->velocity.y*delta + instance->vsp;
    int ysp = (int)ttc_abs(yVel)*ttc_sign(yVel);
    instance->hitOnCeiling = false;
    instance->hitOnFloor = false;

    // Get bounding box side offset
    int side = 0;
    if (ysp > 0) side = 0;
    else if (ysp < 0) side = -instance->height + 1;
    else return;

    int x = (int)instance->position.x;
    int y = (int)instance->position.y;
    int xl = -instance->width/2;
    int xr = instance->width/2 - 1;

    int c = MapGetTile(x >> TILE_SHIFT, (y + side + ysp) >> TILE_SHIFT) > EMPTY;
    int l = MapGetTile((x + xl) >> TILE_SHIFT, (y + side + ysp) >> TILE_SHIFT) > EMPTY;
    int r = MapGetTile((x + xr) >> TILE_SHIFT, (y + side + ysp) >> TILE_SHIFT) > EMPTY;
    
    if (c || l || r)
    {
        if (ysp > 0)
        {
            y = ((y + side + ysp) & ~TILE_ROUND) - 1 - side;
            instance->hitOnFloor = true;
        }
        else
        {
            y = ((y + side + ysp) & ~TILE_ROUND) + TILE_SIZE - side;
            instance->hitOnCeiling = true;
        }
        
        instance->position.y = (float)y;
        instance->velocity.y = 0.0;
        instance->vsp = 0.0;
    }
}

// Return sign of the floal as int (-1, 0, 1)
int ttc_sign(float x)
{
    if (x < 0) return -1;
    else if (x < 0.0001) return 0;
    else return 1;
}

// Return absolute value of float
float ttc_abs(float x)
{
    if (x < 0.0) x *= -1.0;
    return x;
}

// Clamp value between min and max
float ttc_clamp(float value, float min, float max)
{
    const float res = value < min ? min : value;
    return res > max ? max : res;
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}

