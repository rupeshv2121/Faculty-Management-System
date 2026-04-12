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

        // Parse comma-separated values: id,name,dept,desig,mobile,email,spec,pass,role
        Faculty f;
        vector<string> fields;
        stringstream ss(line);
        string field;
        
        while (getline(ss, field, ','))
        {
            field = trim(field);
            fields.push_back(field);
        }
        
        if (fields.size() >= 9)
        {
            f.id = fields[0];
            f.name = fields[1];
            f.dept = fields[2];
            f.desig = fields[3];
            f.mobile = fields[4];
            f.email = fields[5];
            f.spec = fields[6];
            f.pass = fields[7];
            f.role = fields[8];
            faculty.push_back(f);
            cout << "LOADED USER: id=" << f.id << " pass=" << f.pass << " role=" << f.role << endl;
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

// Simple JSON parser for login credentials
map<string, string> parseJSON(const string &json)
{
    map<string, string> result;
    cout << "PARSING JSON: " << json << endl;

    size_t pos = 0;
    while ((pos = json.find("\"", pos)) != string::npos)
    {
        size_t keyStart = pos + 1;
        size_t keyEnd = json.find("\"", keyStart);
        if (keyEnd == string::npos)
            break;

        string key = json.substr(keyStart, keyEnd - keyStart);

        size_t colonPos = json.find(":", keyEnd);
        if (colonPos == string::npos)
            break;

        size_t valueStart = json.find("\"", colonPos);
        if (valueStart == string::npos)
            break;
        valueStart++;

        size_t valueEnd = json.find("\"", valueStart);
        if (valueEnd == string::npos)
            break;

        string value = json.substr(valueStart, valueEnd - valueStart);
        result[key] = value;
        cout << "KEY: " << key << " => VALUE: " << value << endl;

        pos = valueEnd + 1;
    }
    return result;
}

string handleLogin(const string &body)
{
    cout << "LOGIN REQUEST BODY: " << body << endl;

    auto data = parseJSON(body);
    string username = data["username"];
    string password = data["password"];

    cout << "LOGIN ATTEMPT: username=" << username << " password=" << password << endl;

    auto faculty = loadData();
    for (const auto &f : faculty)
    {
        cout << "CHECKING: " << f.id << " / " << f.pass << endl;
        if (f.id == username && f.pass == password)
        {
            cout << "LOGIN SUCCESS for " << username << endl;
            string sessionId = "session_" + to_string(rand() % 100000);
            sessions[sessionId] = {f.role, f.id};

            string response = R"({"status":"success","sessionId":")" + sessionId +
                              R"(","userId":")" + jsonEscape(f.id) +
                              R"(","name":")" + jsonEscape(f.name) +
                              R"(","role":")" + jsonEscape(f.role) + R"("})";
            return httpResponse(200, "application/json", response);
        }
    }

    cout << "LOGIN FAILED" << endl;
    return httpResponse(401, "application/json", R"({"status":"error","message":"Invalid credentials"})");
}

string handleAuthMe(const string &requestLine)
{
    // Extract session from cookie or header
    size_t cookiePos = requestLine.find("Cookie:");
    if (cookiePos == string::npos)
    {
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }

    string cookie = requestLine.substr(cookiePos + 7);
    size_t sessionStart = cookie.find("sessionId=");
    if (sessionStart == string::npos)
    {
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }

    sessionStart += 10;
    size_t sessionEnd = cookie.find(";", sessionStart);
    if (sessionEnd == string::npos)
        sessionEnd = cookie.find("\r", sessionStart);

    string sessionId = cookie.substr(sessionStart, sessionEnd - sessionStart);

    if (sessions.find(sessionId) != sessions.end())
    {
        auto sess = sessions[sessionId];
        string response = R"({"status":"success","userId":")" + jsonEscape(sess.userId) +
                          R"(","role":")" + jsonEscape(sess.role) + R"("})";
        return httpResponse(200, "application/json", response);
    }

    return httpResponse(401, "application/json", R"({"status":"error","message":"Session expired"})");
}

