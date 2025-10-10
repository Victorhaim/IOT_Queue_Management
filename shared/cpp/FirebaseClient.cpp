#include "FirebaseClient.h"
#include <iostream>
#include <sstream>

FirebaseClient::FirebaseClient(const std::string &projectId, const std::string &databaseUrl)
    : projectId(projectId), databaseUrl(databaseUrl)
{
    httpClient = std::make_unique<SimpleHttpClient>(databaseUrl);
}

FirebaseClient::~FirebaseClient()
{
    cleanup();
}

bool FirebaseClient::initialize()
{
    if (!httpClient->initialize())
    {
        std::cerr << "Failed to initialize HTTP client" << std::endl;
        return false;
    }

    std::cout << "Firebase client initialized for project: " << projectId << std::endl;
    return true;
}

bool FirebaseClient::writeData(const std::string &path, const std::string &jsonData)
{
    if (httpClient->sendPutRequest(path, jsonData))
    {
        std::cout << "Successfully wrote to Firebase: " << path << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Firebase write failed for path: " << path << std::endl;
        return false;
    }
}

bool FirebaseClient::updateData(const std::string &path, const std::string &jsonData)
{
    if (httpClient->sendPatchRequest(path, jsonData))
    {
        std::cout << "Successfully updated Firebase: " << path << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Firebase update failed for path: " << path << std::endl;
        return false;
    }
}

bool FirebaseClient::deleteData(const std::string &path)
{
    if (httpClient->sendDeleteRequest(path))
    {
        std::cout << "Successfully deleted from Firebase: " << path << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Firebase delete failed for path: " << path << std::endl;
        return false;
    }
}

std::string FirebaseClient::readData(const std::string &path)
{
    return httpClient->sendGetRequest(path);
}

void FirebaseClient::cleanup()
{
    // HTTP client cleanup is handled by its destructor
}