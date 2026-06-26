/**
 * @file Arkanoid.cpp
 * @brief Breakout-style game using encoder for paddle control
 */
#include "../FactoryTest.h"

#define screen_width 240
#define screen_heigh 135

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define PLAYER_MAX_LIFE 5
#define LINES_OF_BRICKS 5
#define BRICKS_PER_LINE 20

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Vector2
{
    float x;
    float y;
} Vector2;

typedef struct Player
{
    Vector2 position;
    Vector2 size;
    int life;
} Player;

typedef struct Ball
{
    Vector2 position;
    Vector2 speed;
    int radius;
    bool active;
} Ball;

typedef struct Brick
{
    Vector2 position;
    bool active;
} Brick;

typedef struct Rectangle
{
    float x;
    float y;
    float width;
    float height;
} Rectangle;

#define PLAY_HIT_SOUND() _tone(random(200, 600), 30)

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = screen_width;
static const int screenHeight = screen_heigh;

static bool gameOver = false;
static bool __pause = false;

static Player player = {0};
static Ball ball = {0};
static Brick brick[LINES_OF_BRICKS][BRICKS_PER_LINE] = {0};
static Vector2 brickSize = {0};

static uint32_t time_count = 0;
static long __old_pos = 0;

//------------------------------------------------------------------------------------
// Collision detection
//------------------------------------------------------------------------------------
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec)
{
    int recCenterX = (int)(rec.x + rec.width / 2.0f);
    int recCenterY = (int)(rec.y + rec.height / 2.0f);

    float dx = fabsf(center.x - (float)recCenterX);
    float dy = fabsf(center.y - (float)recCenterY);

    if (dx > (rec.width / 2.0f + radius))
        return false;
    if (dy > (rec.height / 2.0f + radius))
        return false;

    if (dx <= (rec.width / 2.0f))
        return true;
    if (dy <= (rec.height / 2.0f))
        return true;

    float cornerDistanceSq =
        (dx - rec.width / 2.0f) * (dx - rec.width / 2.0f) + (dy - rec.height / 2.0f) * (dy - rec.height / 2.0f);

    return (cornerDistanceSq <= (radius * radius));
}

//------------------------------------------------------------------------------------
// Game functions
//------------------------------------------------------------------------------------
void FactoryTest::_arkanoid_start()
{
    _arkanoid_setup();
    while (1)
    {
        _arkanoid_loop();
        _check_encoder(false);
        if (ball.active)
        {
            if (_check_next())
            {
                break;
            }
        }
    }
}

void FactoryTest::_arkanoid_setup()
{
    _InitGame();

    _enc_pos = 0;
    _enc.setPosition(_enc_pos);
}

void FactoryTest::_arkanoid_loop()
{
    if ((millis() - time_count) > 15)
    {
        _UpdateDrawFrame();
        time_count = millis();
    }
}

#define GetScreenWidth() _canvas->width()

// Initialize game variables
void FactoryTest::_InitGame(void)
{
    brickSize = (Vector2){(float)GetScreenWidth() / BRICKS_PER_LINE, 10};

    // Initialize player
    player.size = (Vector2){screenWidth / 10 + 5, 5};
    player.position = (Vector2){screenWidth / 2, screenHeight - player.size.y - 6};
    player.life = PLAYER_MAX_LIFE;

    // Initialize ball
    ball.position = (Vector2){screenWidth / 2, screenHeight - player.size.y};
    ball.speed = (Vector2){0, 0};
    ball.radius = 3;
    ball.active = false;

    // Initialize bricks
    int initialDownPosition = brickSize.y / 2;

    for (int i = 0; i < LINES_OF_BRICKS; i++)
    {
        for (int j = 0; j < BRICKS_PER_LINE; j++)
        {
            brick[i][j].position = (Vector2){j * brickSize.x + brickSize.x / 2, i * brickSize.y + initialDownPosition};
            brick[i][j].active = true;
        }
    }
}