void handleRequest(int clientSocket, const string &requestLine, const string &body)
{
    string path = parseRequestPath(requestLine);

    // Debug: Log all requests
    cout << "REQUEST: " << path << endl;

    // API endpoints
    if (path == "/api/auth/login")
    {
        sendAll(clientSocket, handleLogin(body));
    }
    else if (path == "/api/auth/me")
    {
        sendAll(clientSocket, handleAuthMe(requestLine));
    }
    else if (path == "/api/faculty/list")
    {
        string search = getQueryParam(requestLine, "search");
        sendAll(clientSocket, handleListFaculty(search));
    }
    // Handle root or any HTML file request
    else if (path == "/")
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
    // Handle .html files
    else if (path.find(".html") != string::npos)
    {
        string filePath = "public" + path;
        ifstream file(filePath, ios::binary);
        if (file)
        {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            sendAll(clientSocket, httpResponse(200, "text/html", content));
        }
        else
        {
            sendAll(clientSocket, httpResponse(404, "application/json", R"({"error":"File not found"})"));
        }
    }
    // Handle static files (css, js, images, etc)
    else if (path.find(".css") != string::npos || path.find(".js") != string::npos ||
             path.find(".png") != string::npos || path.find(".jpg") != string::npos ||
             path.find(".jpeg") != string::npos || path.find(".ico") != string::npos ||
             path.find(".txt") != string::npos)
    {
        // Files at root level: ./css/, ./js/, ./assets/, ./components/
        string filePath;
        if (path.find("/css/") == 0 || path.find("/js/") == 0 || path.find("/assets/") == 0 || path.find("/components/") == 0)
        {
            filePath = "." + path;
        }
        else
        {
            filePath = "public" + path;
        }

        cout << "LOADING FILE: " << filePath << endl;
        ifstream file(filePath, ios::binary);
        if (file)
        {
            cout << "FILE FOUND" << endl;
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            string contentType = path.find(".css") != string::npos ? "text/css" : path.find(".js") != string::npos                                      ? "application/javascript"
                                                                              : path.find(".png") != string::npos                                       ? "image/png"
                                                                              : path.find(".ico") != string::npos                                       ? "image/x-icon"
                                                                              : path.find(".jpg") != string::npos || path.find(".jpeg") != string::npos ? "image/jpeg"
                                                                                                                                                        : "text/plain";
            sendAll(clientSocket, httpResponse(200, contentType, content));
        }
        else
        {
            cout << "FILE NOT FOUND: " << filePath << endl;
            sendAll(clientSocket, httpResponse(404, "application/json", R"({"error":"File not found"})"));
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
    int contentLength = 0;

    // Read all data from socket
    while (true)
    {
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0)
            break;

        buffer[bytesReceived] = '\0';
        allData += buffer;

        // Find headers end
        size_t headerEnd = allData.find("\r\n\r\n");
        if (headerEnd != string::npos)
        {
            // Extract Content-Length from headers
            size_t clPos = allData.find("Content-Length:");
            if (clPos != string::npos && clPos < headerEnd)
            {
                size_t clStart = allData.find_first_not_of(" \t", clPos + 15);
                size_t clEnd = allData.find("\r\n", clStart);
                string clStr = allData.substr(clStart, clEnd - clStart);
                contentLength = stoi(clStr);
                cout << "CONTENT-LENGTH: " << contentLength << endl;
            }

            // Check if we have received all the body
            size_t bodyStart = headerEnd + 4;
            size_t bodyReceived = allData.size() - bodyStart;

            cout << "BODY RECEIVED: " << bodyReceived << " bytes, EXPECTED: " << contentLength << " bytes" << endl;

            // If Content-Length is 0 or we have received all body data
            if (contentLength == 0 || bodyReceived >= static_cast<size_t>(contentLength))
            {
                size_t firstNewline = allData.find("\r\n");
                string requestLine = allData.substr(0, firstNewline);
                string body = (contentLength > 0) ? allData.substr(bodyStart, contentLength) : "";

                cout << "REQUEST LINE: " << requestLine << endl;
                cout << "BODY: " << body << endl;
                
                handleRequest(clientSocket, requestLine, body);
                break;
            }
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
