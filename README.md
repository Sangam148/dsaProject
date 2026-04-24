
# Inventory Management System

A modern C++ inventory management system demonstrating best practices with smart pointers, exception handling, and efficient data structures.

## Overview

This project implements a lightweight inventory management system using contemporary C++ features. It provides core functionality for managing product inventory, including adding items, tracking quantities, and processing purchases with built-in validation and error handling.

## Features

- **Item Management**: Add items to inventory with name, price, and quantity
- **Purchase Processing**: Handle item purchases with stock validation
- **Error Handling**: Comprehensive exception handling for invalid operations
- **Smart Memory Management**: Uses `std::unique_ptr` for automatic memory management
- **Efficient Lookups**: Leverages `std::unordered_map` for O(1) average-case item retrieval
- **Type Safety**: Modern C++ with strong typing and const-correctness

## Architecture

### Classes

#### `InventoryItem`
Represents a single product in the inventory.

**Constructor:**
- `InventoryItem(const std::string& name, double price, int quantity)` - Creates a new inventory item

**Public Methods:**
- `getName()` - Returns the item name
- `getPrice()` - Returns the item price
- `getQuantity()` - Returns current stock quantity
- `updateQuantity(int quantity)` - Adds or removes quantity (positive/negative values)

#### `Inventory`
Manages the collection of inventory items.

**Public Methods:**
- `addItem(const std::string& name, double price, int quantity)` - Adds a new item to inventory
  - Throws `std::invalid_argument` if quantity is negative
  - Uses `std::make_unique` for safe memory allocation

- `purchaseItem(const std::string& name, int quantity)` - Processes a purchase
  - Throws `std::runtime_error` if item not found
  - Throws `std::runtime_error` if insufficient stock
  - Automatically updates quantity on successful purchase

## Usage

```cpp
Inventory inventory;

// Add items
inventory.addItem("Widget", 19.99, 100);
inventory.addItem("Gadget", 29.99, 50);

// Purchase items
try {
    inventory.purchaseItem("Widget", 5);
    std::cout << "Purchase successful!" << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## Modern C++ Best Practices Used

1. **Smart Pointers**: `std::unique_ptr` for automatic memory management (RAII principle)
2. **Standard Containers**: `std::unordered_map` for efficient key-value storage
3. **Exception Safety**: Proper exception handling with meaningful error messages
4. **Const Correctness**: Methods marked `const` where appropriate
5. **Move Semantics**: Efficient memory usage through emplace operations
6. **Header Includes**: Only necessary standard library headers included

## Compilation

```bash
g++ -std=c++11 dsaProject.cpp -o inventory
```

Or with clang:
```bash
clang++ -std=c++11 dsaProject.cpp -o inventory
```

## Requirements

- C++11 or later
- Standard C++ Library

## Future Enhancements

- Transaction logging and history tracking
- Persistent storage (database/file-based)
- Multi-item purchases in single transaction
- Inventory analytics and reporting
- Restocking alerts for low stock items
- Product categorization

## License

This project is part of the DSA (Data Structures & Algorithms) learning project.

## Author

**Sangam148** - [GitHub Profile](https://github.com/Sangam148)

---

*Last Updated: April 24, 2026*
