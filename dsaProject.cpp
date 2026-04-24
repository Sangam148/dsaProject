#include <iostream>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <string>

class InventoryItem {
public:
    InventoryItem(const std::string& name, double price, int quantity)
        : name(name), price(price), quantity(quantity) {}

    const std::string& getName() const { return name; }
    double getPrice() const { return price; }
    int getQuantity() const { return quantity; }
    void updateQuantity(int quantity) { this->quantity += quantity; }

private:
    std::string name;
    double price;
    int quantity;
};

class Inventory {
public:
    void addItem(const std::string& name, double price, int quantity) {
        if (quantity < 0) throw std::invalid_argument("Quantity cannot be negative");
        items.emplace(name, std::make_unique<InventoryItem>(name, price, quantity));
    }

    void purchaseItem(const std::string& name, int quantity) {
        auto it = items.find(name);
        if (it == items.end()) throw std::runtime_error("Item not found");
        if (it->second->getQuantity() < quantity) throw std::runtime_error("Not enough stock");

        it->second->updateQuantity(-quantity);
        // handle transaction management here
    }

private:
    std::unordered_map<std::string, std::unique_ptr<InventoryItem>> items;
};

int main() {
    Inventory inventory;
    try {
        inventory.addItem("Widget", 19.99, 100);
        inventory.purchaseItem("Widget", 1);
        std::cout << "Transaction successful." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}