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
std::mutex log_mutex;
std::counting_semaphore<5> empty_slots(5);
std::counting_semaphore<5> full_slots(0);

std::queue<std::string> counter;
std::vector<std::string> coffee_types = {"Espresso", "Latte", "Cappuccino", "Mocha", "Americano"};

void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << msg << std::endl;
}
std::string random_coffee() {
    int idx = rand() % coffee_types.size();
    return coffee_types[idx];
}

void barista(int id) {
    while (true) {
        std::string coffee = random_coffee();

        empty_slots.acquire();

        {
            
        std::lock_guard<std::mutex> lock(counter_mutex);

        counter.push(coffee);
        std::cout << "Barista " << id << " made: " << coffee << " | Counter has " << counter.size() << " coffees." << std::endl;
        }
        full_slots.release();

        std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1500));
    }
}

void customer(int id) {
    std::string preferred = coffee_types[rand() % coffee_types.size()];
    while (true) {
        full_slots.acquire();
        {
        std::lock_guard<std::mutex> lock(counter_mutex);

        bool found = false;
        std::queue<std::string> temp;

        while (!counter.empty()) {
            std::string item = counter.front();
            counter.pop();
            if (!found && item == preferred) {
                found = true;
                std::cout << "Customer " << id << " got preferred: " << item << std::{;
            } else {
                temp.push(item);
            }
        }

        while (!temp.empty()) {
            counter.push(temp.front());
            temp.pop();
        }

        if (!found) {
            std::cout << "Customer " << id << " couldn't find " << preferred << " and left." << std::endl;
        }

        empty_slots.release();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
}

void print_counter_state() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::lock_guard<std::mutex> lock(counter_mutex);
        std::cout << "\n=== Counter State ===" << std::endl;
        std::queue<std::string> temp = counter;
        int i = 1;
        while (!temp.empty()) {
            std::cout << i++ << ". " << temp.front() << std::endl;
            temp.pop();
        }
        std::cout << "=====================\n" << std::endl;
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
    std::thread observer(print_counter_state);
    
    observer.detach(); 


    for (auto& b : baristas) {
        b.join();
    }
    for (auto& c : customers) {
        c.join();
    }

    return 0;
}

