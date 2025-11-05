// inventory.cpp
// g++ -std=c++17 inventory.cpp -o inventory


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <climits>
#include <cctype>
#include <memory>

using namespace std;
/*
Features:
- Linked List for main inventory storage
- BST index by product ID for fast search
- Queue for orders / simple cart handling and billing (file saved)
- Stack for Undo/Redo actions
- Min-Heap for low-stock / earliest-expiry alerts
- Sorting views and reports
- File persistence (CSV)
*/

// ---------- Product Node (Linked List) ----------
struct Product {
    int id;
    string name;
    string category;
    int qty;
    double price;
    string expiry; // YYYY-MM-DD format (optional, can be empty)
    Product* next;
    Product(int i, const string& n, const string& c, int q, double p, const string& e)
        : id(i), name(n), category(c), qty(q), price(p), expiry(e), next(nullptr) {}
};

// ---------- BST Node (index to Product pointer) ----------
struct BSTNode {
    int id;
    Product* prodPtr;
    BSTNode* left;
    BSTNode* right;
    BSTNode(int i, Product* p) : id(i), prodPtr(p), left(nullptr), right(nullptr) {}
};

// ---------- Action for Undo/Redo ----------
enum ActionType { ADD_PROD, DELETE_PROD, UPDATE_QTY };
struct Action {
    ActionType type;
    // For add/delete we store the full product snapshot
    Product* snapshot; // deep-copied node (allocated)
    // For update we store id and previous qty
    int prevQty;
    int id;
    Action(ActionType t=ADD_PROD, Product* s=nullptr, int pid=0, int pq=0)
        : type(t), snapshot(s), prevQty(pq), id(pid) {}
};

// ---------- Inventory Class ----------
class Inventory {
private:
    Product* head;
    BSTNode* bstRoot;

    // helper: delete entire BST
    void deleteBST(BSTNode* node) {
        if (!node) return;
        deleteBST(node->left);
        deleteBST(node->right);
        delete node;
    }

    // helper: insert into BST (by id)
    BSTNode* bstInsertRec(BSTNode* root, int id, Product* p) {
        if (!root) return new BSTNode(id, p);
        if (id < root->id) root->left = bstInsertRec(root->left, id, p);
        else if (id > root->id) root->right = bstInsertRec(root->right, id, p);
        // if equal id, replace pointer (should not happen normally)
        else root->prodPtr = p;
        return root;
    }

    // helper: find min in BST
    BSTNode* bstFindMin(BSTNode* node) {
        while (node && node->left) node = node->left;
        return node;
    }

    // helper: delete id from BST
    BSTNode* bstDeleteRec(BSTNode* root, int id) {
        if (!root) return nullptr;
        if (id < root->id) root->left = bstDeleteRec(root->left, id);
        else if (id > root->id) root->right = bstDeleteRec(root->right, id);
        else {
            // found
            if (!root->left) {
                BSTNode* r = root->right;
                delete root;
                return r;
            } else if (!root->right) {
                BSTNode* l = root->left;
                delete root;
                return l;
            } else {
                BSTNode* succ = bstFindMin(root->right);
                root->id = succ->id;
                root->prodPtr = succ->prodPtr;
                root->right = bstDeleteRec(root->right, succ->id);
            }
        }
        return root;
    }

