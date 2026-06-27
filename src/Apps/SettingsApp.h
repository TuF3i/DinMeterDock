/**
 * @file SettingsApp.h
 * @brief Settings sub-menu with vertical row layout using SmoothSelector
 */
#pragma once
#include "AppBase.h"
#include "DisplayApp.h"
#include "BrightnessApp.h"
#include "EncoderApp.h"
#include "BuzzerVolumeApp.h"
#include "SleepWakeupApp.h"
#include "PowerOffApp.h"
#include <smooth_ui_toolkit.h>
#include <functional>

class SettingsApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        // Init display
        hw.display.canvas->setFont(&fonts::efontCN_24);
        hw.display.canvas->setTextSize(1);

        // Track encoder from current position
        hw.input.resetEncoder(hw.input.enc.getPosition());

        // Create menu
        SettingsMenuCore menu;
        menu.hw = &hw;
        menu.items = _settings;
        menu.itemCount = SETTING_COUNT;

        // Row layout: 6 items need scrolling (25px title + 6*21 = 151 > 135)
        constexpr int rowY[SETTING_COUNT] = {28, 49, 70, 91, 112, 133, 154};
        constexpr int rowH = 20;

        // Add options with row keyframes
        for (int i = 0; i < SETTING_COUNT; i++)
        {
            SmoothUIToolKit::SelectMenu::SmoothSelector::OptionProps_t opt;
            opt.keyframe = {0, rowY[i], 240, rowH};
            opt.userData = (void*)&_settings[i];
            menu.addOption(opt);
        }

        // Set initial selected row offset
        menu.setSelectedRowOffset(0);

        // Sync encoder position to avoid initial jump (Issue 3)
        menu.lastEncPos = hw.input.enc.getPosition();

        // Configure
        menu.setConfig().renderInterval    = 20;
        menu.setConfig().readInputInterval = 50;
        menu.setConfig().moveInLoop        = false;

        // Camera for scrolling (visible area below 25px title bar, height 110px)
        menu.setCameraSize(240, 110);
        menu.getCamera().jumpTo(0, 0);
        menu.getCamera().setDuration(300);
        menu.getCamera().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutBack);

        menu.getSelectorPostion().setDuration(300);
        menu.getSelectorPostion().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutBack);
        menu.getSelectorShape().setDuration(300);
        menu.getSelectorShape().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutBack);

        // Render initial frame to avoid flash
        menu.onRender();

        // Main loop
        while (1)
        {
            menu.update(millis());

            if (menu.goBack)
            {
                menu.resetAllRowOffsets();
                break;
            }
        }

        // Restore encoder position for main menu
        hw.input.enc.setPosition(menu.lastEncPos);
    }

