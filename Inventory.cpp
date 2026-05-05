#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <limits>

using namespace std;

const int LOW_STOCK_THRESHOLD = 5;
const int NEAR_EXPIRY_DAYS = 7;
const string DATA_FILE = "inventory.txt";

struct Batch {
    int quantity;
    tm expiryDate;
};

struct Product {
    string category;
    vector<Batch> batchList;
};

struct AlertCount {
    int lowStockProductCount = 0;
    int nearExpiryBatchCount = 0;
};

map<string, Product> inventory;

string toUpperCase(string text){
    for(char &c : text)
        c = toupper((unsigned char)c);
    return text;
}

tm parseDate(string dateText){
    tm date = {};
    stringstream ss(dateText);
    ss >> get_time(&date, "%Y-%m-%d");
    return date;
}

string formatDate(tm date){
    stringstream ss;
    ss << put_time(&date, "%Y-%m-%d");
    return ss.str();
}

tm getCurrentDate(){
    time_t now = time(nullptr);
    return *localtime(&now);
}

int getDaysUntilExpiry(tm expiryDate){
    time_t expiryTime = mktime(&expiryDate);
    time_t currentTime = time(nullptr);
    return difftime(expiryTime, currentTime) / (60 * 60 * 24);
}

AlertCount countAlerts(){
    AlertCount alertSummary;

    for(auto &productEntry : inventory){

        int totalProductQuantity = 0;

        for(auto &batchEntry : productEntry.second.batchList)
            totalProductQuantity += batchEntry.quantity;

        if(totalProductQuantity <= LOW_STOCK_THRESHOLD)
            alertSummary.lowStockProductCount++;

        for(auto &batchEntry : productEntry.second.batchList){
            int daysUntilExpiration = getDaysUntilExpiry(batchEntry.expiryDate);

            if(daysUntilExpiration <= NEAR_EXPIRY_DAYS)
                alertSummary.nearExpiryBatchCount++;
        }
    }

    return alertSummary;
}

void saveInventory(){
    ofstream file(DATA_FILE);

    for(auto &productEntry : inventory){
        file << productEntry.first << "|" << productEntry.second.category << "\n";

        for(auto &batchEntry : productEntry.second.batchList)
            file << batchEntry.quantity << "," << formatDate(batchEntry.expiryDate) << "\n";

        file << "END\n";
    }
}

void loadInventory(){
    ifstream file(DATA_FILE);
    if(!file) return;

    string line, productName, category;

    while(getline(file, line)){
        if(line.empty()) continue;

        stringstream ss(line);
        getline(ss, productName, '|');
        getline(ss, category);

        Product product;
        product.category = category;

        while(getline(file, line) && line != "END"){
            stringstream ss2(line);
            string quantityText, expiryText;

            getline(ss2, quantityText, ',');
            getline(ss2, expiryText);

            product.batchList.push_back({
                stoi(quantityText),
                parseDate(expiryText)
            });
        }

        inventory[productName] = product;
    }
}

void addProduct(){
    string productName, category, expiryDateText;
    int quantity;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    while(true){
        cout << "\n=== Add Product (type 'done' to stop) ===\n";

        cout << "Product name: ";
        getline(cin, productName);

        if(toUpperCase(productName) == "DONE") break;

        cout << "Category: ";
        getline(cin, category);

        cout << "Quantity: ";
        cin >> quantity;

        cout << "Expiry YYYY-MM-DD: ";
        cin >> expiryDateText;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        string upperName = toUpperCase(productName);
        string upperCategory = toUpperCase(category);

        if(!inventory.count(upperName))
            inventory[upperName] = {upperCategory, {}};

        inventory[upperName].batchList.push_back({
            quantity,
            parseDate(expiryDateText)
        });

        cout << "Product added!\n";
    }

    saveInventory();
}