    // helper: search BST
    BSTNode* bstSearchRec(BSTNode* root, int id) {
        if (!root) return nullptr;
        if (root->id == id) return root;
        if (id < root->id) return bstSearchRec(root->left, id);
        return bstSearchRec(root->right, id);
    }

public:
    Inventory() : head(nullptr), bstRoot(nullptr) {}
    ~Inventory() {
        // delete linked list
        Product* cur = head;
        while (cur) {
            Product* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
        // delete BST
        deleteBST(bstRoot);
    }

    // create snapshot deep copy
    static Product* cloneProduct(Product* p) {
        if (!p) return nullptr;
        Product* cp = new Product(p->id, p->name, p->category, p->qty, p->price, p->expiry);
        return cp;
    }

    // add product at list end; return pointer to new Product
    Product* addProduct(int id, const string& name, const string& cat, int qty, double price, const string& expiry) {
        // check duplicate id - don't allow
        if (searchById(id) != nullptr) {
            cout << "Product with ID " << id << " already exists. Use update instead.\n";
            return nullptr;
        }
        Product* np = new Product(id, name, cat, qty, price, expiry);
        if (!head) head = np;
        else {
            Product* t = head;
            while (t->next) t = t->next;
            t->next = np;
        }
        bstRoot = bstInsertRec(bstRoot, id, np);
        return np;
    }

    // delete product by id; returns snapshot (caller should manage deletion)
    Product* deleteProduct(int id) {
        if (!head) return nullptr;
        Product* prev = nullptr;
        Product* cur = head;
        while (cur && cur->id != id) {
            prev = cur;
            cur = cur->next;
        }
        if (!cur) return nullptr;
        // snapshot
        Product* snap = cloneProduct(cur);
        // unlink
        if (!prev) head = cur->next;
        else prev->next = cur->next;
        // remove from BST
        bstRoot = bstDeleteRec(bstRoot, id);
        delete cur;
        return snap;
    }

    // update quantity for id; returns old qty or -1 if not found
    int updateQuantity(int id, int newQty) {
        Product* p = searchById(id);
        if (!p) return -1;
        int old = p->qty;
        p->qty = newQty;
        return old;
    }

    // search by id (fast via BST)
    Product* searchById(int id) {
        BSTNode* b = bstSearchRec(bstRoot, id);
        return b ? b->prodPtr : nullptr;
    }

    // search by name/category via linear scan (returns first found)
    Product* searchByName(const string& name) {
        Product* t = head;
        while (t) {
            if (t->name == name) return t;
            t = t->next;
        }
        return nullptr;
    }

    // display all products
    void displayAll() {
        cout << "\n--- Inventory ---\n";
        Product* t = head;
        if (!t) {
            cout << "No products.\n";
            return;
        }
        while (t) {
            cout << "ID:" << t->id << " | " << t->name << " | Cat:" << t->category
                 << " | Qty:" << t->qty << " | Rs." << t->price
                 << (t->expiry.empty() ? "" : " | Exp:" + t->expiry) << '\n';
            t = t->next;
        }
    }

    // create a vector of product pointers for sorting/processing
    vector<Product*> toVector() {
        vector<Product*> v;
        Product* t = head;
        while (t) { v.push_back(t); t = t->next; }
        return v;
    }

    // sorting helpers and display
    void displaySortedByPrice(bool ascending=true) {
        auto v = toVector();
        sort(v.begin(), v.end(), [ascending](Product* a, Product* b){
            return ascending ? (a->price < b->price) : (a->price > b->price);
        });
        cout << "\n-- Products sorted by Price --\n";
        for (auto p : v) cout << p->id << " | " << p->name << " | Rs." << p->price << " | Qty:" << p->qty << '\n';
    }
    void displaySortedByName() {
        auto v = toVector();
        sort(v.begin(), v.end(), [](Product* a, Product* b){
            return a->name < b->name;
        });
        cout << "\n-- Products sorted by Name --\n";
        for (auto p : v) cout << p->id << " | " << p->name << " | Rs." << p->price << " | Qty:" << p->qty << '\n';
    }
    void displaySortedByQty(bool ascending=true) {
        auto v = toVector();
        sort(v.begin(), v.end(), [ascending](Product* a, Product* b){
            return ascending ? (a->qty < b->qty) : (a->qty > b->qty);
        });
        cout << "\n-- Products sorted by Quantity --\n";
        for (auto p : v) cout << p->id << " | " << p->name << " | Qty:" << p->qty << " | Rs." << p->price << '\n';
    }

    // statistics
    double totalStockValue() {
        double sum = 0;
        Product* t = head;
        while (t) { sum += t->qty * t->price; t = t->next; }
        return sum;
    }
    Product* mostExpensive() {
        Product* t = head;
        if (!t) return nullptr;
        Product* best = t;
        while (t) { if (t->price > best->price) best = t; t = t->next; }
        return best;
    }
    vector<Product*> outOfStockList() {
        vector<Product*> v;
        Product* t = head;
        while (t) { if (t->qty <= 0) v.push_back(t); t = t->next; }
        return v;
    }

    // low stock alerts using min-heap by qty
    void lowStockAlerts(int threshold = 5) {
        struct Cmp { bool operator()(Product* a, Product* b) { return a->qty > b->qty; } };
        priority_queue<Product*, vector<Product*>, Cmp> pq;
        Product* t = head;
        while (t) { pq.push(t); t = t->next; }
        if (pq.empty()) { cout << "No products.\n"; return; }
        cout << "\n-- Low Stock Alerts (threshold " << threshold << ") --\n";
        bool any = false;
        while (!pq.empty()) {
            Product* p = pq.top(); pq.pop();
            if (p->qty < threshold) {
                cout << p->id << " | " << p->name << " | Qty:" << p->qty << '\n';
                any = true;
            } else break;
        }
        if (!any) cout << "All products sufficiently stocked.\n";
    }

    // earliest expiry product
    Product* earliestExpiry() {
        Product* t = head;
        Product* best = nullptr;
        while (t) {
            if (!t->expiry.empty()) {
                if (!best || t->expiry < best->expiry) best = t;
            }
            t = t->next;
        }
        return best;
    }

    // persistence: save to CSV
    void saveToFile(const string& filename = "inventory.csv") {
        ofstream ofs(filename);
        if (!ofs) { cerr << "Error saving inventory file.\n"; return; }
        // header
        ofs << "id,name,category,qty,price,expiry\n";
        Product* t = head;
        while (t) {
            ofs << t->id << ',' << escapeCSV(t->name) << ',' << escapeCSV(t->category) << ',' << t->qty << ',' << t->price << ',' << t->expiry << '\n';
            t = t->next;
        }
        ofs.close();
    }

    // load from CSV (clears existing)
    void loadFromFile(const string& filename = "inventory.csv") {
        ifstream ifs(filename);
        if (!ifs) {
            // file not found yet
            return;
        }
        // clear existing
        // delete linked list
        Product* cur = head;
        while (cur) { Product* tmp = cur; cur = cur->next; delete tmp; }
        head = nullptr;
        deleteBST(bstRoot); bstRoot = nullptr;

        string line;
        getline(ifs, line); // header
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            vector<string> parts = splitCSV(line);
            if (parts.size() < 6) continue;
            int id = stoi(parts[0]);
            string name = parts[1];
            string cat = parts[2];
            int qty = stoi(parts[3]);
            double price = stod(parts[4]);
            string expiry = parts[5];
            addProduct(id, name, cat, qty, price, expiry);
        }
        ifs.close();
    }