private:
    static constexpr int SETTING_COUNT = 7;

    struct SubAppEntry
    {
        const char* name;
        uint32_t    themeColor;
        uint32_t    textColor;
        std::function<void(Hardware&)> runner;  // nullptr = Back option
    };

    const SubAppEntry _settings[SETTING_COUNT] = {
        {"Display Test",   0xB8DBD9, 0x385B59, [](Hardware& h) { DisplayApp().run(h); }},
        {"Brightness",     0x87C38F, 0x07430F, [](Hardware& h) { BrightnessApp().run(h); }},
        {"Encoder Test",   0x6AB8A0, 0x163820, [](Hardware& h) { EncoderApp().run(h); }},
        {"Sleep & Wakeup", 0xC6D5EF, 0x46556F, [](Hardware& h) { SleepWakeupApp().run(h); }},
        {"Buzzer Volume",  0x8B6914, 0x5C4508, [](Hardware& h) { BuzzerVolumeApp().run(h); }},
        {"Power Off",      0xCEDBB8, 0x4E5B38, [](Hardware& h) { PowerOffApp().run(h); }},
        {"\xe2\x86\x90 Back", 0x888888, 0xDDDDDD, nullptr},  // ← Back, runner=null
    };

    // --- SmoothSelector subclass ---
    class SettingsMenuCore : public SmoothUIToolKit::SelectMenu::SmoothSelector
    {
    public:
        Hardware*          hw = nullptr;
        const SubAppEntry* items = nullptr;
        int                itemCount = 0;
        int                lastEncPos = 0;
        bool               waitButtonRelease = false;
        bool               _btnPressing = false;
        uint32_t           pressStartTime = 0;
        bool               goBack = false;

        void setSelectedRowOffset(int idx)
        {
            _data.option_list[idx].keyframe.x = 12;
        }

        void resetAllRowOffsets()
        {
            for (size_t i = 0; i < _data.option_list.size(); i++)
                _data.option_list[i].keyframe.x = 0;
        }

        void onReadInput() override
        {
            if (isOpening()) return;

            auto& h = *hw;

            // --- Button handling ---
            if (!h.input.btn_pwr.read())  // LOW = pressed
            {
                if (!waitButtonRelease)
                {
                    h.buzzer.tone(2500, 50);
                    waitButtonRelease = true;
                    _btnPressing = true;
                    pressStartTime = millis();
                    press({-10, 20, 260, 35});
                }
            }
            else  // HIGH = released
            {
                if (waitButtonRelease)
                {
                    waitButtonRelease = false;
                    if (_btnPressing)
                    {
                        _btnPressing = false;
                        release();  // short press → onClick → onOpenEnd
                    }
                }
            }

            // --- Encoder navigation (skip during button press to avoid jump) ---
            if (waitButtonRelease)
            {
                // Still need to keep checkEncoder in sync
                h.input.checkEncoder(true);
                lastEncPos = h.input.enc.getPosition();
                return;
            }

            h.input.checkEncoder(true);  // 3000/3500 Hz direction tone
            if (h.input.enc.getPosition() != lastEncPos)
            {
                // Reset old selected row keyframe
                _data.option_list[_data.selected_option_index].keyframe.x = 0;

                if (h.input.enc.getPosition() < lastEncPos)
                    goLast();
                else
                    goNext();

                // Shift new selected row keyframe (slide right)
                _data.option_list[_data.selected_option_index].keyframe.x = 12;

                lastEncPos = h.input.enc.getPosition();
            }
        }

        void onRender() override
        {
            auto& c = hw->display.canvas;
            c->fillScreen(TFT_WHITE);

            // Camera offset for scrolling
            int camY = getCameraOffset().y;

            // --- Title bar (fixed, not affected by camera) ---
            c->fillRect(0, 0, 240, 25, (uint32_t)0x4A4A6A);
            c->setFont(&fonts::efontCN_16);
            c->setTextSize(1);
            c->setTextColor(TFT_WHITE);
            c->setTextDatum(top_center);
            c->drawString("Settings", 120, 4);

            // --- Rows (offset by camera for scrolling) ---
            c->setFont(&fonts::efontCN_16_b);
            c->setTextSize(1);

            for (int i = 0; i < itemCount; i++)
            {
                int rowX = _data.option_list[i].keyframe.x;
                int rowY = _data.option_list[i].keyframe.y - camY;
                int textY = rowY + _rowH / 2;

                // Skip rows that are completely off-screen
                if (rowY + _rowH < 25 || rowY > 135) continue;

                if (i == _data.selected_option_index && !isOpening())
                {
                    auto sf = getSelectorCurrentFrame();
                    c->fillSmoothRoundRect(sf.x, sf.y - camY, sf.w, sf.h, 6, items[i].themeColor);
                    c->setTextColor(items[i].textColor);
                }
                else
                {
                    c->setTextColor((uint32_t)0x888888);
                }

                c->setTextDatum(middle_left);
                c->drawString(items[i].name, rowX + 16, textY);
            }

            hw->display.push();
        }

        void onPress() override
        {
            getSelectorPostion().setDuration(200);
            getSelectorPostion().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutQuad);
            getSelectorShape().setDuration(200);
            getSelectorShape().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutQuad);
        }

        void onClick() override
        {
            if (goBack) return;

            int idx = _data.selected_option_index;
            // Check if this is the Back option (runner == nullptr)
            if (idx >= 0 && idx < itemCount && items[idx].runner == nullptr)
            {
                goBack = true;
                return;
            }

            getSelectorPostion().setDuration(300);
            getSelectorPostion().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutQuad);
            getSelectorShape().setDuration(300);
            getSelectorShape().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutQuad);
            open({-20, -20, 280, 175});
        }

        void onOpenEnd() override
        {
            int idx = _data.selected_option_index;
            if (idx >= 0 && idx < itemCount && items[idx].runner != nullptr)
            {
                items[idx].runner(*hw);
            }

            // Reset after sub-app returns
            getSelectorPostion().setDuration(600);
            getSelectorPostion().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutBack);
            getSelectorShape().setDuration(400);
            getSelectorShape().setTransitionPath(SmoothUIToolKit::EasingPath::easeOutQuad);
            close();

            hw->input.enc.setPosition(lastEncPos);
            hw->display.canvas->setFont(&fonts::efontCN_24);
            hw->display.canvas->setTextSize(1);

            // Reset all keyframe x to 0, then restore selected offset
            for (int i = 0; i < itemCount; i++)
                _data.option_list[i].keyframe.x = 0;
            _data.option_list[_data.selected_option_index].keyframe.x = 12;
        }

    private:
        static constexpr int _rowH = 20;
    };
};
