#ifndef SIMPLE_HTTP_CLIENT_H
#define SIMPLE_HTTP_CLIENT_H

#include <string>

// Simple HTTP client using Windows WinHTTP API
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

#endif // SIMPLE_HTTP_CLIENT_H