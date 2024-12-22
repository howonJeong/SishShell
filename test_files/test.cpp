#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Waiting for 10 seconds..." << std::endl;
    for (int i = 0; i < 10; i++) {
        std::cout << 10 - i << "... ";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "\nWaited for 10 seconds, work done." << std::endl;
    return 0;
}
