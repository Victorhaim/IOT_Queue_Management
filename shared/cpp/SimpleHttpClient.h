#pragma once

#include <string>
#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
// ESP32 supports std::string, so we can use it everywhere
using std::string;
#endif

// Cross-platform HTTP client:
//  - ESP32: WiFiClientSecure + HTTPClient
//  - Windows: WinHTTP API
//  - Non-Windows (macOS/Linux): libcurl
// Supports minimal Firebase REST operations (PUT, PATCH, DELETE, optional GET)
class SimpleHttpClient
{
private:
    std::string baseUrl;
    std::string authSecret;
#ifdef ESP32
    WiFiClientSecure *wifiClient;
    HTTPClient *httpClient;
#endif

public:
    SimpleHttpClient(const std::string &baseUrl, const std::string &authSecret = "");
    ~SimpleHttpClient();

    bool initialize();
    bool sendPutRequest(const std::string &path, const std::string &jsonData);
    bool sendPatchRequest(const std::string &path, const std::string &jsonData);
    bool sendDeleteRequest(const std::string &path);
    std::string sendGetRequest(const std::string &path);

private:
    bool sendRequest(const std::string &method, const std::string &path, const std::string &data);
    std::string constructUrl(const std::string &path);
    void debugPrint(const std::string &message);
};
