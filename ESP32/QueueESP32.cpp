#include <iostream>
#include "../shared/cpp/QueueManager.h"

int main()
{
    // Create QueueManager instance for ESP32 with strategyPrefix "_ESP32"
    // Parameters: maxSize, numberOfLines, strategyPrefix, appName
    QueueManager queueManager(50, 2, "_ESP32", "iot-queue-management-ESP32");

    // Example usage for ESP32 developer:

    // When a customer arrives (detected by sensor):
    std::cout << "\n--- Customer Detection Example ---" << std::endl;
    if (queueManager.enqueue(LineSelectionStrategy::SHORTEST_WAIT_TIME))
    {
        std::cout << "✅ Customer added to queue successfully" << std::endl;
    }

    // When a customer is served (detected by sensor):
    std::cout << "\n--- Customer Served Example ---" << std::endl;
    if (queueManager.dequeue(1))
    { // line 1 served a customer
        std::cout << "✅ Customer served from line 1" << std::endl;
    }

    // Check queue status:
    std::cout << "\n--- Queue Status ---" << std::endl;
    std::cout << "Total people in queue: " << queueManager.size() << std::endl;
    for (int line = 1; line <= 3; line++)
    {
        std::cout << "Line " << line << " has " << queueManager.getLineCount(line) << " people" << std::endl;
    }

    return 0;
}