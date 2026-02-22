#define _GNU_SOURCE
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <tuple>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

string readFile(const string &filename) {
    ifstream file(filename);
    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

map<string,string> parseBody(string body) {
    map<string,string> params;
    stringstream ss(body);
    string part;

    while(getline(ss, part, '&')) {
        size_t eq = part.find('=');
        if(eq != string::npos) {
            params[part.substr(0,eq)] = part.substr(eq+1);
        }
    }
    return params;
}

map<string, tuple<string,string,int>> loadAccounts() {
    map<string, tuple<string,string,int>> accs;
    ifstream file("accounts.txt");
    string acc,name,pin;
    int bal;

    while(file >> acc >> name >> pin >> bal) {
        accs[acc] = make_tuple(name,pin,bal);
    }
    return accs;
}

void saveAccounts(map<string, tuple<string,string,int>> &accs) {
    ofstream out("accounts.txt");
    for(auto &p : accs) {
        out << p.first << " "
            << get<0>(p.second) << " "
            << get<1>(p.second) << " "
            << get<2>(p.second) << "\n";
    }
}

void sendResponse(int client, string html) {
    string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + to_string(html.size()) + "\r\n\r\n";

    string response = header + html;
    send(client, response.c_str(), response.size(), 0);
}

int main() {

    // 🔥 IMPORTANT: Use Render PORT
    int port = 8080;  // default for local

    char* portEnv = getenv("PORT");
    if(portEnv != NULL) {
        port = stoi(portEnv);
    }

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;   // MUST for Render
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    cout << "Server running on port " << port << endl;

    while(true) {

        client_socket = accept(server_fd, (struct sockaddr*)&address,
                               (socklen_t*)&addrlen);

        char buffer[10000];
        memset(buffer, 0, sizeof(buffer));
        char buffer[10000];
memset(buffer, 0, sizeof(buffer));

int totalRead = 0;
int bytesRead = read(client_socket, buffer, sizeof(buffer)-1);
if(bytesRead <= 0) {
    close(client_socket);
    continue;
}

totalRead += bytesRead;
string request(buffer, totalRead);

// Find header end
size_t headerEnd = request.find("\r\n\r\n");
if(headerEnd == string::npos) {
    close(client_socket);
    continue;
}

string headers = request.substr(0, headerEnd);
string body = "";

// If POST → read full body using Content-Length
if(request.find("POST") == 0) {

    size_t clPos = headers.find("Content-Length:");
    if(clPos != string::npos) {

        clPos += 15;
        while(headers[clPos] == ' ') clPos++;

        size_t lineEnd = headers.find("\r\n", clPos);
        int contentLength = stoi(headers.substr(clPos, lineEnd - clPos));

        size_t bodyStart = headerEnd + 4;
        body = request.substr(bodyStart);

        // If body incomplete → keep reading
        while((int)body.size() < contentLength) {
            bytesRead = read(client_socket, buffer, sizeof(buffer)-1);
            if(bytesRead <= 0) break;
            body += string(buffer, bytesRead);
        }

        body = body.substr(0, contentLength);
    }
}

        size_t headerEnd = req.find("\r\n\r\n");
        string body = "";
        if(headerEnd != string::npos)
            body = req.substr(headerEnd + 4);

        // GET request → show login page
        if(req.find("GET / ") == 0 || body.empty()) {
            string html = readFile("index.html");
            sendResponse(client_socket, html);
            close(client_socket);
            continue;
        }

        auto params = parseBody(body);
        string acc = params["acc"];
        string pin = params["pin"];
        string action = params["action"];
        string amountStr = params["amount"];

        auto accounts = loadAccounts();

        // LOGIN
        if(action == "") {

            if(accounts.count(acc) && get<1>(accounts[acc]) == pin) {

                string name = get<0>(accounts[acc]);
                int bal = get<2>(accounts[acc]);

                string html =
                    "<html><body>"
                    "<h2>Welcome " + name + "</h2>"
                    "<p>Balance: " + to_string(bal) + "</p>"
                    "<form method='POST'>"
                    "<input type='hidden' name='acc' value='" + acc + "'>"
                    "<input type='hidden' name='pin' value='" + pin + "'>"
                    "Amount: <input name='amount' required><br><br>"
                    "<button name='action' value='withdraw'>Withdraw</button>"
                    "<button name='action' value='deposit'>Deposit</button>"
                    "</form>"
                    "</body></html>";

                sendResponse(client_socket, html);

            } else {

                sendResponse(client_socket,
                    "<html><body><h2>Login Failed</h2><a href='/'>Try Again</a></body></html>");
            }

        } else {

            if(accounts.count(acc) && get<1>(accounts[acc]) == pin) {

                int amt = stoi(amountStr);
                int &bal = get<2>(accounts[acc]);

                if(action == "withdraw") bal -= amt;
                if(action == "deposit") bal += amt;

                saveAccounts(accounts);

                string html =
                    "<html><body>"
                    "<h2>Transaction Success</h2>"
                    "<p>Updated Balance: " + to_string(bal) + "</p>"
                    "<a href='/'>Logout</a>"
                    "</body></html>";

                sendResponse(client_socket, html);
            }
        }

        close(client_socket);
    }

    return 0;
}

