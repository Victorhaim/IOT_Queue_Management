#pragma once

#include <string>
#include <memory>
#include "../http/SimpleHttpClient.h"

class FirebaseClient
{
private:
    std::string projectId;
    std::string databaseUrl;
    std::unique_ptr<SimpleHttpClient> httpClient;

public:
    FirebaseClient(const std::string &projectId, const std::string &databaseUrl);
    ~FirebaseClient();

    bool initialize();
    bool writeData(const std::string &path, const std::string &jsonData);
    bool updateData(const std::string &path, const std::string &jsonData);
    bool deleteData(const std::string &path);
    std::string readData(const std::string &path);

    void cleanup();
};
