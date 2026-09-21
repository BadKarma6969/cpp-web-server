#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

std::mutex log_mutex;
// --------------------------------------------------
// HTTP handling
// --------------------------------------------------

std::string getContentType(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg"))
        return "image/jpeg";

    return "text/plain";
}


void sendResponse(int client_fd, const std::string& response) {
    size_t total_sent = 0;

    while (total_sent < response.size()) {
        ssize_t sent = send(
            client_fd,
            response.c_str() + total_sent,
            response.size() - total_sent,
            0
        );

        if (sent <= 0)
            break;

        total_sent += sent;
    }
}


void handleClient(int client_fd) {
{
    std::lock_guard<std::mutex> lock(log_mutex);

    std::cout << "Handling request on thread: "
              << std::this_thread::get_id()
              << '\n';
}
    char buffer[4096] = {0};

    ssize_t bytes_received = recv(
        client_fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }

    std::string request(buffer);

{
    std::lock_guard<std::mutex> lock(log_mutex);

    std::cout << "Request received:\n";
    std::cout << request << '\n';
}

    // Parse request line
    std::istringstream request_stream(request);

    std::string method;
    std::string path;
    std::string version;

    request_stream >> method >> path >> version;


    // Only GET is supported
    if (method != "GET") {

        std::string body = "Method Not Allowed";

        std::string response =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        sendResponse(client_fd, response);

        close(client_fd);
        return;
    }


    // "/" → index.html
    if (path == "/")
        path = "/index.html";


    // Basic path traversal protection
    if (path.find("..") != std::string::npos) {

        std::string body = "Forbidden";

        std::string response =
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        sendResponse(client_fd, response);

        close(client_fd);
        return;
    }


    // Find requested file
    std::string file_path = "www" + path;

    std::ifstream file(file_path, std::ios::binary);


    // File doesn't exist
    if (!file) {

        std::string body = "<h1>404 Not Found</h1>";

        std::string response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        sendResponse(client_fd, response);

        close(client_fd);
        return;
    }


    // Read file
    std::ostringstream contents;

    contents << file.rdbuf();

    std::string body = contents.str();


    // Create response
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + getContentType(file_path) + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;


    sendResponse(client_fd, response);

    close(client_fd);
}


// --------------------------------------------------
// Thread Pool
// --------------------------------------------------

class ThreadPool {

private:

    std::vector<std::thread> workers;

    std::queue<int> tasks;

    std::mutex queue_mutex;

    std::condition_variable condition;

    bool stop = false;


public:

    ThreadPool(size_t thread_count) {

        for (size_t i = 0; i < thread_count; ++i) {

            workers.emplace_back([this]() {

                while (true) {

                    int client_fd;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);

                        condition.wait(
                            lock,
                            [this]() {
                                return stop || !tasks.empty();
                            }
                        );


                        if (stop && tasks.empty())
                            return;


                        client_fd = tasks.front();

                        tasks.pop();
                    }


                    handleClient(client_fd);
                }
            });
        }
    }


    void enqueue(int client_fd) {

        {
            std::lock_guard<std::mutex> lock(queue_mutex);

            tasks.push(client_fd);
        }

        condition.notify_one();
    }


    ~ThreadPool() {

        {
            std::lock_guard<std::mutex> lock(queue_mutex);

            stop = true;
        }


        condition.notify_all();


        for (std::thread& worker : workers) {

            if (worker.joinable())
                worker.join();
        }
    }
};


// --------------------------------------------------
// Main Server
// --------------------------------------------------

int main() {

    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );


    if (server_fd < 0) {

        std::cerr << "Socket creation failed\n";

        return 1;
    }


    // Allow quick restart after shutdown
    int opt = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );


    sockaddr_in address{};

    address.sin_family = AF_INET;

    address.sin_addr.s_addr = INADDR_ANY;

    address.sin_port = htons(8080);


    if (bind(
            server_fd,
            (struct sockaddr*)&address,
            sizeof(address)
        ) < 0) {

        std::cerr << "Bind failed\n";

        close(server_fd);

        return 1;
    }


    if (listen(server_fd, 10) < 0) {

        std::cerr << "Listen failed\n";

        close(server_fd);

        return 1;
    }


    std::cout << "Server listening on port 8080...\n";

    std::cout << "Thread pool started with 4 workers.\n";


    // Create thread pool
    ThreadPool pool(4);


    // Continuously accept clients
    while (true) {

        int client_fd = accept(
            server_fd,
            nullptr,
            nullptr
        );


        if (client_fd < 0) {

            std::cerr << "Accept failed\n";

            continue;
        }


        // Give connection to thread pool
        pool.enqueue(client_fd);
    }


    close(server_fd);

    return 0;
}
