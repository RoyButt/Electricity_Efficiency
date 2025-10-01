#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sqlite3.h>

// Global DB handle
sqlite3* db = nullptr;

//  Small helpers functions -
std::string urlDecode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '+') {
            out += ' ';
        } else if (in[i] == '%' && i + 2 < in.size()) {
            std::string hex = in.substr(i + 1, 2);
            char ch = static_cast<char>(strtol(hex.c_str(), nullptr, 16));
            out += ch;
            i += 2;
        } else {
            out += in[i];
        }
    }
    return out;
}

std::string getParam(const std::string& data, const std::string& key) {
    std::string token = key + "=";
    size_t pos = data.find(token);
    if (pos == std::string::npos) return "";
    size_t start = pos + token.size();
    size_t end = data.find('&', start);
    if (end == std::string::npos) end = data.size();
    return urlDecode(data.substr(start, end - start));
}

int getContentLength(const std::string& headers) {
    const char* keys[] = {"Content-Length:", "content-length:", "CONTENT-LENGTH:"};
    for (const char* key : keys) {
        size_t p = headers.find(key);
        if (p != std::string::npos) {
            p += std::string(key).size();
            while (p < headers.size() && (headers[p] == ' ' || headers[p] == '\t')) p++;
            size_t e = headers.find("\r\n", p);
            std::string num = headers.substr(p, (e == std::string::npos ? headers.size() : e) - p);
            try { return std::stoi(num); } catch (...) { return 0; }
        }
    }
    return 0;
}

std::string makeHttpResponse(const std::string& body) {
    return std::string("HTTP/1.1 200 OK\r\n") +
           "Content-Type: text/html; charset=UTF-8\r\n" +
           "Content-Length: " + std::to_string(body.size()) + "\r\n" +
           "Connection: close\r\n\r\n" +
           body;
}

