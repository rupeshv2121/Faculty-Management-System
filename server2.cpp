#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Linux/POSIX headers
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <signal.h>

using namespace std;

// Port from environment variable or default to 8080
int PORT = 8080;

struct SessionInfo
{
    string role;
    string userId;
};

map<string, SessionInfo> sessions;
mutex sessionsMutex;
mutex fileMutex;

struct Faculty
{
    string id, name, dept, desig, mobile, email, spec, pass, role;
};

struct HttpRequest
{
    string method;
    string path;
    map<string, string> headers;
    string body;
};

string urlDecode(const string &s);
vector<Faculty> loadData();

string trim(const string &s)
{
    size_t start = 0;
    while (start < s.size() && isspace(static_cast<unsigned char>(s[start])))
        ++start;

    size_t end = s.size();
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    return s.substr(start, end - start);
}

string toLower(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
              { return static_cast<char>(tolower(c)); });
    return s;
}

bool sendAll(int sock, const string &data)
{
    size_t sentTotal = 0;
    while (sentTotal < data.size())
    {
        ssize_t sent = send(sock, data.c_str() + sentTotal, data.size() - sentTotal, 0);
        if (sent <= 0)
            return false;
        sentTotal += static_cast<size_t>(sent);
    }
    return true;
}

string reasonPhrase(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 409:
        return "Conflict";
    case 500:
        return "Internal Server Error";
    default:
        return "Unknown";
    }
}

string httpResponse(int status, const string &contentType, const string &body)
{
    ostringstream oss;
    oss << "HTTP/1.1 " << status << " " << reasonPhrase(status) << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    return oss.str();
}

string parseRequestPath(const string &requestLine)
{
    istringstream iss(requestLine);
    string method, path, version;
    iss >> method >> path >> version;
    size_t qpos = path.find('?');
    return (qpos != string::npos) ? path.substr(0, qpos) : path;
}

string getQueryParam(const string &requestLine, const string &param)
{
    size_t qpos = requestLine.find('?');
    if (qpos == string::npos)
        return "";
    string query = requestLine.substr(qpos + 1);
    string search = param + "=";
    size_t pos = query.find(search);
    if (pos == string::npos)
        return "";
    pos += search.length();
    size_t end = query.find('&', pos);
    if (end == string::npos)
        end = query.length();
    string value = query.substr(pos, end - pos);
    return urlDecode(value);
}

string urlDecode(const string &s)
{
    string result;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '%' && i + 2 < s.size())
        {
            string hex = s.substr(i + 1, 2);
            char c = static_cast<char>(stoi(hex, nullptr, 16));
            result += c;
            i += 2;
        }
        else if (s[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += s[i];
        }
    }
    return result;
}

vector<Faculty> loadData()
{
    vector<Faculty> faculty;
    string line;
    ifstream file("users.txt", ios::in);
    if (!file.is_open())
    {
        cerr << "Warning: Could not open users.txt" << endl;
        return faculty;
    }

    while (getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        Faculty f;
        istringstream iss(line);
        if (iss >> f.id >> f.name >> f.dept >> f.desig >> f.mobile >> f.email >> f.spec >> f.pass >> f.role)
        {
            faculty.push_back(f);
        }
    }
    file.close();
    return faculty;
}

string jsonEscape(const string &s)
{
    string result;
    for (char c : s)
    {
        if (c == '"')
            result += "\\\"";
        else if (c == '\\')
            result += "\\\\";
        else if (c == '\n')
            result += "\\n";
        else if (c == '\r')
            result += "\\r";
        else
            result += c;
    }
    return result;
}

string handleListFaculty(const string &search = "")
{
    auto faculty = loadData();
    string lower_search = toLower(search);

    string json = R"({"status":"success","faculty":[)";
    bool first = true;

    for (const auto &f : faculty)
    {
        string combinedLower = toLower(f.name + " " + f.dept + " " + f.spec);
        if (search.empty() || combinedLower.find(lower_search) != string::npos)
        {
            if (!first)
                json += ",";
            json += R"({"id":")" + jsonEscape(f.id) + R"(","name":")" + jsonEscape(f.name);
            json += R"(","department":")" + jsonEscape(f.dept) + R"(","designation":")" + jsonEscape(f.desig);
            json += R"(","mobile":")" + jsonEscape(f.mobile) + R"(","email":")" + jsonEscape(f.email);
            json += R"(","subject":")" + jsonEscape(f.spec) + R"("})";
            first = false;
        }
    }

    json += R"(],"total":)" + to_string(first ? 0 : 1) + "}";
    return httpResponse(200, "application/json", json);
}

void handleRequest(int clientSocket, const string &requestLine, const string &body)
{
    string path = parseRequestPath(requestLine);

    if (path == "/" || path == "/index.html")
    {
        ifstream file("public/index.html", ios::binary);
        if (file)
        {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            sendAll(clientSocket, httpResponse(200, "text/html", content));
        }
        else
        {
            sendAll(clientSocket, httpResponse(404, "text/plain", "Not Found"));
        }
    }
    else if (path == "/api/faculty/list")
    {
        string search = getQueryParam(requestLine, "search");
        sendAll(clientSocket, handleListFaculty(search));
    }
    else if (path.find("/public/") == 0)
    {
        string filePath = "." + path;
        ifstream file(filePath, ios::binary);
        if (file)
        {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            string contentType = path.find(".css") != string::npos ? "text/css" : path.find(".js") != string::npos ? "application/javascript"
                                                                              : path.find(".png") != string::npos  ? "image/png"
                                                                                                                   : "text/html";
            sendAll(clientSocket, httpResponse(200, contentType, content));
        }
    }
    else
    {
        sendAll(clientSocket, httpResponse(404, "application/json", R"({"error":"Not Found"})"));
    }
}

void handleClient(int clientSocket)
{
    char buffer[4096];
    string allData;

    while (true)
    {
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0)
            break;

        buffer[bytesReceived] = '\0';
        allData += buffer;

        size_t headerEnd = allData.find("\r\n\r\n");
        if (headerEnd != string::npos)
        {
            size_t firstNewline = allData.find("\r\n");
            string requestLine = allData.substr(0, firstNewline);
            string body = allData.substr(headerEnd + 4);

            handleRequest(clientSocket, requestLine, body);
            break;
        }
    }

    close(clientSocket);
}

int main()
{
    // Get port from environment variable
    const char *portEnv = getenv("PORT");
    if (portEnv)
    {
        PORT = stoi(portEnv);
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cerr << "Error setting socket options" << endl;
        close(serverSocket);
        return 1;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "Error binding socket" << endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0)
    {
        cerr << "Error listening on socket" << endl;
        close(serverSocket);
        return 1;
    }

    cout << "Server running on port " << PORT << endl;

    while (true)
    {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket < 0)
        {
            cerr << "Error accepting client" << endl;
            continue;
        }

        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
