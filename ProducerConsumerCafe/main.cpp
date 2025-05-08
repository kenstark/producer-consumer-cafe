#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <map>
#include <thread>
#include <mutex>
#include <semaphore>  // C++20 semaphore

using namespace std;

mutex mtx;  // Mutex to protect shared resource (console output)
std::counting_semaphore<2> availableBaristas(2); // Semaphore to limit concurrent customers (2 baristas)

// Structure to store item details (name and price)
struct OrderItem {
    string name;
    int price;
};

// Function to clear invalid input
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Function to choose a valid option from a list of options
int chooseOption(const vector<string>& options, const string& prompt) {
    int choice;
    while (true) {
        cout << prompt << endl;
        for (size_t i = 0; i < options.size(); ++i) {
            cout << i + 1 << ". " << options[i] << endl;
        }
        cout << "Your choice: ";
        cin >> choice;
        if (cin.fail() || choice < 1 || choice > options.size()) {
            clearInput();
            cout << "Invalid input. Try again.\n";
        } else {
            break;
        }
    }
    return choice;
}

// Function to order coffee
int getCoffee(vector<OrderItem>& order) {
    vector<string> types = {"Espresso", "Latte", "Cappuccino", "Americano", "Flat White", "Cold Brew"};
    vector<string> sizes = {"Small ($3)", "Medium ($5)", "Large ($7)"};
    vector<int> sizePrices = {3, 5, 7};

    int type = chooseOption(types, "\n☕ Choose your coffee type:") - 1;
    int size = chooseOption(sizes, "\n☕ Choose your coffee size:") - 1;

    order.push_back({sizes[size] + " " + types[type] + " Coffee", sizePrices[size]});
    return sizePrices[size];
}

// Function to order cake
int getCake(vector<OrderItem>& order) {
    vector<string> cakes = {
        "Chocolate Cake ($2)",
        "Cheesecake ($4)",
        "Red Velvet ($4)",
        "Carrot Cake ($3)",
        "Vanilla Cake ($2)",
        "Strawberry Cake ($3)",
        "Lemon Cake ($1)"
    };
    vector<int> cakePrices = {2, 4, 4, 3, 2, 3, 1};

    int cakeChoice = chooseOption(cakes, "\n🍰 Choose your cake slice:") - 1;

    string wholeCakeReply;
    cout << "\n🧁 Barista: Would you like to make it an entire cake for $20? (y/n): ";
    cin >> wholeCakeReply;

    string cakeName = cakes[cakeChoice];
    cakeName = cakeName.substr(0, cakeName.find(" ($")); // Strip price from name

    if (wholeCakeReply == "y" || wholeCakeReply == "Y") {
        order.push_back({"Whole " + cakeName, 20});
        return 20;
    } else {
        order.push_back({"Slice of " + cakeName, cakePrices[cakeChoice]});
        return cakePrices[cakeChoice];
    }
}

// Function to order sandwich
int getSandwich(vector<OrderItem>& order) {
    vector<string> sandwiches = {"BLT", "Caesar Salad", "Chicken", "Ham & Cheese", "Grilled Cheese", "Egg Bacon & Cheese", "Bagel"};
    vector<string> sides = {"Bacon ($2)", "Cheese ($1)", "Chips ($1)", "Fries ($2)", "Lettuce ($1)", "Salad ($2)", "Tomato ($1)"};
    vector<int> sidePrices = {2, 1, 1, 2, 1, 2, 1};

    int sandwich = chooseOption(sandwiches, "\n🥪 Choose your sandwich:") - 1;
    order.push_back({sandwiches[sandwich] + " Sandwich", 5});

    cout << "\n🍟 Add sides (up to 5, enter numbers separated by spaces, 0 to finish):\n";
    for (size_t i = 0; i < sides.size(); ++i) {
        cout << i + 1 << ". " << sides[i] << endl;
    }

    vector<bool> chosen(7, false);
    int sideChoice;
    int sideTotal = 0;
    int sideCount = 0;

    clearInput();
    while (sideCount < 5) {
        cout << "Enter side number (0 to finish): ";
        cin >> sideChoice;
        if (sideChoice == 0) break;
        if (sideChoice < 1 || sideChoice > 7 || chosen[sideChoice - 1]) {
            cout << "Invalid or duplicate choice. Try again.\n";
        } else {
            chosen[sideChoice - 1] = true;
            order.push_back({sides[sideChoice - 1], sidePrices[sideChoice - 1]});
            sideTotal += sidePrices[sideChoice - 1];
            sideCount++;
        }
    }
    return 5 + sideTotal;
}

