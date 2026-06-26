/**
 * @file WifiScanApp.h
 * @brief WiFi network scanner
 */
#pragma once
#include "AppBase.h"
#include <WiFi.h>
#include <esp_wifi.h>

class WifiScanApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& canvas = hw.display.canvas;

        canvas->fillScreen((uint32_t)0xF6A4A4);
        canvas->setFont(&fonts::Font0);
        canvas->setTextSize(2);
        canvas->setTextColor((uint32_t)0xF6A4A4);
        canvas->fillRect(0, 0, 240, 25, (uint32_t)0x762424);
        canvas->printf("      WiFi Scan\n");

        canvas->setFont(&fonts::efontCN_16_b);
        canvas->setTextScroll(false);
        canvas->setTextSize(1);
        canvas->setCursor(0, 28);
        canvas->setTextColor((uint32_t)0x762424);
        canvas->printf(" Scanning...\n");
        hw.display.push();
        canvas->setCursor(0, 28);

        // Scan
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);

        int n = WiFi.scanNetworks();
        int count = (n > 6) ? 6 : n;

        if (n == 0)
        {
            canvas->printf(" No networks found\n");
        }
        else
        {
            for (int i = 0; i < count; ++i)
            {
                String ssid = WiFi.SSID(i);
                int rssi = WiFi.RSSI(i);

                switch (WiFi.encryptionType(i))
                {
                case WIFI_AUTH_OPEN:            ssid += " (open)"; break;
                case WIFI_AUTH_WEP:             ssid += " (WEP)"; break;
                case WIFI_AUTH_WPA_PSK:         ssid += " (WPA)"; break;
                case WIFI_AUTH_WPA2_PSK:        ssid += " (WPA2)"; break;
                case WIFI_AUTH_WPA_WPA2_PSK:    ssid += " (WPA+WPA2)"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: ssid += " (WPA2-EAP)"; break;
                case WIFI_AUTH_WPA3_PSK:        ssid += " (WPA3)"; break;
                case WIFI_AUTH_WPA2_WPA3_PSK:   ssid += " (WPA2+WPA3)"; break;
                case WIFI_AUTH_WAPI_PSK:        ssid += " (WAPI)"; break;
                default: break;
                }

                canvas->setTextColor((rssi > -75) ? TFT_DARKGREEN : TFT_BROWN, (uint32_t)0xF6A4A4);
                canvas->printf("%d", rssi);
                canvas->printf(" %s\n", ssid.c_str());
                hw.display.push();
            }
        }

        WiFi.scanDelete();
        WiFi.disconnect(true);
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_wifi_clear_ap_list();

        // Wait for exit
        while (1)
        {
            hw.input.checkEncoder();
            if (hw.input.checkNext()) break;
        }
    }
};
