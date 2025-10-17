#pragma once

#include <string>
#include <memory>
#ifdef ESP32
#include <Arduino.h>
#endif
#include "SimpleHttpClient.h"

class FirebaseClient
{
private:
    std::string projectId;
    std::string databaseUrl;
    std::string databaseSecret;
    std::unique_ptr<SimpleHttpClient> httpClient;

public:
    FirebaseClient(const std::string &projectId, const std::string &databaseUrl, const std::string &databaseSecret = "");
    ~FirebaseClient();

    bool initialize();
    bool writeData(const std::string &path, const std::string &jsonData);
    bool updateData(const std::string &path, const std::string &jsonData);
    bool deleteData(const std::string &path);
    std::string readData(const std::string &path);

    void cleanup();

private:
    void debugPrint(const std::string &message);
};