// Function to process a customer's order
void processCustomer(int customerId) {
    availableBaristas.acquire(); // Limit the number of concurrently running threads (customers)

    lock_guard<mutex> lock(mtx);  // Locking to ensure thread-safe output

    vector<OrderItem> order;
    int total = 0;
    bool comboChosen = false;

    cout << "👋 Barista: Hello there, glad you came to Cafe Tech!\n\n";

    vector<string> greetings = {"Hello!", "Hey there!", "How's your day going?", "I'm in a rush!"};
    int greetChoice = chooseOption(greetings, "🧍 You, the customer, choose a greeting:");
    cout << "\n🧍 You: " << greetings[greetChoice - 1] << endl;

    // Main menu with Stop option
    vector<string> mainOptions = {"Coffee", "Cake", "Sandwich", "Combo (any two or all three)", "Order all 3 items", "Stop"};
    int mainChoice = chooseOption(mainOptions, "\n🧑‍🍳 Barista: What would you like to order?");

    // Stop option handling
    if (mainChoice == 6) {
        cout << "\n🛑 Barista: Thanks for visiting! Have a great day!\n";
        availableBaristas.release();  // Release semaphore and exit
        return;
    }

    if (mainChoice == 4) {
        comboChosen = true;
        cout << "\n🍽️ Great choice! Let's get a combo!\n";
        for (int i = 0; i < 2; ++i) {
            cout << "\n🧑‍🍳 Barista: Please choose item " << (i + 1) << ":\n";
            int comboPick = chooseOption({"Coffee", "Cake", "Sandwich"}, "Choose your item:");
            if (comboPick == 1) total += getCoffee(order);
            else if (comboPick == 2) total += getCake(order);
            else total += getSandwich(order);
        }
        cout << "🎉 Barista: Great choice! You've ordered a combo. You've saved $2!\n";
        total -= 2;
    } else if (mainChoice == 5) {
        cout << "\n🍽️ Ordering all 3 items!\n";
        total += getCoffee(order);
        total += getCake(order);
        total += getSandwich(order);
    } else {
        if (mainChoice == 1) total += getCoffee(order);
        else if (mainChoice == 2) total += getCake(order);
        else total += getSandwich(order);

        string comboReply;
        cout << "\n🧑‍🍳 Barista: Would you like to make it a combo for $2 off? (y/n): ";
        cin >> comboReply;
        clearInput();
        if (comboReply == "y" || comboReply == "Y") {
            comboChosen = true;
            int secondChoice = chooseOption({"Coffee", "Cake", "Sandwich"}, "\n🍽️ Choose your second item:");
            if (secondChoice == 1) total += getCoffee(order);
            else if (secondChoice == 2) total += getCake(order);
            else total += getSandwich(order);
            cout << "🎉 Barista: Great choice! You've made it a combo and saved $2!\n";
            total -= 2;
        }
    }

    cout << "\n🏨 Barista: Here's your order summary:\n";
    for (auto& item : order) {
        cout << "- " << item.name << " ($" << item.price << ")\n";
    }

    cout << "\n💳 Barista: Your total is $" << total << ".\n";

    string payment;
    cout << "\n👨‍🏫 Barista: Cash or card? ";
    cin >> payment;
    cout << "\n🙏 Thank you for your payment via " << payment << "! Enjoy your order!\n\n";

    availableBaristas.release();  // Release the semaphore to allow another customer to be processed
}

int main() {
    int customerCount = 5;  // Simulating 5 customers for the example

    // Creating threads for multiple customers
    vector<thread> threads;
    for (int i = 0; i < customerCount; ++i) {
        threads.push_back(thread(processCustomer, i + 1));
    }

    // Wait for all customers to be processed
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}


