#include "SimpleHttpClient.h"
#include <iostream>
#include <windows.h>
#include <winhttp.h>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

SimpleHttpClient::SimpleHttpClient(const std::string &baseUrl) : baseUrl(baseUrl)
{
}

SimpleHttpClient::~SimpleHttpClient()
{
}

bool SimpleHttpClient::initialize()
{
    return true; // No initialization needed for WinHTTP
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
    // For now, just implement PUT/PATCH for writing to Firebase
    return "";
}

bool SimpleHttpClient::sendRequest(const std::string &method, const std::string &path, const std::string &data)
{
    try
    {
        // Parse the base URL
        std::string hostname;
        std::string urlPath;
        bool isHttps = false;

        if (baseUrl.find("https://") == 0)
        {
            isHttps = true;
            hostname = baseUrl.substr(8);
        }
        else if (baseUrl.find("http://") == 0)
        {
            hostname = baseUrl.substr(7);
        }
        else
        {
            hostname = baseUrl;
        }

        // Remove any trailing slash and path
        size_t slashPos = hostname.find('/');
        if (slashPos != std::string::npos)
        {
            urlPath = hostname.substr(slashPos);
            hostname = hostname.substr(0, slashPos);
        }

        // Combine URL path with request path
        std::string fullPath = urlPath + "/" + path + ".json";

        // Convert strings to wide strings for Windows API
        std::wstring wHostname(hostname.begin(), hostname.end());
        std::wstring wPath(fullPath.begin(), fullPath.end());
        std::wstring wMethod(method.begin(), method.end());

        // Initialize WinHTTP
        HINTERNET hSession = WinHttpOpen(L"Queue Simulator/1.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession)
        {
            std::cerr << "WinHttpOpen failed" << std::endl;
            return false;
        }

        // Connect to server
        HINTERNET hConnect = WinHttpConnect(hSession, wHostname.c_str(),
                                            isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
                                            0);

        if (!hConnect)
        {
            std::cerr << "WinHttpConnect failed" << std::endl;
            WinHttpCloseHandle(hSession);
            return false;
        }

        // Create request
        DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), wPath.c_str(),
                                                NULL, WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

        if (!hRequest)
        {
            std::cerr << "WinHttpOpenRequest failed" << std::endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // Set headers
        std::wstring headers = L"Content-Type: application/json\\r\\n";
        WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

        // Send request
        BOOL result = WinHttpSendRequest(hRequest,
                                         WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                         (LPVOID)data.c_str(), data.length(),
                                         data.length(), 0);

        if (!result)
        {
            std::cerr << "WinHttpSendRequest failed. Error: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // Receive response
        result = WinHttpReceiveResponse(hRequest, NULL);
        if (!result)
        {
            std::cerr << "WinHttpReceiveResponse failed" << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // Check status code
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

        // Cleanup
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (statusCode >= 200 && statusCode < 300)
        {
            std::cout << "HTTP " << method << " successful (status: " << statusCode << ")" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "HTTP request failed with status: " << statusCode << std::endl;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "HTTP request exception: " << e.what() << std::endl;
        return false;
    }
}