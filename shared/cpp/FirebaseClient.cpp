#include "FirebaseClient.h"
#include <iostream>
#ifdef ESP32
#include <Arduino.h>
#endif

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
        debugPrint("Failed to initialize HTTP client");
        return false;
    }

    debugPrint("Firebase client initialized for project: " + projectId);
    return true;
}

bool FirebaseClient::writeData(const std::string &path, const std::string &jsonData)
{
    if (httpClient->sendPutRequest(path, jsonData))
    {
        debugPrint("Successfully wrote to Firebase: " + path);
        return true;
    }
    else
    {
        debugPrint("Firebase write failed for path: " + path);
        return false;
    }
}

bool FirebaseClient::updateData(const std::string &path, const std::string &jsonData)
{
    if (httpClient->sendPatchRequest(path, jsonData))
    {
        debugPrint("Successfully updated Firebase: " + path);
        return true;
    }
    else
    {
        debugPrint("Firebase update failed for path: " + path);
        return false;
    }
}

bool FirebaseClient::deleteData(const std::string &path)
{
    if (httpClient->sendDeleteRequest(path))
    {
        debugPrint("Successfully deleted from Firebase: " + path);
        return true;
    }
    else
    {
        debugPrint("Firebase delete failed for path: " + path);
        return false;
    }
}

std::string FirebaseClient::readData(const std::string &path)
{
    return httpClient->sendGetRequest(path);
}

void FirebaseClient::debugPrint(const std::string &message)
{
#ifdef ESP32
    Serial.println(("[Firebase] " + message).c_str());
#else
    std::cout << "[Firebase] " << message << std::endl;
#endif
}

void FirebaseClient::cleanup()
{
    // HTTP client cleanup is handled by its destructor
}