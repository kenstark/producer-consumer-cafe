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
#include <atomic>

std::mutex counter_mutex;
std::mutex log_mutex;
std::mutex revenue_mutex;
std::counting_semaphore<5> empty_slots(5);
std::counting_semaphore<5> full_slots(0);

// Track total revenue
int totalRevenue = 0;

// Atomic flag to control when to end the simulation
std::atomic<bool> running = true;

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
    while (running) {
        // Try to acquire with a timeout to check if simulation should end
        if (!empty_slots.try_acquire_for(std::chrono::milliseconds(500))) {
            if (!running) return;
            continue;
        }

        if (!running) {
            empty_slots.release();
            return;
        }

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
    // Each customer has a random patience level (2-5 seconds)
    int patience = 2000 + (rand() % 3000);
    
    while (running) {
        // Try to acquire with a timeout based on patience
        bool got_item = full_slots.try_acquire_for(std::chrono::milliseconds(patience));
        
        if (!running) return;
        
        if (!got_item) {
            // Customer got impatient and left
            log("Customer " + std::to_string(id) + " got impatient and left without buying anything!");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 2000));
            continue;
        }
        
        Item item;
        bool valid_item = false;

        {
            std::lock_guard<std::mutex> lock(counter_mutex);
            if (!counter.empty()) {
                item = counter.front();
                counter.pop();
                valid_item = true;
            }
        }

        if (valid_item) {
            log("Customer " + std::to_string(id) + " bought: " + item.name + " for $" + std::to_string(item.price));
            
            // Update total revenue
            {
                std::lock_guard<std::mutex> lock(revenue_mutex);
                totalRevenue += item.price;
            }
            
            empty_slots.release();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 2000));
    }
}

void print_counter_state() {
    while (running) {
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
        
        // Display current revenue
        {
            std::lock_guard<std::mutex> revenue_lock(revenue_mutex);
            std::cout << "Current Total Revenue: $" << totalRevenue << std::endl;
        }
        
        std::cout << "=====================\n" << std::endl;
    }
}

// Timer to end the simulation after 30 seconds
void timer() {
    std::this_thread::sleep_for(std::chrono::seconds(30));
    log("\n*** SIMULATION TIME EXPIRED (30 seconds) ***\n");
    running = false;
}

int main() {
    srand(time(0));

    std::vector<std::thread> baristas;
    std::vector<std::thread> customers;

    // Start the timer
    std::thread timer_thread(timer);
    timer_thread.detach();

    for (int i = 0; i < 2; i++) {
        baristas.emplace_back(barista, i + 1);
    }
    for (int i = 0; i < 6; i++) {
        customers.emplace_back(customer, i + 1);
    }

    std::thread observer(print_counter_state);

    // Join all threads (they'll terminate when running becomes false)
    for (auto& b : baristas) b.join();
    for (auto& c : customers) c.join();
    observer.join();

    // Print final statistics
    std::cout << "\n=== FINAL STATISTICS ===" << std::endl;
    std::cout << "Total Revenue: $" << totalRevenue << std::endl;
    std::cout << "=====================\n" << std::endl;

    return 0;
}