// Update game (one frame)
void FactoryTest::_UpdateGame(void)
{
    if (!gameOver)
    {
        if (!__pause)
        {
            // Player movement via encoder
            if (_enc_pos < __old_pos)
            {
                player.position.x -= 6;
                if ((player.position.x - player.size.x / 2) <= 0)
                    player.position.x = player.size.x / 2;

                __old_pos = _enc_pos;
            }
            else if (_enc_pos > __old_pos)
            {
                player.position.x += 6;
                if ((player.position.x + player.size.x / 2) >= screenWidth)
                    player.position.x = screenWidth - player.size.x / 2;

                __old_pos = _enc_pos;
            }

            // Ball launching logic
            if (!ball.active)
            {
                if (_check_next())
                {
                    ball.active = true;
                    ball.speed = (Vector2){0, -2};
                }
            }

            // Ball movement logic
            if (ball.active)
            {
                ball.position.x += ball.speed.x;
                ball.position.y += ball.speed.y;
            }
            else
            {
                ball.position = (Vector2){player.position.x, screenHeight - player.size.y - ball.radius * 4};
            }

            // Collision logic: ball vs walls
            if (((ball.position.x + ball.radius) >= screenWidth) || ((ball.position.x - ball.radius) <= 0))
            {
                ball.speed.x *= -1;
                PLAY_HIT_SOUND();
            }
            if ((ball.position.y - ball.radius) <= 0)
            {
                ball.speed.y *= -1;
                PLAY_HIT_SOUND();
            }
            if ((ball.position.y + ball.radius) >= screenHeight)
            {
                ball.speed = (Vector2){0, 0};
                ball.active = false;
                player.life--;
            }

            // Collision logic: ball vs player
            if (CheckCollisionCircleRec(ball.position,
                                        ball.radius,
                                        (Rectangle){player.position.x - player.size.x / 2,
                                                    player.position.y - player.size.y / 2,
                                                    player.size.x,
                                                    player.size.y}))
            {
                if (ball.speed.y > 0)
                {
                    ball.speed.y *= -1;
                    ball.speed.x = (ball.position.x - player.position.x) / (player.size.x / 2) * 2;
                }

                PLAY_HIT_SOUND();
            }

            // Collision logic: ball vs bricks
            for (int i = 0; i < LINES_OF_BRICKS; i++)
            {
                for (int j = 0; j < BRICKS_PER_LINE; j++)
                {
                    if (brick[i][j].active)
                    {
                        // Hit below
                        if (((ball.position.y - ball.radius) <= (brick[i][j].position.y + brickSize.y / 2)) &&
                            ((ball.position.y - ball.radius) > (brick[i][j].position.y + brickSize.y / 2 + ball.speed.y)) &&
                            ((fabs(ball.position.x - brick[i][j].position.x)) < (brickSize.x / 2 + ball.radius * 2 / 3)) &&
                            (ball.speed.y < 0))
                        {
                            brick[i][j].active = false;
                            ball.speed.y *= -1;
                            PLAY_HIT_SOUND();
                        }
                        // Hit above
                        else if (((ball.position.y + ball.radius) >= (brick[i][j].position.y - brickSize.y / 2)) &&
                                 ((ball.position.y + ball.radius) <
                                  (brick[i][j].position.y - brickSize.y / 2 + ball.speed.y)) &&
                                 ((fabs(ball.position.x - brick[i][j].position.x)) < (brickSize.x / 2 + ball.radius * 2 / 3)) &&
                                 (ball.speed.y > 0))
                        {
                            brick[i][j].active = false;
                            ball.speed.y *= -1;
                            PLAY_HIT_SOUND();
                        }
                        // Hit left
                        else if (((ball.position.x + ball.radius) >= (brick[i][j].position.x - brickSize.x / 2)) &&
                                 ((ball.position.x + ball.radius) <
                                  (brick[i][j].position.x - brickSize.x / 2 + ball.speed.x)) &&
                                 ((fabs(ball.position.y - brick[i][j].position.y)) < (brickSize.y / 2 + ball.radius * 2 / 3)) &&
                                 (ball.speed.x > 0))
                        {
                            brick[i][j].active = false;
                            ball.speed.x *= -1;
                            PLAY_HIT_SOUND();
                        }
                        // Hit right
                        else if (((ball.position.x - ball.radius) <= (brick[i][j].position.x + brickSize.x / 2)) &&
                                 ((ball.position.x - ball.radius) >
                                  (brick[i][j].position.x + brickSize.x / 2 + ball.speed.x)) &&
                                 ((fabs(ball.position.y - brick[i][j].position.y)) < (brickSize.y / 2 + ball.radius * 2 / 3)) &&
                                 (ball.speed.x < 0))
                        {
                            brick[i][j].active = false;
                            ball.speed.x *= -1;
                            PLAY_HIT_SOUND();
                        }
                    }
                }
            }

            // Game over logic
            if (player.life <= 0)
                gameOver = true;
            else
            {
                gameOver = true;

                for (int i = 0; i < LINES_OF_BRICKS; i++)
                {
                    for (int j = 0; j < BRICKS_PER_LINE; j++)
                    {
                        if (brick[i][j].active)
                            gameOver = false;
                    }
                }
            }
        }
    }
    else
    {
        _InitGame();
        gameOver = false;
    }
}

// Draw game (one frame)
void FactoryTest::_DrawGame(void)
{
    _canvas->fillScreen((uint32_t)0xF5C396);

    if (!gameOver)
    {
        // Draw player bar
        _canvas->fillRect(player.position.x - player.size.x / 2,
                          player.position.y - player.size.y / 2,
                          player.size.x,
                          player.size.y,
                          (uint32_t)0x754316);

        // Draw player lives
        for (int i = 0; i < player.life; i++)
            _canvas->fillSmoothCircle(
                10 + (ball.radius * 3) * i, screenHeight - ball.radius * 2, ball.radius, (uint32_t)0x754316);

        // Draw ball
        _canvas->fillSmoothCircle(ball.position.x, ball.position.y, ball.radius, (uint32_t)0x754316);

        // Draw bricks
        for (int i = 0; i < LINES_OF_BRICKS; i++)
        {
            for (int j = 0; j < BRICKS_PER_LINE; j++)
            {
                if (brick[i][j].active)
                {
                    _canvas->fillRect(brick[i][j].position.x - brickSize.x / 2,
                                      brick[i][j].position.y - brickSize.y / 2,
                                      brickSize.x,
                                      brickSize.y,
                                      (uint32_t)0x754316);
                }
            }
        }
    }

    _canvas->pushSprite(0, 0);
}

// Unload game variables
void FactoryTest::_UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data
}

// Update and Draw (one frame)
void FactoryTest::_UpdateDrawFrame(void)
{
    _UpdateGame();
    _DrawGame();
}
