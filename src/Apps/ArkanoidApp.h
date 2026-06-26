/**
 * @file ArkanoidApp.h
 * @brief Breakout-style game using encoder for paddle control
 */
#pragma once
#include "AppBase.h"

class ArkanoidApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        _hw = &hw;
        _initGame();
        hw.input.resetEncoder(0);
        _oldPos = 0;

        while (1)
        {
            if ((millis() - _frameCount) > 15)
            {
                _update();
                _draw();
                _frameCount = millis();
            }

            hw.input.checkEncoder(false); // no beep, we handle sound ourselves

            if (_ballActive)
            {
                if (hw.input.checkNext()) break;
            }
        }
    }

private:
    Hardware* _hw = nullptr;

    static constexpr int SCREEN_W = 240;
    static constexpr int SCREEN_H = 135;
    static constexpr int MAX_LIFE = 5;
    static constexpr int BRICK_LINES = 5;
    static constexpr int BRICKS_PER_LINE = 20;

    struct Vec2 { float x, y; };
    struct Rect  { float x, y, w, h; };

    Vec2  _playerPos{}, _playerSize{};
    int   _lives = MAX_LIFE;
    Vec2  _ballPos{}, _ballSpeed{};
    int   _ballRadius = 3;
    bool  _ballActive = false;
    bool  _gameOver = false;
    Vec2  _brickSize{};
    bool  _bricks[BRICK_LINES][BRICKS_PER_LINE] = {};
    long  _oldPos = 0;
    uint32_t _frameCount = 0;

    static bool _collision(Vec2 center, float radius, Rect rec)
    {
        float dx = fabsf(center.x - (rec.x + rec.w / 2));
        float dy = fabsf(center.y - (rec.y + rec.h / 2));
        if (dx > rec.w / 2 + radius || dy > rec.h / 2 + radius) return false;
        if (dx <= rec.w / 2 || dy <= rec.h / 2) return true;
        float d = (dx - rec.w / 2) * (dx - rec.w / 2) + (dy - rec.h / 2) * (dy - rec.h / 2);
        return d <= radius * radius;
    }

    void _initGame()
    {
        _brickSize = { (float)SCREEN_W / BRICKS_PER_LINE, 10 };
        _playerSize = { SCREEN_W / 10.0f + 5, 5 };
        _playerPos = { SCREEN_W / 2.0f, SCREEN_H - _playerSize.y - 6.0f };
        _lives = MAX_LIFE;
        _ballPos = { SCREEN_W / 2.0f, SCREEN_H - _playerSize.y };
        _ballSpeed = { 0, 0 };
        _ballActive = false;
        _gameOver = false;

        int offsetY = _brickSize.y / 2;
        for (int i = 0; i < BRICK_LINES; i++)
            for (int j = 0; j < BRICKS_PER_LINE; j++)
                _bricks[i][j] = true;
        (void)offsetY;
    }

    void _update()
    {
        if (_gameOver) { _initGame(); return; }

        // Paddle movement via encoder
        if (_hw->input.enc_pos < _oldPos)
        {
            _playerPos.x -= 6;
            if (_playerPos.x - _playerSize.x / 2 <= 0) _playerPos.x = _playerSize.x / 2;
            _oldPos = _hw->input.enc_pos;
        }
        else if (_hw->input.enc_pos > _oldPos)
        {
            _playerPos.x += 6;
            if (_playerPos.x + _playerSize.x / 2 >= SCREEN_W) _playerPos.x = SCREEN_W - _playerSize.x / 2;
            _oldPos = _hw->input.enc_pos;
        }

        // Launch ball
        if (!_ballActive)
        {
            if (_hw->input.checkNext()) { _ballActive = true; _ballSpeed = { 0, -2 }; }
        }

        if (_ballActive)
        {
            _ballPos.x += _ballSpeed.x;
            _ballPos.y += _ballSpeed.y;
        }
        else
        {
            _ballPos = { _playerPos.x, SCREEN_H - _playerSize.y - _ballRadius * 4.0f };
        }

        // Wall collisions
        if (_ballPos.x + _ballRadius >= SCREEN_W || _ballPos.x - _ballRadius <= 0)
        { _ballSpeed.x *= -1; _hw->buzzer.click(); }
        if (_ballPos.y - _ballRadius <= 0)
        { _ballSpeed.y *= -1; _hw->buzzer.click(); }
        if (_ballPos.y + _ballRadius >= SCREEN_H)
        { _ballSpeed = { 0, 0 }; _ballActive = false; _lives--; }

        // Player collision
        if (_collision(_ballPos, _ballRadius,
                       { _playerPos.x - _playerSize.x / 2, _playerPos.y - _playerSize.y / 2, _playerSize.x, _playerSize.y }))
        {
            if (_ballSpeed.y > 0)
            {
                _ballSpeed.y *= -1;
                _ballSpeed.x = (_ballPos.x - _playerPos.x) / (_playerSize.x / 2) * 2;
            }
            _hw->buzzer.click();
        }

        // Brick collisions
        for (int i = 0; i < BRICK_LINES; i++)
        {
            for (int j = 0; j < BRICKS_PER_LINE; j++)
            {
                if (!_bricks[i][j]) continue;

                float bx = j * _brickSize.x + _brickSize.x / 2;
                float by = i * _brickSize.y + _brickSize.y / 2;

                // Hit below
                if (_ballPos.y - _ballRadius <= by + _brickSize.y / 2 &&
                    _ballPos.y - _ballRadius > by + _brickSize.y / 2 + _ballSpeed.y &&
                    fabsf(_ballPos.x - bx) < _brickSize.x / 2 + _ballRadius * 2.0f / 3 &&
                    _ballSpeed.y < 0)
                { _bricks[i][j] = false; _ballSpeed.y *= -1; _hw->buzzer.click(); }
                // Hit above
                else if (_ballPos.y + _ballRadius >= by - _brickSize.y / 2 &&
                         _ballPos.y + _ballRadius < by - _brickSize.y / 2 + _ballSpeed.y &&
                         fabsf(_ballPos.x - bx) < _brickSize.x / 2 + _ballRadius * 2.0f / 3 &&
                         _ballSpeed.y > 0)
                { _bricks[i][j] = false; _ballSpeed.y *= -1; _hw->buzzer.click(); }
                // Hit left
                else if (_ballPos.x + _ballRadius >= bx - _brickSize.x / 2 &&
                         _ballPos.x + _ballRadius < bx - _brickSize.x / 2 + _ballSpeed.x &&
                         fabsf(_ballPos.y - by) < _brickSize.y / 2 + _ballRadius * 2.0f / 3 &&
                         _ballSpeed.x > 0)
                { _bricks[i][j] = false; _ballSpeed.x *= -1; _hw->buzzer.click(); }
                // Hit right
                else if (_ballPos.x - _ballRadius <= bx + _brickSize.x / 2 &&
                         _ballPos.x - _ballRadius > bx + _brickSize.x / 2 + _ballSpeed.x &&
                         fabsf(_ballPos.y - by) < _brickSize.y / 2 + _ballRadius * 2.0f / 3 &&
                         _ballSpeed.x < 0)
                { _bricks[i][j] = false; _ballSpeed.x *= -1; _hw->buzzer.click(); }
            }
        }

        // Game over check
        if (_lives <= 0) _gameOver = true;
        else
        {
            _gameOver = true;
            for (int i = 0; i < BRICK_LINES; i++)
                for (int j = 0; j < BRICKS_PER_LINE; j++)
                    if (_bricks[i][j]) _gameOver = false;
        }
    }

    void _draw()
    {
        auto& c = _hw->display.canvas;
        c->fillScreen((uint32_t)0xF5C396);

        if (!_gameOver)
        {
            // Paddle
            c->fillRect(_playerPos.x - _playerSize.x / 2, _playerPos.y - _playerSize.y / 2,
                        _playerSize.x, _playerSize.y, (uint32_t)0x754316);
            // Lives
            for (int i = 0; i < _lives; i++)
                c->fillSmoothCircle(10 + _ballRadius * 3 * i, SCREEN_H - _ballRadius * 2,
                                    _ballRadius, (uint32_t)0x754316);
            // Ball
            c->fillSmoothCircle(_ballPos.x, _ballPos.y, _ballRadius, (uint32_t)0x754316);
            // Bricks
            for (int i = 0; i < BRICK_LINES; i++)
                for (int j = 0; j < BRICKS_PER_LINE; j++)
                    if (_bricks[i][j])
                        c->fillRect(j * _brickSize.x + 1, i * _brickSize.y + _brickSize.y / 2,
                                    _brickSize.x - 1, _brickSize.y, (uint32_t)0x754316);
        }

        _hw->display.push();
    }
};