// - SQLite - Section
bool setupDB() {
    if (sqlite3_open("students.db", &db) != SQLITE_OK) return false;
    const char* sql =
        "CREATE TABLE IF NOT EXISTS students (\n"
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "  name TEXT NOT NULL,\n"
        "  age INTEGER,\n"
        "  grade TEXT\n"
        ");";
    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

void addStudent(const std::string& name, int age, const std::string& grade) {
    std::string sql = "INSERT INTO students (name, age, grade) VALUES ('" + name + "', " + std::to_string(age) + ", '" + grade + "');";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

void updateStudent(int id, const std::string& name, int age, const std::string& grade) {
    std::string sql = "UPDATE students SET name='" + name + "', age=" + std::to_string(age) + ", grade='" + grade + "' WHERE id=" + std::to_string(id) + ";";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

void deleteStudent(int id) {
    std::string sql = "DELETE FROM students WHERE id=" + std::to_string(id) + ";";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

std::string getStudentsRowsHTML() {
    std::string html;
    const char* sql = "SELECT id, name, age, grade FROM students ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string name = (const char*)sqlite3_column_text(stmt, 1);
            int age = sqlite3_column_int(stmt, 2);
            std::string grade = (const char*)sqlite3_column_text(stmt, 3);

            html += "<tr>";
            html += "<td>" + std::to_string(id) + "</td>";
            html += "<td>" + name + "</td>";
            html += "<td>" + std::to_string(age) + "</td>";
            html += "<td>" + grade + "</td>";
            html += "<td>";
            // Inline edit form (POST /update)
            html += "<form method='POST' action='/update' style='display:inline;'>";
            html += "<input type='hidden' name='id' value='" + std::to_string(id) + "'>";
            html += "<input name='name' value='" + name + "' required style='width:120px;'>";
            html += "<input name='age' value='" + std::to_string(age) + "' type='number' min='1' max='150' required style='width:60px;'>";
            html += "<input name='grade' value='" + grade + "' required style='width:60px;'>";
            html += "<button type='submit'>Update</button>";
            html += "</form> ";
            html += "<a href='/delete?id=" + std::to_string(id) + "' style='color:#c00; margin-left:8px;'>Delete</a>";
            html += "</td>";
            html += "</tr>";
        }
    }
    sqlite3_finalize(stmt);
    return html;
}

// HTML Page 
std::string createPage() {
    std::string rows = getStudentsRowsHTML();
    std::string html =
        "<!DOCTYPE html>\n"
        "<html><head><meta charset='utf-8'><title>Hello World CRUD</title>\n"
        "<style>body{font-family:Arial;background:#f0f8ff;margin:0;padding:40px;text-align:center}"
        ".box{background:#fff;max-width:900px;margin:0 auto;padding:24px;border-radius:10px;box-shadow:0 4px 12px rgba(0,0,0,.1)}"
        "h1{margin:0 0 12px;color:#2c3e50}"
        ".ok{background:#d4edda;color:#155724;padding:10px;border-radius:6px;margin:12px 0}"
        "form input,form select{padding:8px;margin:4px}table{width:100%;border-collapse:collapse;margin-top:16px}"
        "th,td{border:1px solid #ddd;padding:8px;text-align:left}th{background:#f8f8f8}"\
        "</style></head><body>\n"
        "<div class='box'>\n"
        "<h1>Hello World! Student CRUD</h1>\n"
        "<div class='ok'><b>Congratulations!</b> William <3</div>\n"
        "<p>Add students using the form. Edit inline and click Update. Use Delete to remove.</p>\n"
        "<hr><h3>Add Student</h3>\n"
        "<form method='POST' action='/add'>\n"
        "<input name='name' placeholder='Name' required>\n"
        "<input name='age' type='number' min='1' max='150' placeholder='Age' required>\n"
        "<select name='grade' required><option value=''>Grade</option><option>A+</option><option>A</option><option>A-</option><option>B+</option><option>B</option><option>B-</option><option>C+</option><option>C</option></select>\n"
        "<button type='submit'>Add</button>\n"
        "</form>\n"
        "<h3>Students</h3>\n"
        "<table><tr><th>ID</th><th>Name</th><th>Age</th><th>Grade</th><th>Actions</th></tr>" + rows + "</table>\n"
        "</div></body></html>";
    return html;
}

//Request handling + routing paths (Database requests)
std::string handleRequest(const std::string& req) {
    // Parse request line
    size_t sp1 = req.find(' ');
    size_t sp2 = (sp1 == std::string::npos) ? std::string::npos : req.find(' ', sp1 + 1);
    std::string method = (sp1 == std::string::npos) ? "" : req.substr(0, sp1);
    std::string path   = (sp1 == std::string::npos || sp2 == std::string::npos) ? "/" : req.substr(sp1 + 1, sp2 - sp1 - 1);

    // Body
    size_t bodyPos = req.find("\r\n\r\n");
    std::string body = (bodyPos == std::string::npos) ? "" : req.substr(bodyPos + 4);

    // Routes
    if (method == "POST" && path == "/add") {
        std::string name  = getParam(body, "name");
        std::string ageS  = getParam(body, "age");
        std::string grade = getParam(body, "grade");
        if (!name.empty() && !ageS.empty() && !grade.empty()) {
            addStudent(name, std::atoi(ageS.c_str()), grade);
        }
    } else if (method == "POST" && path == "/update") {
        std::string idS   = getParam(body, "id");
        std::string name  = getParam(body, "name");
        std::string ageS  = getParam(body, "age");
        std::string grade = getParam(body, "grade");
        if (!idS.empty() && !name.empty() && !ageS.empty() && !grade.empty()) {
            updateStudent(std::atoi(idS.c_str()), name, std::atoi(ageS.c_str()), grade);
        }
    } else if (method == "GET" && path.rfind("/delete", 0) == 0) { // starts with /delete
        size_t q = path.find('?');
        std::string query = (q == std::string::npos) ? "" : path.substr(q + 1);
        std::string idS = getParam(query, "id");
        if (!idS.empty()) deleteStudent(std::atoi(idS.c_str()));
    }

    return makeHttpResponse(createPage());
}

//web server 
void startWebServer(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd <= 0) { std::cout << "Failed to create socket" << std::endl; return; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "Failed to bind to port " << port << std::endl; return;
    }
    if (listen(server_fd, 3) < 0) { std::cout << "Failed to listen" << std::endl; return; }

    std::cout << "==================================\nHello World CRUD Server Started!\n==================================\n"
              << "URL: http://localhost:" << port << std::endl;

    const size_t MAX_REQ = 1 << 20; // 1MB safety

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) continue;

        // Read full HTTP request (headers + body)
        std::string req; req.reserve(16384);
        char buf[4096]; ssize_t n;
        size_t headerEnd = std::string::npos; int contentLen = 0;

        while ((n = recv(client, buf, sizeof(buf), 0)) > 0) {
            req.append(buf, buf + n);
            if (headerEnd == std::string::npos) {
                headerEnd = req.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    std::string headers = req.substr(0, headerEnd);
                    contentLen = getContentLength(headers);
                }
            }
            if (headerEnd != std::string::npos) {
                size_t totalNeeded = headerEnd + 4 + static_cast<size_t>(contentLen);
                if (req.size() >= totalNeeded) break; // got full request
            }
            if (req.size() > MAX_REQ) break; // prevent huge requests
        }

        std::string resp = handleRequest(req);
        send(client, resp.c_str(), resp.size(), 0);
        close(client);
    }
}

int main() {
    std::cout << "Starting Hello World CRUD Web Server..." << std::endl;
    if (!setupDB()) { std::cout << "Failed to setup SQLite DB" << std::endl; return 1; }
    startWebServer(8080);
    if (db) sqlite3_close(db);
    return 0;
}