    // helper: CSV escape (wrap in quotes if contains comma)
    static string escapeCSV(const string& s) {
        if (s.find(',') == string::npos && s.find('"') == string::npos) return s;
        string r = "\"";
        for (char c : s) {
            if (c == '"') r += "\"\"";
            else r += c;
        }
        r += "\"";
        return r;
    }

    // helper: split CSV (basic, handles quoted fields)
    static vector<string> splitCSV(const string& line) {
        vector<string> res;
        string cur;
        bool inQuotes = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"' ) {
                inQuotes = !inQuotes;
                continue;
            }
            if (c == ',' && !inQuotes) {
                res.push_back(cur);
                cur.clear();
            } else cur.push_back(c);
        }
        res.push_back(cur);
        return res;
    }

    // helper: for undo restore (inserting a cloned product)
    Product* insertSnapshot(Product* snap) {
        if (!snap) return nullptr;
        Product* np = new Product(snap->id, snap->name, snap->category, snap->qty, snap->price, snap->expiry);
        if (!head) head = np;
        else {
            Product* t = head;
            while (t->next) t = t->next;
            t->next = np;
        }
        bstRoot = bstInsertRec(bstRoot, np->id, np);
        return np;
    }

    // display single product
    void displayProduct(Product* p) {
        if (!p) { cout << "Product not found.\n"; return; }
        cout << "ID:" << p->id << " | " << p->name << " | Cat:" << p->category
             << " | Qty:" << p->qty << " | Rs." << p->price
             << (p->expiry.empty() ? "" : " | Exp:" + p->expiry) << '\n';
    }
};

// ---------- Order / Billing ----------
struct CartItem {
    int id;
    int qty;
};

class BillingSystem {
private:
    Inventory* inv;
public:
    BillingSystem(Inventory* inventory) : inv(inventory) {}

