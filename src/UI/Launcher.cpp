/**
 * @file Launcher.cpp
 * @brief Menu UI implementation using SmoothUIToolKit
 */
#include "Launcher.h"
#include "../Apps/SettingsApp.h"
#include "../Apps/RtcTimeApp.h"
#include "../Apps/WifiScanApp.h"
#include "../Apps/ArkanoidApp.h"
#include <smooth_ui_toolkit.h>

using namespace SmoothUIToolKit;
using namespace SmoothUIToolKit::SelectMenu;

// === Impl (PIMPL to keep SmoothUIToolKit details out of header) ===
struct Launcher::Impl
{
    Hardware& hw;

    // --- Menu state ---
    class MenuCore : public SmoothOptions
    {
    public:
        Impl* owner = nullptr;
        int matchingIndex = 0;
        bool waitButtonRelease = false;
        bool _pressing = false;  // note: avoid shadowing SmoothOptions::isPressing()

        void onReadInput() override;
        void onRender() override;
        void onPress() override;
        void onClick() override;
        void onOpenEnd() override;
    };

    MenuCore*      menu     = nullptr;
    Transition2D*  batPanel = nullptr;
    uint32_t       batTime  = 0;
    char           batStr[10] = {0};
    int            lastEncPos = 0;
    bool           justBoot   = true;

    // --- App render props (externally defined) ---
    const AppEntry* apps;
    int             appCount;

    Impl(Hardware& h, const AppEntry* a, int n) : hw(h), apps(a), appCount(n) {}
};

// === MenuCore callbacks ===

void Launcher::Impl::MenuCore::onReadInput()
{
    if (isOpening()) return;

    auto& h = owner->hw;

    h.input.checkEncoder(true);
    if (h.input.enc.getPosition() != owner->lastEncPos)
    {
        if (h.input.enc.getPosition() < owner->lastEncPos) goLast();
        else goNext();
        owner->lastEncPos = h.input.enc.getPosition();
    }

    if (owner->justBoot)
    {
        if (h.input.btn_pwr.read()) owner->justBoot = false;
    }
    else if (!h.input.btn_pwr.read())
    {
        if (!waitButtonRelease)
        {
            h.buzzer.tone(2500, 50);
            waitButtonRelease = true;
            _pressing = true;
            press({0, 12, 240, 52});
        }
    }
    else
    {
        waitButtonRelease = false;
        if (_pressing) { _pressing = false; release(); }
    }
}

void Launcher::Impl::MenuCore::onRender()
{
    auto& c = owner->hw.display.canvas;
    c->fillScreen(TFT_WHITE);

    // Battery panel
    c->pushImage(owner->batPanel->getValue().x, owner->batPanel->getValue().y, 83, 54, image_data_bat_panel);
    c->setTextDatum(top_left);
    c->setTextColor(0x7F5845);
    c->setFont(&fonts::efontCN_16);
    c->drawString("Bat:", owner->batPanel->getValue().x + 6, owner->batPanel->getValue().y + 13);
    c->setFont(&fonts::efontCN_24);
    c->drawString(owner->batStr, owner->batPanel->getValue().x + 4, owner->batPanel->getValue().y + 29);

    // Options
    int yOff = 6;
    c->setTextDatum(top_right);
    for (int i = getKeyframeList().size() - 1; i >= 0; i--)
    {
        getMatchingOptionIndex(i, matchingIndex);
        int idx = matchingIndex;

        c->fillSmoothRoundRect(getOptionCurrentFrame(idx).x, getOptionCurrentFrame(idx).y,
                               getOptionCurrentFrame(idx).w, getOptionCurrentFrame(idx).h,
                               20, owner->apps[idx].themeColor);

        if (!isOpening())
        {
            if (i <= 1)
            {
                yOff = getOptionCurrentFrame(idx).y + 16 -
                       std::abs(getOptionCurrentFrame(idx).y - getKeyframe(0).y) * 10 / 75;
                if (isPressing()) yOff = (i == 0) ? (getKeyframe(0).y + 16) : (getKeyframe(1).y + 6);
            }
            else yOff = getOptionCurrentFrame(idx).y + 6;

            c->pushImage(getOptionCurrentFrame(idx).x + 13, yOff, 32, 32, owner->apps[idx].icon);
        }

        if (i == 0 && !isOpening())
        {
            c->setTextColor(owner->apps[idx].tagColor);
            c->drawString(owner->apps[idx].name, 218, 26);
        }
    }

    owner->hw.display.push();
}

