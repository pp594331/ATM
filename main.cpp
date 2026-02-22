#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
  #define CLOSESOCKET closesocket
#else
  #include <netinet/in.h>
  #include <unistd.h>
  #include <sys/socket.h>
  #define CLOSESOCKET close
#endif

using namespace std;

/* ========== DATA STRUCTURES ========== */

struct Account {
    string accNo;
    string name;
    string pin;
    int balance;
};

struct Transaction {
    string accNo;
    string type;    // DEPOSIT, WITHDRAW, TRANSFER_IN, TRANSFER_OUT
    int amount;
    int balanceAfter;
    string timestamp;
    string detail;  // extra info like "from 5689" or "to 5790"
};

/* ========== UTILITY FUNCTIONS ========== */

string getTimestamp()
{
    time_t now = time(0);
    struct tm* t = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    return string(buf);
}

string urlDecode(const string& s)
{
    string result;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '+') result += ' ';
        else if (s[i] == '%' && i + 2 < s.length()) {
            int val;
            sscanf(s.substr(i+1, 2).c_str(), "%x", &val);
            result += (char)val;
            i += 2;
        } else result += s[i];
    }
    return result;
}

string formatMoney(int amount)
{
    string s = to_string(amount);
    int n = s.length();
    string result;
    for (int i = 0; i < n; i++) {
        if (i > 0 && (n - i) % 3 == 0) result += ',';
        result += s[i];
    }
    return result;
}

/* ========== FILE I/O ========== */

string readHTML(const string& filename)
{
    ifstream file(filename);
    if (!file.is_open()) return "<h3>Error: Could not load page</h3>";
    string line, html;
    while (getline(file, line))
        html += line + "\n";
    return html;
}

vector<Account> loadAccounts()
{
    vector<Account> accounts;
    ifstream file("accounts.txt");
    Account a;
    while (file >> a.accNo >> a.name >> a.pin >> a.balance)
        accounts.push_back(a);
    return accounts;
}

void saveAccounts(const vector<Account>& accounts)
{
    ofstream file("accounts.txt");
    for (const auto& a : accounts)
        file << a.accNo << " " << a.name << " " << a.pin << " " << a.balance << endl;
}

void logTransaction(const Transaction& t)
{
    ofstream file("transactions.txt", ios::app);
    file << t.accNo << "|" << t.type << "|" << t.amount << "|"
         << t.balanceAfter << "|" << t.timestamp << "|" << t.detail << endl;
}

vector<Transaction> getTransactions(const string& accNo, int limit = 10)
{
    vector<Transaction> all;
    ifstream file("transactions.txt");
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        Transaction t;
        string tmp;
        getline(ss, t.accNo, '|');
        getline(ss, t.type, '|');
        getline(ss, tmp, '|'); t.amount = atoi(tmp.c_str());
        getline(ss, tmp, '|'); t.balanceAfter = atoi(tmp.c_str());
        getline(ss, t.timestamp, '|');
        getline(ss, t.detail, '|');
        if (t.accNo == accNo) all.push_back(t);
    }
    // return last N
    if ((int)all.size() > limit)
        all.erase(all.begin(), all.end() - limit);
    return all;
}

Account* findAccount(vector<Account>& accounts, const string& accNo)
{
    for (auto& a : accounts)
        if (a.accNo == accNo) return &a;
    return nullptr;
}

/* ========== CSS THEME (shared across pages) ========== */