    // process a cart: vector<CartItem> cart; returns total amount and writes bill to file
    double processOrder(const vector<CartItem>& cart, const string& customerName = "Customer") {
        double total = 0;
        vector<pair<Product*, int>> billed;
        for (auto &ci : cart) {
            Product* p = inv->searchById(ci.id);
            if (!p) {
                cout << "Product ID " << ci.id << " not found. Skipping.\n";
                continue;
            }
            if (p->qty < ci.qty) {
                cout << "Not enough stock for " << p->name << ". Available " << p->qty << ". Taking available quantity.\n";
            }
            int sold = min(ci.qty, p->qty);
            if (sold <= 0) continue;
            // deduct from inventory
            int newQty = p->qty - sold;
            inv->updateQuantity(p->id, newQty);
            billed.push_back({p, sold});
            total += sold * p->price;
        }
        // write bill to file
        ofstream ofs("bills.txt", ios::app);
        if (ofs) {
            ofs << "----- BILL -----\n";
            ofs << "Customer: " << customerName << "\n";
            ofs << "Date: " << currentDateTime() << "\n";
            for (auto &b : billed) {
                ofs << b.first->id << " | " << b.first->name << " | Qty:" << b.second << " | Rs." << (b.second * b.first->price) << "\n";
            }
            ofs << "Total: Rs." << total << "\n";
            ofs << "----------------\n\n";
            ofs.close();
        } else {
            cout << "Warning: could not write bill to file.\n";
        }
        cout << "Order processed. Total = Rs." << total << '\n';
        return total;
    }

    static string currentDateTime() {
        time_t now = time(nullptr);
        tm* l = localtime(&now);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", l);
        return string(buf);
    }
};

// ---------- Undo/Redo Manager ----------
class UndoRedoManager {
private:
    stack<Action> undoSt;
    stack<Action> redoSt;
    Inventory* inv;
public:
    UndoRedoManager(Inventory* invRef) : inv(invRef) {}

    // record actions: for add -> snapshot of product; for delete -> snapshot; for update -> id and prevQty
    void recordAdd(Product* pSnapshot) {
        // snapshot should be deep copy
        undoSt.push(Action(ADD_PROD, pSnapshot));
        // clear redo
        clearStack(redoSt);
    }
    void recordDelete(Product* pSnapshot) {
        undoSt.push(Action(DELETE_PROD, pSnapshot));
        clearStack(redoSt);
    }
    void recordUpdate(int id, int prevQty) {
        Action a(UPDATE_QTY, nullptr, id, prevQty);
        undoSt.push(a);
        clearStack(redoSt);
    }

    void clearStack(stack<Action>& s) {
        while (!s.empty()) {
            // free snapshot if present
            if (s.top().snapshot) delete s.top().snapshot;
            s.pop();
        }
    }

    void undo() {
        if (undoSt.empty()) { cout << "Nothing to undo.\n"; return; }
        Action a = undoSt.top(); undoSt.pop();
        if (a.type == ADD_PROD) {
            // an ADD was done earlier; undo by deleting that id
            int id = a.snapshot->id;
            Product* deletedSnap = inv->deleteProduct(id);
            // push reverse action (to redo) -> we need snapshot of deleted product (deletedSnap)
            redoSt.push(Action(ADD_PROD, deletedSnap));
            // free original snapshot
            delete a.snapshot;
            cout << "Undone: Added product ID " << id << '\n';
        } else if (a.type == DELETE_PROD) {
            // a delete happened earlier; undo by inserting the snapshot
            Product* rest = inv->insertSnapshot(a.snapshot); // a.snapshot becomes part of inventory; do NOT delete here
            // for redo, we want to delete this id again -> push snapshot clone
            Product* clone = Inventory::cloneProduct(a.snapshot);
            redoSt.push(Action(DELETE_PROD, clone));
            cout << "Undone: Deleted product restored ID " << a.snapshot->id << '\n';
            // note: a.snapshot was used for insertSnapshot and should NOT be deleted here
        } else if (a.type == UPDATE_QTY) {
            // revert quantity to prevQty
            int id = a.id;
            int prev = a.prevQty;
            Product* p = inv->searchById(id);
            if (p) {
                int cur = p->qty;
                inv->updateQuantity(id, prev);
                // push redo action storing current (which was before undo) to redo
                redoSt.push(Action(UPDATE_QTY, nullptr, id, cur));
                cout << "Undone: Updated product ID " << id << " restored quantity from " << cur << " to " << prev << '\n';
            } else {
                cout << "Undo failed: product not found.\n";
            }
        }
    }