void Launcher::Impl::MenuCore::onPress()
{
    setDuration(200);
    setTransitionPath(EasingPath::easeOutQuad);
}

void Launcher::Impl::MenuCore::onClick()
{
    setDuration(300);
    setTransitionPath(EasingPath::easeOutQuad);
    open({-20, -20, 280, 175});
}

void Launcher::Impl::MenuCore::onOpenEnd()
{
    // Execute app
    int idx = getSelectedOptionIndex();
    if (idx >= 0 && idx < owner->appCount)
    {
        owner->apps[idx].runner(owner->hw);
    }

    // Reset
    setPositionDuration(600);
    setPositionTransitionPath(EasingPath::easeOutBack);
    setShapeDuration(400);
    close();
    owner->hw.input.enc.setPosition(owner->lastEncPos);
    owner->hw.display.canvas->setFont(&fonts::efontCN_24);
    owner->hw.display.canvas->setTextSize(1);
}

// === Launcher public API ===

Launcher::Launcher(Hardware& hw)
    : _hw(hw)
    , _apps{
        {"SETTINGS",  0x4A4A6A, 0xFFFFFF, image_data_icon_menu,  [](Hardware& h) { SettingsApp().run(h); }},
        {"RTC TIME",  0xC9C9EE, 0x49496E, image_data_icon_rtc,   [](Hardware& h) { RtcTimeApp().run(h); }},
        {"WIFI SCAN", 0xF6A4A4, 0x762424, image_data_icon_wifi,  [](Hardware& h) { WifiScanApp().run(h); }},
        {"ARKANOID",  0xF5C396, 0x754316, image_data_icon_game,  [](Hardware& h) { ArkanoidApp().run(h); }},
      }
    , _impl(new Impl(hw, _apps, APP_COUNT))
{
}

void Launcher::begin()
{
    auto& i = *_impl;

    _hw.input.enc.setPosition(i.lastEncPos);
    _hw.display.canvas->setFont(&fonts::efontCN_24);
    _hw.display.canvas->setTextSize(1);

    // Create menu
    i.menu = new Impl::MenuCore;
    i.menu->owner = &i;

    // Selected option
    i.menu->addOption();
    i.menu->setLastKeyframe({6, 6, 228, 64});

    // Waiting line
    for (int n = 0; n < APP_COUNT - 2; n++)
    {
        i.menu->addOption();
        i.menu->setLastKeyframe({88 + 65 * n, 81, 58, 44});
    }

    // Closing dummy for smooth loop
    i.menu->addOption();
    i.menu->setLastKeyframe({6 + 228 + 24, 6, 58, 44});

    i.menu->setConfig().renderInterval    = 20;
    i.menu->setConfig().readInputInterval = 50;
    i.menu->setPositionDuration(600);
    i.menu->setPositionTransitionPath(EasingPath::easeOutBack);
    i.menu->setShapeDuration(400);

    // Battery panel transition
    i.batPanel = new Transition2D(-83, 135);
    i.batPanel->moveTo(0, 81);
    i.batPanel->setDelay(300);
    i.batPanel->setDuration(800);

    float v = (float)analogReadMilliVolts(10) * 2 / 1000;
    snprintf(i.batStr, 10, "%.1fV", v);
}

void Launcher::update()
{
    auto& i = *_impl;
    i.menu->update(millis());
    i.batPanel->update(millis());

    // Battery read every 3s
    if (millis() - i.batTime > 3000)
    {
        float v = (float)analogReadMilliVolts(10) * 2 / 1000;
        snprintf(i.batStr, 10, "%.1fV", v);
        i.batTime = millis();
    }
}