string getCSS()
{
    return R"(
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Inter', sans-serif;
            min-height: 100vh;
            background: linear-gradient(135deg, #0a0e27 0%, #1a1a3e 40%, #2d1b69 100%);
            color: #fff;
        }
        .bg-effects { position: fixed; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 0; }
        .orb { position: absolute; border-radius: 50%; filter: blur(80px); opacity: 0.25; animation: float 8s ease-in-out infinite; }
        .orb:nth-child(1) { width: 400px; height: 400px; background: #6c63ff; top: -100px; left: -100px; }
        .orb:nth-child(2) { width: 300px; height: 300px; background: #00d2ff; bottom: -80px; right: -80px; animation-delay: 2s; }
        .orb:nth-child(3) { width: 200px; height: 200px; background: #ff6b6b; top: 50%; left: 60%; animation-delay: 4s; }
        @keyframes float { 0%,100%{transform:translateY(0) scale(1)} 50%{transform:translateY(-30px) scale(1.05)} }

        .container { position: relative; z-index: 1; max-width: 900px; margin: 0 auto; padding: 30px 20px; }
        .fade-in { animation: fadeIn 0.6s ease forwards; opacity: 0; }
        @keyframes fadeIn { to { opacity: 1; } }

        /* Top bar */
        .topbar { display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; flex-wrap: wrap; gap: 12px; }
        .topbar h1 { font-size: 22px; font-weight: 700; }
        .topbar h1 span { color: #6c63ff; }
        .topbar-right { display: flex; align-items: center; gap: 12px; }
        .user-badge { background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.1); border-radius: 12px; padding: 8px 16px; font-size: 13px; }
        .logout-btn { background: rgba(255,75,75,0.15); border: 1px solid rgba(255,75,75,0.3); color: #ff6b6b; padding: 8px 18px; border-radius: 12px; cursor: pointer; font-size: 13px; font-family: 'Inter',sans-serif; transition: all 0.3s; }
        .logout-btn:hover { background: rgba(255,75,75,0.3); }

        /* Balance card */
        .balance-card { background: linear-gradient(135deg, #6c63ff, #4834d4, #00d2ff); border-radius: 24px; padding: 36px; margin-bottom: 28px; position: relative; overflow: hidden; }
        .balance-card::before { content: ''; position: absolute; top: -50%; right: -20%; width: 300px; height: 300px; background: rgba(255,255,255,0.06); border-radius: 50%; }
        .balance-card .label { font-size: 13px; opacity: 0.8; margin-bottom: 4px; }
        .balance-card .amount { font-size: 42px; font-weight: 700; letter-spacing: -1px; }
        .balance-card .acc-info { margin-top: 12px; font-size: 13px; opacity: 0.7; }

        /* Action grid */
        .action-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 14px; margin-bottom: 28px; }
        .action-card { background: rgba(255,255,255,0.05); backdrop-filter: blur(20px); border: 1px solid rgba(255,255,255,0.08); border-radius: 18px; padding: 22px 16px; text-align: center; cursor: pointer; transition: all 0.3s; text-decoration: none; color: #fff; }
        .action-card:hover { background: rgba(108,99,255,0.15); border-color: rgba(108,99,255,0.3); transform: translateY(-3px); }
        .action-card .icon { font-size: 28px; margin-bottom: 8px; }
        .action-card .name { font-size: 13px; font-weight: 500; }

        /* Glass card */
        .glass { background: rgba(255,255,255,0.05); backdrop-filter: blur(20px); border: 1px solid rgba(255,255,255,0.08); border-radius: 20px; padding: 28px; margin-bottom: 20px; }
        .glass h3 { font-size: 18px; margin-bottom: 20px; font-weight: 600; }

        /* Forms */
        .form-group { margin-bottom: 16px; }
        .form-group label { display: block; font-size: 13px; color: rgba(255,255,255,0.6); margin-bottom: 6px; }
        .form-group input, .form-group select {
            width: 100%; padding: 13px 16px;
            background: rgba(255,255,255,0.07); border: 1px solid rgba(255,255,255,0.1);
            border-radius: 14px; font-size: 15px; color: #fff;
            font-family: 'Inter',sans-serif; outline: none; transition: all 0.3s;
        }
        .form-group input:focus, .form-group select:focus { border-color: #6c63ff; box-shadow: 0 0 0 3px rgba(108,99,255,0.15); }
        .form-group input::placeholder { color: rgba(255,255,255,0.3); }
        .form-group select option { background: #1a1a3e; }

        .btn { padding: 13px 28px; border-radius: 14px; border: none; font-size: 15px; font-weight: 600; font-family: 'Inter',sans-serif; cursor: pointer; transition: all 0.3s; }
        .btn-primary { background: linear-gradient(135deg, #6c63ff, #4834d4); color: #fff; }
        .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 8px 25px rgba(108,99,255,0.4); }
        .btn-success { background: linear-gradient(135deg, #00b894, #00cec9); color: #fff; }
        .btn-success:hover { transform: translateY(-2px); box-shadow: 0 8px 25px rgba(0,184,148,0.4); }
        .btn-danger { background: linear-gradient(135deg, #ff6b6b, #ee5a24); color: #fff; }
        .btn-danger:hover { transform: translateY(-2px); box-shadow: 0 8px 25px rgba(255,107,107,0.4); }
        .btn-secondary { background: rgba(255,255,255,0.1); color: #fff; border: 1px solid rgba(255,255,255,0.15); }
        .btn-secondary:hover { background: rgba(255,255,255,0.15); }

        .btn-row { display: flex; gap: 12px; flex-wrap: wrap; }

        /* Transaction table */
        .tx-table { width: 100%; border-collapse: collapse; }
        .tx-table th { text-align: left; font-size: 12px; color: rgba(255,255,255,0.4); text-transform: uppercase; letter-spacing: 0.5px; padding: 10px 12px; border-bottom: 1px solid rgba(255,255,255,0.08); }
        .tx-table td { padding: 12px; font-size: 14px; border-bottom: 1px solid rgba(255,255,255,0.04); }
        .tx-type { display: inline-block; padding: 3px 10px; border-radius: 8px; font-size: 12px; font-weight: 600; }
        .tx-deposit { background: rgba(0,184,148,0.15); color: #55efc4; }
        .tx-withdraw { background: rgba(255,107,107,0.15); color: #ff6b6b; }
        .tx-transfer-in { background: rgba(0,210,255,0.15); color: #74b9ff; }
        .tx-transfer-out { background: rgba(253,203,110,0.15); color: #ffeaa7; }

        /* Alerts */
        .alert { padding: 14px 20px; border-radius: 14px; margin-bottom: 20px; font-size: 14px; }
        .alert-success { background: rgba(0,184,148,0.15); border: 1px solid rgba(0,184,148,0.3); color: #55efc4; }
        .alert-error { background: rgba(255,107,107,0.15); border: 1px solid rgba(255,107,107,0.3); color: #ff6b6b; }
        .alert-info { background: rgba(108,99,255,0.15); border: 1px solid rgba(108,99,255,0.3); color: #a29bfe; }

        /* Responsive */
        @media (max-width: 600px) {
            .balance-card .amount { font-size: 32px; }
            .action-grid { grid-template-columns: repeat(3, 1fr); }
        }
    </style>
    )";
}

/* ========== PAGE BUILDERS ========== */

string bgEffects()
{
    return "<div class='bg-effects'><div class='orb'></div><div class='orb'></div><div class='orb'></div></div>";
}

string topBar(const string& name, const string& acc, const string& pin)
{
    stringstream ss;
    ss << "<div class='topbar'>"
       << "<h1>&#x1F3E6; Secure<span>Vault</span></h1>"
       << "<div class='topbar-right'>"
       << "<div class='user-badge'>&#x1F464; " << name << " &bull; Acc: " << acc << "</div>"
       << "<form method='POST' style='display:inline'>"
       << "<button class='logout-btn' type='submit'>&#x2716; Logout</button>"
       << "</form></div></div>";
    return ss.str();
}

string hiddenFields(const string& acc, const string& pin)
{
    return "<input type='hidden' name='acc' value='" + acc + "'>"
           "<input type='hidden' name='pin' value='" + pin + "'>";
}

string buildDashboard(const string& acc, const string& pin, const string& name,
                       int balance, const string& alertMsg = "", const string& alertType = "")
{
    stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       << "<title>SecureVault - Dashboard</title>" << getCSS() << "</head><body>"
       << bgEffects()
       << "<div class='container fade-in'>"
       << topBar(name, acc, pin);

    // Alert message
    if (!alertMsg.empty())
        ss << "<div class='alert alert-" << alertType << "'>" << alertMsg << "</div>";

    // Balance card
    ss << "<div class='balance-card'>"
       << "<div class='label'>Available Balance</div>"
       << "<div class='amount'>&#x20B9; " << formatMoney(balance) << "</div>"
       << "<div class='acc-info'>Account: " << acc << " &bull; " << name << " &bull; Savings Account</div>"
       << "</div>";

    // Action grid
    ss << "<div class='action-grid'>";

    // Each action is a small form
    string actions[][3] = {
        {"deposit_page", "&#x1F4B0;", "Deposit"},
        {"withdraw_page", "&#x1F4B8;", "Withdraw"},
        {"transfer_page", "&#x1F504;", "Transfer"},
        {"history_page", "&#x1F4CB;", "History"},
        {"changepin_page", "&#x1F511;", "Change PIN"},
        {"profile_page", "&#x1F464;", "Profile"}
    };

    for (int i = 0; i < 6; i++) {
        ss << "<form method='POST' style='margin:0'>"
           << hiddenFields(acc, pin)
           << "<input type='hidden' name='action' value='" << actions[i][0] << "'>"
           << "<button type='submit' class='action-card' style='border:1px solid rgba(255,255,255,0.08);width:100%'>"
           << "<div class='icon'>" << actions[i][1] << "</div>"
           << "<div class='name'>" << actions[i][2] << "</div>"
           << "</button></form>";
    }
    ss << "</div>";

    // Recent transactions
    vector<Transaction> txns = getTransactions(acc, 5);
    ss << "<div class='glass'><h3>&#x1F4CA; Recent Transactions</h3>";
    if (txns.empty()) {
        ss << "<p style='color:rgba(255,255,255,0.4);font-size:14px'>No transactions yet. Start by making a deposit!</p>";
    } else {
        ss << "<div style='overflow-x:auto'><table class='tx-table'>"
           << "<tr><th>Date</th><th>Type</th><th>Amount</th><th>Balance</th><th>Details</th></tr>";
        for (int i = txns.size()-1; i >= 0; i--) {
            string cls = "tx-deposit";
            if (txns[i].type == "WITHDRAW") cls = "tx-withdraw";
            else if (txns[i].type == "TRANSFER_IN") cls = "tx-transfer-in";
            else if (txns[i].type == "TRANSFER_OUT") cls = "tx-transfer-out";

            string sign = "+";
            if (txns[i].type == "WITHDRAW" || txns[i].type == "TRANSFER_OUT") sign = "-";

            ss << "<tr><td style='color:rgba(255,255,255,0.6)'>" << txns[i].timestamp << "</td>"
               << "<td><span class='tx-type " << cls << "'>" << txns[i].type << "</span></td>"
               << "<td>" << sign << " &#x20B9;" << formatMoney(txns[i].amount) << "</td>"
               << "<td>&#x20B9;" << formatMoney(txns[i].balanceAfter) << "</td>"
               << "<td style='color:rgba(255,255,255,0.5)'>" << txns[i].detail << "</td></tr>";
        }
        ss << "</table></div>";
    }
    ss << "</div>";

    ss << "<div style='text-align:center;color:rgba(255,255,255,0.2);font-size:12px;padding:20px 0'>SecureVault Banking &copy; 2026 &bull; Encrypted Connection</div>";
    ss << "</div></body></html>";
    return ss.str();
}

string buildActionPage(const string& acc, const string& pin, const string& name,
                        const string& actionType, const string& title, const string& icon,
                        const string& btnClass, bool showTargetAcc = false)
{
    stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       << "<title>SecureVault - " << title << "</title>" << getCSS() << "</head><body>"
       << bgEffects()
       << "<div class='container fade-in'>"
       << topBar(name, acc, pin)
       << "<div class='glass' style='max-width:480px;margin:40px auto'>"
       << "<h3>" << icon << " " << title << "</h3>"
       << "<form method='POST'>"
       << hiddenFields(acc, pin);

    if (showTargetAcc) {
        ss << "<div class='form-group'><label>Recipient Account Number</label>"
           << "<input type='text' name='target_acc' placeholder='Enter account number' required></div>";
    }

    ss << "<div class='form-group'><label>Amount (&#x20B9;)</label>"
       << "<input type='number' name='amount' placeholder='Enter amount' min='1' required></div>"
       << "<div class='btn-row'>"
       << "<button type='submit' name='action' value='" << actionType << "' class='btn " << btnClass << "'>" << title << "</button>"
       << "<button type='submit' name='action' value='dashboard' class='btn btn-secondary'>&#x2190; Back</button>"
       << "</div></form></div></div></body></html>";
    return ss.str();
}

string buildChangePinPage(const string& acc, const string& pin, const string& name,
                           const string& alertMsg = "", const string& alertType = "")
{
    stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       << "<title>SecureVault - Change PIN</title>" << getCSS() << "</head><body>"
       << bgEffects()
       << "<div class='container fade-in'>"
       << topBar(name, acc, pin)
       << "<div class='glass' style='max-width:480px;margin:40px auto'>"
       << "<h3>&#x1F511; Change PIN</h3>";

    if (!alertMsg.empty())
        ss << "<div class='alert alert-" << alertType << "'>" << alertMsg << "</div>";

    ss << "<form method='POST'>"
       << hiddenFields(acc, pin)
       << "<div class='form-group'><label>Current PIN</label>"
       << "<input type='password' name='old_pin' placeholder='Enter current PIN' required></div>"
       << "<div class='form-group'><label>New PIN</label>"
       << "<input type='password' name='new_pin' placeholder='Enter new PIN (4 digits)' required pattern='[0-9]{4}' maxlength='4'></div>"
       << "<div class='form-group'><label>Confirm New PIN</label>"
       << "<input type='password' name='confirm_pin' placeholder='Confirm new PIN' required pattern='[0-9]{4}' maxlength='4'></div>"
       << "<div class='btn-row'>"
       << "<button type='submit' name='action' value='change_pin' class='btn btn-primary'>Update PIN</button>"
       << "<button type='submit' name='action' value='dashboard' class='btn btn-secondary'>&#x2190; Back</button>"
       << "</div></form></div></div></body></html>";
    return ss.str();
}

string buildHistoryPage(const string& acc, const string& pin, const string& name)
{
    stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       << "<title>SecureVault - History</title>" << getCSS() << "</head><body>"
       << bgEffects()
       << "<div class='container fade-in'>"
       << topBar(name, acc, pin)
       << "<div class='glass'><h3>&#x1F4CB; Transaction History (Last 20)</h3>";

    vector<Transaction> txns = getTransactions(acc, 20);
    if (txns.empty()) {
        ss << "<p style='color:rgba(255,255,255,0.4)'>No transactions found.</p>";
    } else {
        ss << "<div style='overflow-x:auto'><table class='tx-table'>"
           << "<tr><th>#</th><th>Date &amp; Time</th><th>Type</th><th>Amount</th><th>Balance After</th><th>Details</th></tr>";
        int num = txns.size();
        for (int i = num-1; i >= 0; i--) {
            string cls = "tx-deposit";
            if (txns[i].type == "WITHDRAW") cls = "tx-withdraw";
            else if (txns[i].type == "TRANSFER_IN") cls = "tx-transfer-in";
            else if (txns[i].type == "TRANSFER_OUT") cls = "tx-transfer-out";

            string sign = "+";
            if (txns[i].type == "WITHDRAW" || txns[i].type == "TRANSFER_OUT") sign = "-";

            ss << "<tr><td>" << (num - i) << "</td>"
               << "<td style='color:rgba(255,255,255,0.6)'>" << txns[i].timestamp << "</td>"
               << "<td><span class='tx-type " << cls << "'>" << txns[i].type << "</span></td>"
               << "<td>" << sign << " &#x20B9;" << formatMoney(txns[i].amount) << "</td>"
               << "<td>&#x20B9;" << formatMoney(txns[i].balanceAfter) << "</td>"
               << "<td style='color:rgba(255,255,255,0.5)'>" << txns[i].detail << "</td></tr>";
        }
        ss << "</table></div>";
    }

    ss << "<br><form method='POST'>" << hiddenFields(acc, pin)
       << "<button type='submit' name='action' value='dashboard' class='btn btn-secondary'>&#x2190; Back to Dashboard</button>"
       << "</form></div></div></body></html>";
    return ss.str();
}

string buildProfilePage(const string& acc, const string& pin, const string& name, int balance)
{
    vector<Transaction> txns = getTransactions(acc, 100);
    int totalDeposits = 0, totalWithdrawals = 0, txCount = txns.size();
    for (auto& t : txns) {
        if (t.type == "DEPOSIT" || t.type == "TRANSFER_IN") totalDeposits += t.amount;
        if (t.type == "WITHDRAW" || t.type == "TRANSFER_OUT") totalWithdrawals += t.amount;
    }

    stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       << "<title>SecureVault - Profile</title>" << getCSS() << "</head><body>"
       << bgEffects()
       << "<div class='container fade-in'>"
       << topBar(name, acc, pin)
       << "<div class='glass' style='max-width:560px;margin:40px auto'>"
       << "<h3>&#x1F464; Account Profile</h3>"
       << "<div style='display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:20px'>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Account Holder</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px'>" << name << "</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Account Number</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px'>" << acc << "</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Current Balance</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px;color:#55efc4'>&#x20B9; " << formatMoney(balance) << "</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Account Type</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px'>Savings</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Total Deposits</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px;color:#55efc4'>&#x20B9; " << formatMoney(totalDeposits) << "</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Total Withdrawals</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px;color:#ff6b6b'>&#x20B9; " << formatMoney(totalWithdrawals) << "</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Transactions</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px'>" << txCount << "</div></div>"
       << "<div style='background:rgba(255,255,255,0.04);border-radius:14px;padding:20px'>"
       << "<div style='font-size:12px;color:rgba(255,255,255,0.4)'>Status</div>"
       << "<div style='font-size:18px;font-weight:600;margin-top:4px;color:#55efc4'>&#x2705; Active</div></div>"
       << "</div><br>"
       << "<form method='POST'>" << hiddenFields(acc, pin)
       << "<button type='submit' name='action' value='dashboard' class='btn btn-secondary'>&#x2190; Back to Dashboard</button>"
       << "</form></div></div></body></html>";
    return ss.str();
}

string buildLoginFailed()
{
    stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       << "<title>SecureVault - Login Failed</title>" << getCSS() << "</head><body>"
       << bgEffects()
       << "<div style='position:relative;z-index:1;display:flex;justify-content:center;align-items:center;min-height:100vh'>"
       << "<div class='glass fade-in' style='max-width:420px;text-align:center;padding:48px 40px'>"
       << "<div style='font-size:64px;margin-bottom:16px'>&#x1F6AB;</div>"
       << "<h3 style='color:#ff6b6b;margin-bottom:12px'>Login Failed</h3>"
       << "<p style='color:rgba(255,255,255,0.5);margin-bottom:28px'>Invalid account number or PIN. Please check your credentials and try again.</p>"
       << "<form method='GET' action='/'><button class='btn btn-primary' type='submit'>&#x2190; Try Again</button></form>"
       << "</div></div></body></html>";
    return ss.str();
}

/* ========== REQUEST HANDLER ========== */

string handleRequest(const string& method, const string& body)
{
    // Parse form data
    string acc, pin, action, amountStr, targetAcc, oldPin, newPin, confirmPin;
    stringstream ss(body);
    string token;
    while (getline(ss, token, '&')) {
        size_t eq = token.find('=');
        if (eq == string::npos) continue;
        string key = token.substr(0, eq);
        string val = urlDecode(token.substr(eq + 1));
        if (key == "acc") acc = val;
        else if (key == "pin") pin = val;
        else if (key == "action") action = val;
        else if (key == "amount") amountStr = val;
        else if (key == "target_acc") targetAcc = val;
        else if (key == "old_pin") oldPin = val;
        else if (key == "new_pin") newPin = val;
        else if (key == "confirm_pin") confirmPin = val;
    }

    // GET or no credentials -> login page
    if (method == "GET" || acc.empty())
        return readHTML("index.html");

    // Load accounts
    vector<Account> accounts = loadAccounts();
    Account* user = findAccount(accounts, acc);

    // Validate login
    if (!user || user->pin != pin)
        return buildLoginFailed();

    string name = user->name;
    int balance = user->balance;

    // Logout (empty action from topbar form)
    if (action.empty())
        return buildDashboard(acc, pin, name, balance);

    // Navigation pages
    if (action == "dashboard")
        return buildDashboard(acc, pin, name, balance);
    if (action == "deposit_page")
        return buildActionPage(acc, pin, name, "deposit", "Deposit Money", "&#x1F4B0;", "btn-success");
    if (action == "withdraw_page")
        return buildActionPage(acc, pin, name, "withdraw", "Withdraw Money", "&#x1F4B8;", "btn-danger");
    if (action == "transfer_page")
        return buildActionPage(acc, pin, name, "transfer", "Transfer Money", "&#x1F504;", "btn-primary", true);
    if (action == "history_page")
        return buildHistoryPage(acc, pin, name);
    if (action == "changepin_page")
        return buildChangePinPage(acc, pin, name);
    if (action == "profile_page")
        return buildProfilePage(acc, pin, name, balance);

    // ===== DEPOSIT =====
    if (action == "deposit") {
        if (amountStr.empty() || atoi(amountStr.c_str()) <= 0)
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Please enter a valid amount.", "error");

        int amount = atoi(amountStr.c_str());
        if (amount > 100000)
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Maximum deposit limit is &#x20B9;1,00,000 per transaction.", "error");

        user->balance += amount;
        saveAccounts(accounts);
        logTransaction({acc, "DEPOSIT", amount, user->balance, getTimestamp(), "Cash deposit"});
        return buildDashboard(acc, pin, name, user->balance,
            "&#x2705; Successfully deposited &#x20B9;" + formatMoney(amount) + ". New balance: &#x20B9;" + formatMoney(user->balance), "success");
    }

    // ===== WITHDRAW =====
    if (action == "withdraw") {
        if (amountStr.empty() || atoi(amountStr.c_str()) <= 0)
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Please enter a valid amount.", "error");

        int amount = atoi(amountStr.c_str());
        if (amount > balance)
            return buildDashboard(acc, pin, name, balance, "&#x274C; Insufficient balance! Your current balance is &#x20B9;" + formatMoney(balance), "error");
        if (amount > 50000)
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Maximum withdrawal limit is &#x20B9;50,000 per transaction.", "error");

        user->balance -= amount;
        saveAccounts(accounts);
        logTransaction({acc, "WITHDRAW", amount, user->balance, getTimestamp(), "Cash withdrawal"});
        return buildDashboard(acc, pin, name, user->balance,
            "&#x2705; Successfully withdrew &#x20B9;" + formatMoney(amount) + ". New balance: &#x20B9;" + formatMoney(user->balance), "success");
    }

    // ===== TRANSFER =====
    if (action == "transfer") {
        if (amountStr.empty() || atoi(amountStr.c_str()) <= 0)
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Please enter a valid amount.", "error");
        if (targetAcc.empty())
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Please enter a recipient account number.", "error");
        if (targetAcc == acc)
            return buildDashboard(acc, pin, name, balance, "&#x274C; Cannot transfer to your own account!", "error");

        Account* target = findAccount(accounts, targetAcc);
        if (!target)
            return buildDashboard(acc, pin, name, balance, "&#x274C; Recipient account " + targetAcc + " not found!", "error");

        int amount = atoi(amountStr.c_str());
        if (amount > balance)
            return buildDashboard(acc, pin, name, balance, "&#x274C; Insufficient balance for transfer!", "error");
        if (amount > 200000)
            return buildDashboard(acc, pin, name, balance, "&#x26A0; Maximum transfer limit is &#x20B9;2,00,000 per transaction.", "error");

        user->balance -= amount;
        target->balance += amount;
        saveAccounts(accounts);
        logTransaction({acc, "TRANSFER_OUT", amount, user->balance, getTimestamp(), "To " + targetAcc + " (" + target->name + ")"});
        logTransaction({targetAcc, "TRANSFER_IN", amount, target->balance, getTimestamp(), "From " + acc + " (" + name + ")"});
        return buildDashboard(acc, pin, name, user->balance,
            "&#x2705; Successfully transferred &#x20B9;" + formatMoney(amount) + " to " + target->name + " (" + targetAcc + ")", "success");
    }

    // ===== CHANGE PIN =====
    if (action == "change_pin") {
        if (oldPin != pin)
            return buildChangePinPage(acc, pin, name, "&#x274C; Current PIN is incorrect!", "error");
        if (newPin.length() != 4)
            return buildChangePinPage(acc, pin, name, "&#x26A0; New PIN must be exactly 4 digits.", "error");
        if (newPin != confirmPin)
            return buildChangePinPage(acc, pin, name, "&#x274C; New PINs do not match!", "error");
        if (newPin == pin)
            return buildChangePinPage(acc, pin, name, "&#x26A0; New PIN cannot be the same as current PIN.", "error");

        user->pin = newPin;
        saveAccounts(accounts);
        return buildDashboard(acc, newPin, name, user->balance,
            "&#x2705; PIN changed successfully! Use your new PIN for future logins.", "success");
    }

    return buildDashboard(acc, pin, name, balance);
}

/* ========== MAIN SERVER ========== */

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "ERROR: WSAStartup failed" << endl;
        return 1;
    }
#endif

    // Create transactions file if it doesn't exist
    {
        ofstream f("transactions.txt", ios::app);
        f.close();
    }

    /* Dynamic PORT for Render */
    int port = 8080;
    char* portEnv = getenv("PORT");
    if (portEnv)
        port = stoi(portEnv);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Allow port reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "ERROR: Could not bind to port " << port << endl;
        return 1;
    }

    listen(server_fd, 10);

    cout << "========================================" << endl;
    cout << "  SecureVault ATM Server" << endl;
    cout << "  Running on http://localhost:" << port << endl;
    cout << "========================================" << endl;

    while (true) {
        new_socket = accept(server_fd,
                            (struct sockaddr*)&address,
                            (socklen_t*)&addrlen);

        if (new_socket < 0) continue;

        char buffer[8192] = {0};
        recv(new_socket, buffer, sizeof(buffer) - 1, 0);

        string request(buffer);

        // Parse method
        string method = "GET";
        if (request.substr(0, 4) == "POST") method = "POST";

        // Parse body
        string body;
        size_t pos = request.find("\r\n\r\n");
        if (pos != string::npos)
            body = request.substr(pos + 4);

        // Handle request
        string responseHTML = handleRequest(method, body);

        // Build HTTP response
        string fullResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: " + to_string(responseHTML.length()) + "\r\n"
            "Connection: close\r\n"
            "Cache-Control: no-store\r\n"
            "\r\n" + responseHTML;

        send(new_socket, fullResponse.c_str(), fullResponse.length(), 0);
        CLOSESOCKET(new_socket);
    }

    return 0;
}