    void redo() {
        if (redoSt.empty()) { cout << "Nothing to redo.\n"; return; }
        Action a = redoSt.top(); redoSt.pop();
        if (a.type == ADD_PROD) {
            // redo an add -> insert snapshot
            Product* p = inv->insertSnapshot(a.snapshot);
            // push to undo stack the snapshot (note: we transferred ownership to inventory; make a clone for undo)
            Product* clone = Inventory::cloneProduct(a.snapshot);
            undoSt.push(Action(ADD_PROD, clone));
            cout << "Redone: Added product ID " << a.snapshot->id << '\n';
            // a.snapshot used for insertSnapshot, should not be deleted here
        } else if (a.type == DELETE_PROD) {
            // redo a delete -> delete id
            int id = a.snapshot->id;
            Product* deleted = inv->deleteProduct(id);
            // push to undo stack snapshot of deleted for undo
            undoSt.push(Action(DELETE_PROD, deleted));
            cout << "Redone: Deleted product ID " << id << '\n';
            // a.snapshot was only for redo; delete it
            delete a.snapshot;
        } else if (a.type == UPDATE_QTY) {
            int id = a.id;
            int prev = a.prevQty;
            Product* p = inv->searchById(id);
            if (p) {
                int old = p->qty;
                inv->updateQuantity(id, prev);
                undoSt.push(Action(UPDATE_QTY, nullptr, id, old));
                cout << "Redone: Updated product ID " << id << " set quantity to " << prev << '\n';
            }
        }
    }
};

// ---------- Simple Menu & Interaction ----------
void adminMenu(Inventory& inv, BillingSystem& biller, UndoRedoManager& urm) {
    while (true) {
        cout << "\n--- ADMIN MENU ---\n";
        cout << "1) Add Product\n2) Delete Product\n3) Update Quantity\n4) Search Product (by ID)\n5) Display All\n6) Sorted Views\n7) Low Stock Alerts\n8) Reports & Stats\n9) Undo\n10) Redo\n11) Save Inventory\n0) Logout\nChoice: ";
        int ch; cin >> ch;
        if (ch == 0) break;
        if (ch == 1) {
            int id, qty; double price; string name, cat, expiry;
            cout << "Enter ID: "; cin >> id;
            cout << "Enter Name: "; cin.ignore(); getline(cin, name);
            cout << "Enter Category: "; getline(cin, cat);
            cout << "Enter Quantity: "; cin >> qty;
            cout << "Enter Price: "; cin >> price;
            cout << "Enter Expiry (YYYY-MM-DD) or blank: "; cin.ignore(); getline(cin, expiry);
            Product* added = inv.addProduct(id, name, cat, qty, price, expiry);
            if (added) {
                // snapshot for undo (clone)
                Product* snap = Inventory::cloneProduct(added);
                urm.recordAdd(snap);
            }
        } else if (ch == 2) {
            int id; cout << "Enter ID to delete: "; cin >> id;
            Product* snap = inv.deleteProduct(id);
            if (snap) {
                urm.recordDelete(snap); // ownership transferred to undo manager (will delete or insert)
            } else cout << "Product not found.\n";
        } else if (ch == 3) {
            int id, newq; cout << "Enter ID and new quantity: "; cin >> id >> newq;
            Product* p = inv.searchById(id);
            if (p) {
                int old = p->qty;
                inv.updateQuantity(id, newq);
                urm.recordUpdate(id, old);
                cout << "Updated.\n";
            } else cout << "Not found.\n";
        } else if (ch == 4) {
            int id; cout << "Enter ID: "; cin >> id;
            Product* p = inv.searchById(id);
            inv.displayProduct(p);
        } else if (ch == 5) {
            inv.displayAll();
        } else if (ch == 6) {
            cout << "a) By Price asc b) By Price desc c) By Name d) By Qty desc\nChoose: ";
            char c; cin >> c;
            if (c == 'a') inv.displaySortedByPrice(true);
            else if (c == 'b') inv.displaySortedByPrice(false);
            else if (c == 'c') inv.displaySortedByName();
            else if (c == 'd') inv.displaySortedByQty(false);
        } else if (ch == 7) {
            inv.lowStockAlerts();
            cout << "Auto-restock threshold? Enter 0 to skip, else amount to restock to: ";
            int restTo; cin >> restTo;
            if (restTo > 0) {
                auto v = inv.toVector();
                for (auto p : v) {
                    if (p->qty < 5) {
                        inv.updateQuantity(p->id, restTo);
                        cout << "Auto-restocked " << p->name << " to " << restTo << '\n';
                    }
                }
            }
        } else if (ch == 8) {
            cout << "Total stock value: Rs." << inv.totalStockValue() << '\n';
            Product* me = inv.mostExpensive();
            if (me) cout << "Most expensive: " << me->name << " Rs." << me->price << '\n';
            auto outs = inv.outOfStockList();
            if (!outs.empty()) {
                cout << "Out of stock items:\n";
                for (auto p : outs) cout << p->id << " | " << p->name << '\n';
            }
            Product* soon = inv.earliestExpiry();
            if (soon) cout << "Earliest expiry: " << soon->name << " Exp:" << soon->expiry << '\n';
        } else if (ch == 9) {
            urm.undo();
        } else if (ch == 10) {
            urm.redo();
        } else if (ch == 11) {
            inv.saveToFile();
            cout << "Saved.\n";
        } else {
            cout << "Invalid.\n";
        }
    }
}

