#define _GNU_SOURCE
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

struct Account {
    string acc;
    string name;
    string pin;
    int balance;
};

vector<Account> loadAccounts() {
    vector<Account> accounts;
    ifstream file("accounts.txt");
    string acc, name, pin;
    int balance;

    while(file >> acc >> name >> pin >> balance) {
        accounts.push_back({acc, name, pin, balance});
    }
    return accounts;
}

void saveAccounts(vector<Account>& accounts) {
    ofstream file("accounts.txt");
    for(auto &a : accounts) {
        file << a.acc << " " << a.name << " "
             << a.pin << " " << a.balance << "\n";
    }
}

string loginPage() {
    return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
           "<h2>ATM Login</h2>"
           "<form method='POST'>"
           "Account: <input name='acc'><br><br>"
           "PIN: <input type='password' name='pin'><br><br>"
           "<button type='submit'>Login</button>"
           "</form>";
}

string dashboard(Account& user) {
    stringstream ss;
    ss << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    ss << "<h2>Welcome " << user.name << "</h2>";
    ss << "<p>Balance: ₹" << user.balance << "</p>";

    ss << "<h3>Deposit</h3>"
       << "<form method='POST'>"
       << "<input type='hidden' name='acc' value='" << user.acc << "'>"
       << "<input type='hidden' name='pin' value='" << user.pin << "'>"
       << "<input name='deposit'>"
       << "<button type='submit'>Deposit</button>"
       << "</form>";

    ss << "<h3>Withdraw</h3>"
       << "<form method='POST'>"
       << "<input type='hidden' name='acc' value='" << user.acc << "'>"
       << "<input type='hidden' name='pin' value='" << user.pin << "'>"
       << "<input name='withdraw'>"
       << "<button type='submit'>Withdraw</button>"
       << "</form>";

    return ss.str();
}

string getValue(string body, string key) {
    size_t pos = body.find(key + "=");
    if(pos == string::npos) return "";
    pos += key.length() + 1;
    size_t end = body.find("&", pos);
    if(end == string::npos) end = body.length();
    return body.substr(pos, end - pos);
}

int main() {

    int port = 8080;
    char* portEnv = getenv("PORT");
    if(portEnv != NULL) {
        port = atoi(portEnv);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    cout << "Server running on port " << port << endl;

    while(true) {

        int client = accept(server_fd, NULL, NULL);

        char buffer[10000];
        memset(buffer, 0, sizeof(buffer));

        int bytesRead = read(client, buffer, sizeof(buffer)-1);
        if(bytesRead <= 0) {
            close(client);
            continue;
        }

        string request(buffer, bytesRead);

        size_t headerEnd = request.find("\r\n\r\n");
        if(headerEnd == string::npos) {
            close(client);
            continue;
        }

        string headers = request.substr(0, headerEnd);
        string body = "";

        if(request.find("POST") == 0) {

            size_t clPos = headers.find("Content-Length:");
            if(clPos != string::npos) {

                clPos += 15;
                while(headers[clPos] == ' ') clPos++;

                size_t lineEnd = headers.find("\r\n", clPos);
                int contentLength = stoi(headers.substr(clPos, lineEnd - clPos));

                size_t bodyStart = headerEnd + 4;
                body = request.substr(bodyStart);

                while((int)body.size() < contentLength) {
                    bytesRead = read(client, buffer, sizeof(buffer)-1);
                    if(bytesRead <= 0) break;
                    body += string(buffer, bytesRead);
                }

                body = body.substr(0, contentLength);
            }
        }

        if(body.empty()) {
            string response = loginPage();
            send(client, response.c_str(), response.size(), 0);
            close(client);
            continue;
        }

        string acc = getValue(body, "acc");
        string pin = getValue(body, "pin");

        vector<Account> accounts = loadAccounts();
        bool found = false;

        for(auto &a : accounts) {

            if(a.acc == acc && a.pin == pin) {
                found = true;

                string dep = getValue(body, "deposit");
                string wit = getValue(body, "withdraw");

                if(!dep.empty()) {
                    a.balance += stoi(dep);
                }

                if(!wit.empty()) {
                    int w = stoi(wit);
                    if(w <= a.balance)
                        a.balance -= w;
                }

                saveAccounts(accounts);

                string response = dashboard(a);
                send(client, response.c_str(), response.size(), 0);
                break;
            }
        }

        if(!found) {
            string response = loginPage();
            send(client, response.c_str(), response.size(), 0);
        }

        close(client);
    }

    return 0;
}
