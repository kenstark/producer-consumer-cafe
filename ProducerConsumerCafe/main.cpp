//
//  main.cpp
//  ProducerConsumerCafe
//


#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <queue>
#include <vector>
#include <chrono>
#include <random>


std::mutex counter_mutex;
std::counting_semaphore<5> empty_slots(5);
std::counting_semaphore<5> full_slots(0);

std::queue<std::string> counter;
std::vector<std::string> coffee_types = {"Espresso", "Latte", "Cappuccino", "Mocha", "Americano"};

std::string random_coffee() {
    int idx = rand() % coffee_types.size();
    return coffee_types[idx];
}

void barista(int id) {
    while (true) {
        std::string coffee = random_coffee();

        empty_slots.acquire();
        std::lock_guard<std::mutex> lock(counter_mutex);

        counter.push(coffee);
        std::cout << "Barista " << id << " made: " << coffee << " | Counter has " << counter.size() << " coffees." << std::endl;

        full_slots.release();

        std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1500));
    }
}

void customer(int id) {
    while (true) {
        full_slots.acquire();
        std::lock_guard<std::mutex> lock(counter_mutex);

        std::string coffee = counter.front();
        counter.pop();
        std::cout << "Customer " << id << " took: " << coffee << " | Counter has " << counter.size() << " coffees." << std::endl;

        empty_slots.release();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 2000));
    }
}

int main() {
    srand(time(0));

    std::vector<std::thread> baristas;
    std::vector<std::thread> customers;

    for (int i = 0; i < 2; i++) {
        baristas.push_back(std::thread(barista, i+1));
    }
    for (int i = 0; i < 3; i++) {
        customers.push_back(std::thread(customer, i+1));
    }

    for (auto& b : baristas) {
        b.join();
    }
    for (auto& c : customers) {
        c.join();
    }

    return 0;
}

