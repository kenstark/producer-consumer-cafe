#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <queue>
#include <vector>
#include <chrono>
#include <random>
#include <map>
#include <cstdlib>

std::mutex counter_mutex;
std::mutex log_mutex;
std::counting_semaphore<10> empty_slots(10);
std::counting_semaphore<10> full_slots(0);

enum ItemType { COFFEE, CAKE, SANDWICH };

struct Item {
    ItemType type;
    std::string name;
    int price;
};

std::queue<Item> counter;

std::vector<std::string> coffee_sizes = {"Small", "Medium", "Large"};
std::map<std::string, int> coffee_prices = {{"Small", 3}, {"Medium", 5}, {"Large", 7}};

std::vector<std::string> cakes = {"Cheesecake", "Carrot Cake", "Red Velvet", "Sponge", "Plain", "Corn Bread"};
std::vector<int> cake_prices = {3, 4, 5};
int full_cake_price = 15;

std::vector<std::string> sandwiches = {"BLT", "Caesar Salad", "Chicken", "Ham & Cheese", "Grilled Cheese", "Egg Bacon & Cheese", "Bagel"};
int sandwich_price = 8;
int topping_price = 1;

std::string random_choice(const std::vector<std::string>& options) {
    return options[rand() % options.size()];
}

void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << msg << std::endl;
}

void barista(int id) {
    while (true) {
        empty_slots.acquire();

        Item item;
        int kind = rand() % 3;
        if (kind == 0) { // coffee
            std::string size = random_choice(coffee_sizes);
            item = {COFFEE, size + " Coffee", coffee_prices[size]};
        } else if (kind == 1) { // cake
            std::string cake = random_choice(cakes);
            bool full = (rand() % 5 == 0);
            if (full) {
                item = {CAKE, "Whole " + cake, full_cake_price};
            } else {
                item = {CAKE, cake + " Slice", cake_prices[rand() % cake_prices.size()]};
            }
        } else { // sandwich
            std::string s = random_choice(sandwiches);
            int toppings = rand() % 4; // up to 3 extra toppings
            item = {SANDWICH, s + (toppings ? " +" + std::to_string(toppings) + " toppings" : ""), sandwich_price + toppings * topping_price};
        }

        {
            std::lock_guard<std::mutex> lock(counter_mutex);
            counter.push(item);
            log("Barista " + std::to_string(id) + " prepared: " + item.name + " ($" + std::to_string(item.price) + ")");
        }

        full_slots.release();
        std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1500));
    }
}

void customer(int id) {
    while (true) {
        full_slots.acquire();
        Item item;

        {
            std::lock_guard<std::mutex> lock(counter_mutex);
            if (!counter.empty()) {
                item = counter.front();
                counter.pop();
            }
        }

        log("Customer " + std::to_string(id) + " bought: " + item.name + " for $" + std::to_string(item.price));
        empty_slots.release();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 2000));
    }
}

void print_counter_state() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::lock_guard<std::mutex> lock(counter_mutex);
        std::queue<Item> temp = counter;
        std::cout << "\n=== Counter State ===" << std::endl;
        int i = 1;
        while (!temp.empty()) {
            Item item = temp.front();
            std::cout << i++ << ". " << item.name << " ($" << item.price << ")" << std::endl;
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
        baristas.emplace_back(barista, i + 1);
    }
    for (int i = 0; i < 3; i++) {
        customers.emplace_back(customer, i + 1);
    }

    std::thread observer(print_counter_state);
    observer.detach();

    for (auto& b : baristas) b.join();
    for (auto& c : customers) c.join();

    return 0;
}
