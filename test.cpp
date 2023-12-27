#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int func() {
    int a = 0;
    std::cin >> a;
    return a;
}

int main() {
    std::future<int> f = std::async(std::launch::async, func);
    std::future_status status;
    while ((status = f.wait_for(std::chrono::seconds(1))) != std::future_status::ready) {
        if (status == std::future_status::timeout) {
            std::cout << "timeout" << std::endl;
        }
    }
    std::cout << f.get() << std::endl;
}