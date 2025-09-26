#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

// ( HTML page)
std::string createHelloWorldPage() {
    std::string html =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Hello World - C++ Web Server Willia Sample Base</title>\n"
        "    <style>\n"
        "        body {\n"
        "            font-family: Arial, sans-serif;\n"
        "            text-align: center;\n"
        "            background-color: #f0f8ff;\n"
        "            margin: 0;\n"
        "            padding: 50px;\n"
        "        }\n"
        "        .container {\n"
        "            background: white;\n"
        "            padding: 50px;\n"
        "            border-radius: 10px;\n"
        "            box-shadow: 0 4px 8px rgba(0,0,0,0.1);\n"
        "            max-width: 600px;\n"
        "            margin: 0 auto;\n"
        "        }\n"
        "        h1 {\n"
        "            color: #2c3e50;\n"
        "            font-size: 3em;\n"
        "            margin-bottom: 20px;\n"
        "        }\n"
        "        p {\n"
        "            color: #7f8c8d;\n"
        "            font-size: 1.2em;\n"
        "            line-height: 1.6;\n"
        "        }\n"
        "        .success {\n"
        "            background: #d4edda;\n"
        "            color: #155724;\n"
        "            padding: 20px;\n"
        "            border-radius: 5px;\n"
        "            margin: 20px 0;\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>Hello World!</h1>\n"
        "        <div class=\"success\">\n"
        "            <strong>Congratulations!</strong> William <3\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";

    return html;
}

// HTTP requests
std::string handleRequest(const std::string& request) {
    
    std::string html = createHelloWorldPage();

   
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(html.length()) + "\r\n"
        "\r\n" + html;

    return response;
}


void startWebServer(int port) {
  
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cout << "Failed to create socket" << std::endl;
        return;
    }

  
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    struct sockaddr_in address;
    address.sin_family = AF_INET;           // IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // Accept connections from any IP
    address.sin_port = htons(port);         // Convert port to network byte order


    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cout << "Failed to bind to port " << port << std::endl;
        return;
    }


    if (listen(server_fd, 3) < 0) {
        std::cout << "Failed to listen on port " << port << std::endl;
        return;
    }

    std::cout << "==================================" << std::endl;
    std::cout << "Hello World Web Server Started!" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Server running on: http://localhost:" << port << std::endl;
    std::cout << "Open your web browser and visit the URL above!" << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    std::cout << "==================================" << std::endl;

 
    while (true) {
        
        int new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket < 0) {
            continue; 
        }

      
        char buffer[4096] = {0};
        read(new_socket, buffer, 4096);

        std::string request(buffer);
        std::cout << "Received request from browser!" << std::endl;

      
        std::string response = handleRequest(request);

     
        send(new_socket, response.c_str(), response.length(), 0);

  
        close(new_socket);

        std::cout << "Sent Hello World page to browser!" << std::endl;
    }
}

int main() {
    std::cout << "Starting Hello World Web Server..." << std::endl;
    std::cout << "This is a simple C++ web server for learning!" << std::endl;

   
    startWebServer(8080);

    return 0;
}