void updateStock(){
    string productName;
    int choice;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Product name: ";
    getline(cin, productName);
    productName = toUpperCase(productName);

    if(!inventory.count(productName)){
        cout << "Not found.\n";
        return;
    }

    cout << "1 Add Stock OR 2 Deduct Stock: ";
    cin >> choice;

    if(choice == 1){
        int quantity;
        string expiryDateText;

        cout << "Quantity: ";
        cin >> quantity;

        cout << "Expiry YYYY-MM-DD: ";
        cin >> expiryDateText;

        inventory[productName].batchList.push_back({
            quantity,
            parseDate(expiryDateText)
        });
    }
    else if(choice == 2){
        int quantityToDeduct;
        cout << "Quantity to deduct: ";
        cin >> quantityToDeduct;

        for(auto &batchEntry : inventory[productName].batchList){

            if(quantityToDeduct <= 0) break;

            if(batchEntry.quantity >= quantityToDeduct){
                batchEntry.quantity -= quantityToDeduct;
                quantityToDeduct = 0;
            }
            else{
                quantityToDeduct -= batchEntry.quantity;
                batchEntry.quantity = 0;
            }
        }

        inventory[productName].batchList.erase(
            remove_if(inventory[productName].batchList.begin(),
                      inventory[productName].batchList.end(),
                      [](Batch b){ return b.quantity == 0; }),
            inventory[productName].batchList.end()
        );
    }
    else{
        cout << "Invalid choice.\n";
    }

    saveInventory();
}

void editBatch(){
    string productName;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Product name: ";
    getline(cin, productName);
    productName = toUpperCase(productName);

    if(!inventory.count(productName)){
        cout << "Product not found.\n";
        return;
    }

    // Show all batches
    cout << "\nBatches of " << productName << ":\n";
    int index = 1;
    for(auto &batchEntry : inventory[productName].batchList){
        cout << index++ << ". Qty: " << batchEntry.quantity
             << " | Exp: " << formatDate(batchEntry.expiryDate) << endl;
    }

    int choice;
    cout << "Select batch #: ";
    cin >> choice;

    if(choice < 1 || choice > inventory[productName].batchList.size()){
        cout << "Invalid selection.\n";
        return;
    }

    // Get selected batch reference
    Batch &selectedBatch = inventory[productName].batchList[choice - 1];

    // Ask what to edit
    cout << "\n1 Edit Quantity\n";
    cout << "2 Edit Expiry Date\n";
    cout << "3 Edit BOTH\n";
    cout << "Choice: ";

    int editChoice;
    cin >> editChoice;

    if(editChoice == 1 || editChoice == 3){
        cout << "Enter new quantity: ";
        cin >> selectedBatch.quantity;
    }

    if(editChoice == 2 || editChoice == 3){
        string newExpiry;
        cout << "Enter new expiry (YYYY-MM-DD): ";
        cin >> newExpiry;
        selectedBatch.expiryDate = parseDate(newExpiry);
    }

    cout << "Batch updated successfully!\n";
    saveInventory();
}

void generate_inventory_report(){
    cout << "\n================ INVENTORY REPORT ================\n";

    AlertCount alertSummary = countAlerts();

    cout << "LOW STOCK PRODUCTS: " << alertSummary.lowStockProductCount << "\n";
    cout << "NEAR EXPIRY BATCHES: " << alertSummary.nearExpiryBatchCount << "\n\n";

    cout << left
         << setw(25) << "PRODUCT NAME"
         << setw(15) << "CATEGORY"
         << setw(10) << "QUANTITY"
         << setw(15) << "EXPIRY"
         << "STATUS ALERT"
         << endl;

    cout << string(80, '-') << endl;

    for(auto &productEntry : inventory){

        int totalQuantity = 0;

        for(auto &batchEntry : productEntry.second.batchList)
            totalQuantity += batchEntry.quantity;

        for(auto &batchEntry : productEntry.second.batchList){

            int daysUntilExpiration = getDaysUntilExpiry(batchEntry.expiryDate);

            cout << left
                 << setw(25) << productEntry.first
                 << setw(15) << productEntry.second.category
                 << setw(10) << batchEntry.quantity
                 << setw(15) << formatDate(batchEntry.expiryDate);

            string status;

            if(totalQuantity <= LOW_STOCK_THRESHOLD)
                status += "LOW STOCK ";

            if(daysUntilExpiration <= NEAR_EXPIRY_DAYS)
                status += "NEAR EXPIRY";

            cout << status << endl;
        }
    }

    cout << string(80, '=') << endl;
}

// ---------- MAIN ----------

int main(){
    loadInventory();

    int choice;

    while(true){
        cout << "\n1 Add Product\n2 Update Product\n3 Edit Batch\n4 Generate Report\n5 Exit\nChoice: ";
        cin >> choice;

        if(choice == 1) addProduct();
        else if(choice == 2) updateStock();
        else if(choice == 3) editBatch();
        else if(choice == 4) generate_inventory_report();
        else break;
    }
}