void customerMenu(Inventory& inv, BillingSystem& biller) {
    queue<string> orderQueue;
    while (true) {
        cout << "\n--- CUSTOMER MENU ---\n";
        cout << "1) View All Products\n2) Search by ID\n3) Add to Cart\n4) Checkout (Process Cart)\n5) Add Simple Order (queue)\n6) Process Next Order\n0) Logout\nChoice: ";
        int ch; cin >> ch;
        if (ch == 0) break;
        static vector<CartItem> cart;
        if (ch == 1) inv.displayAll();
        else if (ch == 2) {
            int id; cout << "Enter ID: "; cin >> id;
            Product* p = inv.searchById(id);
            inv.displayProduct(p);
        } else if (ch == 3) {
            int id, qty; cout << "Enter ID and quantity to add to cart: "; cin >> id >> qty;
            cart.push_back({id, qty});
            cout << "Added to cart.\n";
        } else if (ch == 4) {
            if (cart.empty()) { cout << "Cart empty.\n"; continue; }
            string cname; cout << "Enter customer name: "; cin.ignore(); getline(cin, cname);
            biller.processOrder(cart, cname);
            cart.clear();
        } else if (ch == 5) {
            cout << "Enter a short order description: ";
            cin.ignore(); string s; getline(cin, s);
            orderQueue.push(s);
            cout << "Order queued.\n";
        } else if (ch == 6) {
            if (orderQueue.empty()) cout << "No queued orders.\n";
            else { cout << "Processing queued order: " << orderQueue.front() << '\n'; orderQueue.pop(); }
        } else {
            cout << "Invalid.\n";
        }
    }
}

int main() {
    Inventory inventory;
    // load existing inventory if any
    inventory.loadFromFile("inventory.csv");

    BillingSystem biller(&inventory);
    UndoRedoManager urm(&inventory);

    // sample ADMIN password (simple)
    const string ADMIN_PASS = "admin123";

    cout << "==== Inventory & Stock Management System ====\n";
    while (true) {
        cout << "\nMain Menu:\n1) Admin Login\n2) Customer\n3) Save & Exit\nChoice: ";
        int ch; cin >> ch;
        if (ch == 1) {
            string pass; cout << "Enter admin password: "; cin >> pass;
            if (pass == ADMIN_PASS) {
                adminMenu(inventory, biller, urm);
            } else cout << "Wrong password.\n";
        } else if (ch == 2) {
            customerMenu(inventory, biller);
        } else if (ch == 3) {
            inventory.saveToFile("inventory.csv");
            cout << "Saved inventory to inventory.csv. Exiting.\n";
            break;
        } else {
            cout << "Invalid.\n";
        }
    }
    return 0;
}
