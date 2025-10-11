#pragma once

#include <string>

// Simple cross-platform HTTP client:
//  - Windows: WinHTTP API
//  - Non-Windows (macOS/Linux): libcurl
// Supports minimal Firebase REST operations (PUT, PATCH, DELETE, optional GET)
class SimpleHttpClient
{
private:
    std::string baseUrl;

public:
    SimpleHttpClient(const std::string &baseUrl);
    ~SimpleHttpClient();

    bool initialize();
    bool sendPutRequest(const std::string &path, const std::string &jsonData);
    bool sendPatchRequest(const std::string &path, const std::string &jsonData);
    bool sendDeleteRequest(const std::string &path);
    std::string sendGetRequest(const std::string &path);

private:
    bool sendRequest(const std::string &method, const std::string &path, const std::string &data);
};
