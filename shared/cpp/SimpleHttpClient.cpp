// Cross-platform implementation: ESP32 WiFi, WinHTTP on Windows, libcurl elsewhere
#include "SimpleHttpClient.h"
#include <iostream>
#include <sstream>

#ifdef ESP32
//#include <ArduinoJson.h>
#endif

#if defined(_WIN32) && !defined(ESP32)
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
#elif !defined(ESP32)
    #include <curl/curl.h>
#endif

SimpleHttpClient::SimpleHttpClient(const std::string &baseUrl, const std::string &authSecret) : baseUrl(baseUrl), authSecret(authSecret)
{
#ifdef ESP32
    wifiClient = new WiFiClientSecure();
    httpClient = new HTTPClient();
    wifiClient->setInsecure(); // For testing - in production use proper certificates
#endif
}

SimpleHttpClient::~SimpleHttpClient()
{
#ifdef ESP32
    if (httpClient) {
        httpClient->end();
        delete httpClient;
    }
    if (wifiClient) {
        delete wifiClient;
    }
#endif
}

bool SimpleHttpClient::initialize()
{
    debugPrint("HTTP client initialized");
    return true;
}

bool SimpleHttpClient::sendPutRequest(const std::string &path, const std::string &jsonData)
{
    return sendRequest("PUT", path, jsonData);
}

bool SimpleHttpClient::sendPatchRequest(const std::string &path, const std::string &jsonData)
{
    return sendRequest("PATCH", path, jsonData);
}

bool SimpleHttpClient::sendDeleteRequest(const std::string &path)
{
    return sendRequest("DELETE", path, "");
}

std::string SimpleHttpClient::sendGetRequest(const std::string &path)
{
#ifdef ESP32
    // ESP32 implementation for GET request
    std::string url = constructUrl(path);
    
    httpClient->begin(*wifiClient, url.c_str());
    httpClient->addHeader("Content-Type", "application/json");
    
    int httpCode = httpClient->GET();
    std::string response = "";
    
    if (httpCode > 0) {
        response = httpClient->getString().c_str();
        debugPrint("GET successful: " + std::to_string(httpCode));
    } else {
        debugPrint("GET failed: " + std::to_string(httpCode));
    }
    
    httpClient->end();
    return response;
#else
    // For now, just implement PUT/PATCH for writing to Firebase on desktop
    return "";
#endif
}

std::string SimpleHttpClient::constructUrl(const std::string &path)
{
    std::string url = baseUrl;
    if (!url.empty() && url.back() != '/') url += "/";
    url += path + ".json";
    
    // Add authentication if secret is provided
    if (!authSecret.empty()) {
        url += "?auth=" + authSecret;
    }
    
    return url;
}

void SimpleHttpClient::debugPrint(const std::string &message)
{
#ifdef ESP32
    Serial.println(("[HTTP] " + message).c_str());
#else
    std::cout << "[HTTP] " << message << std::endl;
#endif
}

bool SimpleHttpClient::sendRequest(const std::string &method, const std::string &path, const std::string &data)
{
#ifdef ESP32
    // ESP32 implementation
    std::string url = constructUrl(path);
    
    httpClient->begin(*wifiClient, url.c_str());
    httpClient->addHeader("Content-Type", "application/json");
    
    int httpCode = -1;
    
    if (method == "PUT") {
        httpCode = httpClient->PUT(data.c_str());
    } else if (method == "PATCH") {
        httpCode = httpClient->PATCH(data.c_str());
    } else if (method == "DELETE") {
        httpCode = httpClient->sendRequest("DELETE");
    } else if (method == "GET") {
        httpCode = httpClient->GET();
    }
    
    bool success = (httpCode >= 200 && httpCode < 300);
    
    if (success) {
        debugPrint("HTTP " + method + " successful (status: " + std::to_string(httpCode) + ")");
    } else {
        debugPrint("HTTP " + method + " failed (status: " + std::to_string(httpCode) + ")");
    }
    
    httpClient->end();
    return success;
}
#else
{
#if defined(_WIN32)
    try {
        // Windows implementation (existing WinHTTP logic)
        std::string hostname; std::string urlPath; bool isHttps = false;
        if (baseUrl.rfind("https://",0)==0) { isHttps = true; hostname = baseUrl.substr(8); }
        else if (baseUrl.rfind("http://",0)==0) { hostname = baseUrl.substr(7); }
        else { hostname = baseUrl; }
        size_t slashPos = hostname.find('/');
        if (slashPos != std::string::npos) { urlPath = hostname.substr(slashPos); hostname = hostname.substr(0, slashPos); }
        std::string fullPath = urlPath + "/" + path + ".json";
        if (!authSecret.empty()) {
            fullPath += "?auth=" + authSecret;
        }
        std::wstring wHostname(hostname.begin(), hostname.end());
        std::wstring wPath(fullPath.begin(), fullPath.end());
        std::wstring wMethod(method.begin(), method.end());
        HINTERNET hSession = WinHttpOpen(L"Queue Simulator/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) { std::cerr << "WinHttpOpen failed" << std::endl; return false; }
        HINTERNET hConnect = WinHttpConnect(hSession, wHostname.c_str(), isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!hConnect) { std::cerr << "WinHttpConnect failed" << std::endl; WinHttpCloseHandle(hSession); return false; }
        DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), wPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) { std::cerr << "WinHttpOpenRequest failed" << std::endl; WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
        std::wstring headers = L"Content-Type: application/json\r\n";
        WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
        BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)data.c_str(), data.length(), data.length(), 0);
        if (!result) { std::cerr << "WinHttpSendRequest failed. Error: " << GetLastError() << std::endl; WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
        result = WinHttpReceiveResponse(hRequest, NULL);
        if (!result) { std::cerr << "WinHttpReceiveResponse failed" << std::endl; WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
        DWORD statusCode = 0; DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        if (statusCode >= 200 && statusCode < 300) { std::cout << "HTTP " << method << " successful (status: " << statusCode << ")" << std::endl; return true; }
        std::cerr << "HTTP request failed with status: " << statusCode << std::endl; return false;
    } catch (const std::exception &e) { std::cerr << "HTTP request exception: " << e.what() << std::endl; return false; }
#else
    // libcurl implementation
    CURL *curl = curl_easy_init();
    if (!curl) { std::cerr << "curl_easy_init failed" << std::endl; return false; }
    std::string url = baseUrl;
    if (!url.empty() && url.back() == '/') url.pop_back();
    url += "/" + path + ".json";
    if (!authSecret.empty()) {
        url += "?auth=" + authSecret;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "QueueSimulator/1.0");
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    } else if (method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method == "GET") {
        // default
    } else {
        curl_slist_free_all(headers); curl_easy_cleanup(curl); std::cerr << "Unsupported method: " << method << std::endl; return false; }
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    CURLcode res = curl_easy_perform(curl);
    long code = 0; if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if (res != CURLE_OK) {
        std::cerr << "curl perform error: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers); curl_easy_cleanup(curl); return false; }
    curl_slist_free_all(headers); curl_easy_cleanup(curl);
    if (code >= 200 && code < 300) { std::cout << "HTTP " << method << " successful (status: " << code << ")" << std::endl; return true; }
    std::cerr << "HTTP request failed with status: " << code << std::endl; return false;
#endif
}
}
#endif