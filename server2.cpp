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

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <signal.h>

using namespace std;

int PORT = 8080;

struct SessionInfo
{
    string role, userId;
};

map<string, SessionInfo> sessions;
mutex sessionsMutex, fileMutex;

struct Faculty
{
    string id, name, dept, desig, mobile, email, spec, pass, role;
};

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

string httpResponse(int status, const string &contentType, const string &body, const string &setCookie = "")
{
    ostringstream oss;
    oss << "HTTP/1.1 " << status << " " << reasonPhrase(status) << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Credentials: true\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Session-ID\r\n";
    if (!setCookie.empty())
    {
        oss << "Set-Cookie: " << setCookie << "; Path=/; HttpOnly; SameSite=Lax\r\n";
    }
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
    return value;
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

string handleListFaculty(const string &search = "")
{
    auto faculty = loadData();
    string lower_search = toLower(search);
    string json = R"({"status":"success","faculty":[)";
    bool first = true;
    int count = 0;
    for (const auto &f : faculty)
    {
        if (f.role == "faculty")
        {
            string combinedLower = toLower(f.name + " " + f.dept + " " + f.spec);
            if (search.empty() || combinedLower.find(lower_search) != string::npos)
            {
                if (!first)
                    json += ",";
                json += R"({"id":")" + jsonEscape(f.id) + R"(","name":")" + jsonEscape(f.name);
                json += R"(","department":")" + jsonEscape(f.dept) + R"(","designation":")" + jsonEscape(f.desig);
                json += R"(","mobile":")" + jsonEscape(f.mobile) + R"(","email":")" + jsonEscape(f.email);
                json += R"(","subject":")" + jsonEscape(f.spec) + R"(","qualification":"M.Tech","experience":"5","officeHours":"Mon-Wed 2-4 PM"})";
                first = false;
                count++;
            }
        }
    }
    json += R"(],"total":)" + to_string(count) + "}";
    return httpResponse(200, "application/json", json);
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
            string sessionId = "session_" + to_string(rand() % 1000000);
            {
                lock_guard<mutex> lock(sessionsMutex);
                sessions[sessionId] = {f.role, f.id};
            }
            string response = R"({"status":"success","sessionId":")" + sessionId +
                              R"(","userId":")" + jsonEscape(f.id) +
                              R"(","facultyId":")" + jsonEscape(f.id) +
                              R"(","name":")" + jsonEscape(f.name) +
                              R"(","role":")" + jsonEscape(f.role) + R"("})";
            string setCookie = "sessionId=" + sessionId;
            return httpResponse(200, "application/json", response, setCookie);
        }
    }
    cout << "LOGIN FAILED" << endl;
    return httpResponse(401, "application/json", R"({"status":"error","message":"Invalid credentials"})");
}

string handleAuthMe(const string &allHeaders)
{
    cout << "AUTH ME REQUEST HEADERS:\n"
         << allHeaders << endl;
    string sessionId;
    size_t cookiePos = allHeaders.find("Cookie:");
    if (cookiePos != string::npos)
    {
        size_t cookieEnd = allHeaders.find("\r\n", cookiePos);
        string cookies = allHeaders.substr(cookiePos + 7, cookieEnd - cookiePos - 7);
        cout << "COOKIES: " << cookies << endl;
        size_t sessionStart = cookies.find("sessionId=");
        if (sessionStart != string::npos)
        {
            sessionStart += 10;
            size_t sessionEnd = cookies.find(";", sessionStart);
            if (sessionEnd == string::npos)
                sessionEnd = cookies.length();
            sessionId = cookies.substr(sessionStart, sessionEnd - sessionStart);
            sessionId = trim(sessionId);
            cout << "SESSION FROM COOKIE: " << sessionId << endl;
        }
    }
    if (sessionId.empty())
    {
        cout << "NO SESSION ID FOUND" << endl;
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }
    cout << "CHECKING SESSION: " << sessionId << endl;
    {
        lock_guard<mutex> lock(sessionsMutex);
        if (sessions.find(sessionId) != sessions.end())
        {
            auto sess = sessions[sessionId];
            cout << "SESSION FOUND! User: " << sess.userId << " Role: " << sess.role << endl;
            auto faculty = loadData();
            for (const auto &f : faculty)
            {
                if (f.id == sess.userId)
                {
                    string response = R"({"status":"success","userId":")" + jsonEscape(sess.userId) +
                                      R"(","facultyId":")" + jsonEscape(sess.userId) +
                                      R"(","name":")" + jsonEscape(f.name) +
                                      R"(","role":")" + jsonEscape(sess.role) + R"("})";
                    return httpResponse(200, "application/json", response);
                }
            }
            string response = R"({"status":"success","userId":")" + jsonEscape(sess.userId) +
                              R"(","role":")" + jsonEscape(sess.role) + R"("})";
            return httpResponse(200, "application/json", response);
        }
    }
    cout << "SESSION NOT FOUND" << endl;
    return httpResponse(401, "application/json", R"({"status":"error","message":"Session expired"})");
}

string handleLogout(const string &allHeaders)
{
    cout << "LOGOUT REQUEST" << endl;
    string sessionId;
    size_t cookiePos = allHeaders.find("Cookie:");
    if (cookiePos != string::npos)
    {
        size_t cookieEnd = allHeaders.find("\r\n", cookiePos);
        string cookies = allHeaders.substr(cookiePos + 7, cookieEnd - cookiePos - 7);
        size_t sessionStart = cookies.find("sessionId=");
        if (sessionStart != string::npos)
        {
            sessionStart += 10;
            size_t sessionEnd = cookies.find(";", sessionStart);
            if (sessionEnd == string::npos)
                sessionEnd = cookies.length();
            sessionId = cookies.substr(sessionStart, sessionEnd - sessionStart);
            sessionId = trim(sessionId);
        }
    }
    if (!sessionId.empty())
    {
        lock_guard<mutex> lock(sessionsMutex);
        sessions.erase(sessionId);
        cout << "SESSION CLEARED: " << sessionId << endl;
    }
    string setCookie = "sessionId=; Max-Age=0";
    return httpResponse(200, "application/json", R"({"status":"success","message":"Logged out successfully"})", setCookie);
}

string handleUserProfile(const string &allHeaders)
{
    string sessionId;
    size_t cookiePos = allHeaders.find("Cookie:");
    if (cookiePos != string::npos)
    {
        size_t cookieEnd = allHeaders.find("\r\n", cookiePos);
        string cookies = allHeaders.substr(cookiePos + 7, cookieEnd - cookiePos - 7);
        size_t sessionStart = cookies.find("sessionId=");
        if (sessionStart != string::npos)
        {
            sessionStart += 10;
            size_t sessionEnd = cookies.find(";", sessionStart);
            if (sessionEnd == string::npos)
                sessionEnd = cookies.length();
            sessionId = cookies.substr(sessionStart, sessionEnd - sessionStart);
            sessionId = trim(sessionId);
        }
    }
    if (sessionId.empty() || sessions.find(sessionId) == sessions.end())
    {
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }
    auto sess = sessions[sessionId];
    auto faculty = loadData();
    for (const auto &f : faculty)
    {
        if (f.id == sess.userId)
        {
            string response = R"({"status":"success","id":")" + jsonEscape(f.id) + R"(","name":")" + jsonEscape(f.name) +
                              R"(","email":")" + jsonEscape(f.email) + R"(","mobile":")" + jsonEscape(f.mobile) +
                              R"(","department":")" + jsonEscape(f.dept) + R"(","designation":")" + jsonEscape(f.desig) +
                              R"(","subject":")" + jsonEscape(f.spec) + R"(","role":")" + jsonEscape(f.role) + R"("})";
            return httpResponse(200, "application/json", response);
        }
    }
    return httpResponse(404, "application/json", R"({"status":"error","message":"User not found"})");
}

string handleGetFaculty(const string &facultyId)
{
    auto faculty = loadData();
    for (const auto &f : faculty)
    {
        if (f.id == facultyId && f.role == "faculty")
        {
            string response = R"({"status":"success","id":")" + jsonEscape(f.id) +
                              R"(","name":")" + jsonEscape(f.name) +
                              R"(","email":")" + jsonEscape(f.email) +
                              R"(","mobile":")" + jsonEscape(f.mobile) +
                              R"(","department":")" + jsonEscape(f.dept) +
                              R"(","designation":")" + jsonEscape(f.desig) +
                              R"(","subject":")" + jsonEscape(f.spec) +
                              R"(","qualification":"M.Tech","experience":"5","officeHours":"Mon-Wed 2-4 PM"})";
            return httpResponse(200, "application/json", response);
        }
    }
    return httpResponse(404, "application/json", R"({"status":"error","message":"Faculty not found"})");
}

string handleAddFaculty(const string &body, const string &allHeaders)
{
    string sessionId;
    size_t cookiePos = allHeaders.find("Cookie:");
    if (cookiePos != string::npos)
    {
        size_t cookieEnd = allHeaders.find("\r\n", cookiePos);
        string cookies = allHeaders.substr(cookiePos + 7, cookieEnd - cookiePos - 7);
        size_t sessionStart = cookies.find("sessionId=");
        if (sessionStart != string::npos)
        {
            sessionStart += 10;
            size_t sessionEnd = cookies.find(";", sessionStart);
            if (sessionEnd == string::npos)
                sessionEnd = cookies.length();
            sessionId = cookies.substr(sessionStart, sessionEnd - sessionStart);
            sessionId = trim(sessionId);
        }
    }
    if (sessionId.empty() || sessions.find(sessionId) == sessions.end())
    {
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }
    auto data = parseJSON(body);
    Faculty newFaculty;
    newFaculty.id = data["id"];
    newFaculty.name = data["name"];
    newFaculty.email = data["email"];
    newFaculty.mobile = data["mobile"];
    newFaculty.dept = data["department"];
    newFaculty.desig = data["designation"];
    newFaculty.spec = data["subject"];
    newFaculty.pass = data.count("password") ? data["password"] : "default123";
    newFaculty.role = "faculty";
    string line = newFaculty.id + "," + newFaculty.name + "," + newFaculty.dept + "," + newFaculty.desig + "," + newFaculty.mobile + "," + newFaculty.email + "," + newFaculty.spec + "," + newFaculty.pass + "," + newFaculty.role;
    ofstream userFile("users.txt", ios::app);
    if (userFile)
    {
        userFile << "\n"
                 << line;
        userFile.close();
        cout << "FACULTY ADDED: " << newFaculty.id << " " << newFaculty.name << endl;
        string response = R"({"status":"success","message":"Faculty added successfully","faculty":{"id":")" + jsonEscape(newFaculty.id) + R"(","name":")" + jsonEscape(newFaculty.name) + R"("}})";
        return httpResponse(201, "application/json", response);
    }
    return httpResponse(500, "application/json", R"({"status":"error","message":"Failed to add faculty"})");
}

string handleDeleteFaculty(const string &facultyId, const string &allHeaders)
{
    string sessionId;
    size_t cookiePos = allHeaders.find("Cookie:");
    if (cookiePos != string::npos)
    {
        size_t cookieEnd = allHeaders.find("\r\n", cookiePos);
        string cookies = allHeaders.substr(cookiePos + 7, cookieEnd - cookiePos - 7);
        size_t sessionStart = cookies.find("sessionId=");
        if (sessionStart != string::npos)
        {
            sessionStart += 10;
            size_t sessionEnd = cookies.find(";", sessionStart);
            if (sessionEnd == string::npos)
                sessionEnd = cookies.length();
            sessionId = cookies.substr(sessionStart, sessionEnd - sessionStart);
            sessionId = trim(sessionId);
        }
    }
    if (sessionId.empty() || sessions.find(sessionId) == sessions.end())
    {
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }
    auto allFaculty = loadData();
    vector<Faculty> updatedList;
    bool found = false;
    for (const auto &f : allFaculty)
    {
        if (f.id != facultyId)
        {
            updatedList.push_back(f);
        }
        else
        {
            found = true;
        }
    }
    if (!found)
    {
        return httpResponse(404, "application/json", R"({"status":"error","message":"Faculty not found"})");
    }
    ofstream userFile("users.txt", ios::trunc);
    if (userFile)
    {
        bool first = true;
        for (const auto &f : updatedList)
        {
            string line = f.id + "," + f.name + "," + f.dept + "," + f.desig + "," + f.mobile + "," + f.email + "," + f.spec + "," + f.pass + "," + f.role;
            if (!first)
                userFile << "\n";
            userFile << line;
            first = false;
        }
        userFile.close();
        cout << "FACULTY DELETED: " << facultyId << endl;
        return httpResponse(200, "application/json", R"({"status":"success","message":"Faculty deleted successfully"})");
    }
    return httpResponse(500, "application/json", R"({"status":"error","message":"Failed to delete faculty"})");
}

string handleUpdateFaculty(const string &facultyId, const string &body, const string &allHeaders)
{
    string sessionId;
    size_t cookiePos = allHeaders.find("Cookie:");
    if (cookiePos != string::npos)
    {
        size_t cookieEnd = allHeaders.find("\r\n", cookiePos);
        string cookies = allHeaders.substr(cookiePos + 7, cookieEnd - cookiePos - 7);
        size_t sessionStart = cookies.find("sessionId=");
        if (sessionStart != string::npos)
        {
            sessionStart += 10;
            size_t sessionEnd = cookies.find(";", sessionStart);
            if (sessionEnd == string::npos)
                sessionEnd = cookies.length();
            sessionId = cookies.substr(sessionStart, sessionEnd - sessionStart);
            sessionId = trim(sessionId);
        }
    }
    if (sessionId.empty() || sessions.find(sessionId) == sessions.end())
    {
        return httpResponse(401, "application/json", R"({"status":"error","message":"Not authenticated"})");
    }
    auto data = parseJSON(body);
    cout << "UPDATE REQUEST FOR: " << facultyId << " with " << data.size() << " fields" << endl;
    for (const auto &kv : data)
    {
        cout << "UPDATE FIELD: " << kv.first << " = " << kv.second << endl;
    }
    auto allFaculty = loadData();
    bool found = false;
    for (auto &f : allFaculty)
    {
        if (f.id == facultyId)
        {
            found = true;
            cout << "FOUND FACULTY TO UPDATE: " << f.name << endl;
            if (data.count("name") && !data["name"].empty())
            {
                cout << "UPDATING NAME FROM " << f.name << " TO " << data["name"] << endl;
                f.name = data["name"];
            }
            if (data.count("email") && !data["email"].empty())
                f.email = data["email"];
            if (data.count("mobile") && !data["mobile"].empty())
                f.mobile = data["mobile"];
            if (data.count("department") && !data["department"].empty())
                f.dept = data["department"];
            if (data.count("designation") && !data["designation"].empty())
                f.desig = data["designation"];
            if (data.count("subject") && !data["subject"].empty())
                f.spec = data["subject"];
            break;
        }
    }
    if (!found)
    {
        return httpResponse(404, "application/json", R"({"status":"error","message":"Faculty not found"})");
    }
    cout << "UPDATING FACULTY: " << facultyId << " with " << allFaculty.size() << " total records" << endl;
    ofstream userFile("users.txt", ios::trunc);
    if (userFile)
    {
        bool first = true;
        for (const auto &f : allFaculty)
        {
            string line = f.id + "," + f.name + "," + f.dept + "," + f.desig + "," + f.mobile + "," + f.email + "," + f.spec + "," + f.pass + "," + f.role;
            if (!first)
                userFile << "\n";
            userFile << line;
            first = false;
        }
        userFile.close();
        cout << "FACULTY UPDATED: " << facultyId << endl;
        string response = R"({"status":"success","message":"Faculty updated successfully","faculty":{"id":")" + jsonEscape(facultyId) + R"("}})";
        return httpResponse(200, "application/json", response);
    }
    return httpResponse(500, "application/json", R"({"status":"error","message":"Failed to update faculty"})");
}

void handleRequest(int clientSocket, const string &requestLine, const string &allHeaders, const string &body);

void handleClient(int clientSocket)
{
    char buffer[4096];
    string allData;
    int contentLength = 0;
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
            size_t clPos = allData.find("Content-Length:");
            if (clPos != string::npos && clPos < headerEnd)
            {
                size_t clStart = allData.find_first_not_of(" \t", clPos + 15);
                size_t clEnd = allData.find("\r\n", clStart);
                string clStr = allData.substr(clStart, clEnd - clStart);
                contentLength = stoi(clStr);
                cout << "CONTENT-LENGTH: " << contentLength << endl;
            }
            size_t bodyStart = headerEnd + 4;
            size_t bodyReceived = allData.size() - bodyStart;
            cout << "BODY RECEIVED: " << bodyReceived << " bytes, EXPECTED: " << contentLength << " bytes" << endl;
            if (contentLength == 0 || bodyReceived >= static_cast<size_t>(contentLength))
            {
                size_t firstNewline = allData.find("\r\n");
                string requestLine = allData.substr(0, firstNewline);
                string allHeaders = allData.substr(0, headerEnd);
                string body = (contentLength > 0) ? allData.substr(bodyStart, contentLength) : "";
                cout << "REQUEST LINE: " << requestLine << endl;
                cout << "BODY: " << body << endl;
                handleRequest(clientSocket, requestLine, allHeaders, body);
                break;
            }
        }
    }
    close(clientSocket);
}

string handleStatsEndpoint()
{
    auto faculty = loadData();
    int totalFaculty = 0, totalStudents = 0;
    for (const auto &f : faculty)
    {
        if (f.role == "faculty")
            totalFaculty++;
        else if (f.role == "student")
            totalStudents++;
    }
    string json = R"({"totalFaculty":)" + to_string(totalFaculty) + R"(,"departments":4,"students":)" + to_string(totalStudents) + R"(,"recentlyAdded":15})";
    return httpResponse(200, "application/json", json);
}

string handleDepartmentsEndpoint()
{
    string json = R"({"departments":[{"name":"CSE","count":45},{"name":"ECE","count":35},{"name":"ME","count":40},{"name":"CIVIL","count":30}]})";
    return httpResponse(200, "application/json", json);
}

void handleRequest(int clientSocket, const string &requestLine, const string &allHeaders, const string &body)
{
    string path = parseRequestPath(requestLine);
    string method = requestLine.substr(0, requestLine.find(" "));
    cout << "REQUEST: " << method << " " << path << endl;
    if (path == "/api/auth/login")
    {
        sendAll(clientSocket, handleLogin(body));
    }
    else if (path == "/api/auth/me")
    {
        sendAll(clientSocket, handleAuthMe(allHeaders));
    }
    else if (path == "/api/auth/logout")
    {
        sendAll(clientSocket, handleLogout(allHeaders));
    }
    else if (path == "/api/faculty/list")
    {
        string search = getQueryParam(requestLine, "search");
        sendAll(clientSocket, handleListFaculty(search));
    }
    else if (path == "/api/faculty")
    {
        sendAll(clientSocket, handleListFaculty());
    }
    else if (path == "/api/stats/summary")
    {
        sendAll(clientSocket, handleStatsEndpoint());
    }
    else if (path == "/api/stats/departments")
    {
        sendAll(clientSocket, handleDepartmentsEndpoint());
    }
    else if (path == "/api/profile")
    {
        sendAll(clientSocket, handleUserProfile(allHeaders));
    }
    else if (path == "/api/faculty" && method == "POST")
    {
        sendAll(clientSocket, handleAddFaculty(body, allHeaders));
    }
    else if (path.find("/api/faculty/") == 0 && path != "/api/faculty/list")
    {
        size_t lastSlash = path.rfind("/");
        string facultyId = path.substr(lastSlash + 1);
        if (method == "DELETE")
        {
            sendAll(clientSocket, handleDeleteFaculty(facultyId, allHeaders));
        }
        else if (method == "PUT")
        {
            sendAll(clientSocket, handleUpdateFaculty(facultyId, body, allHeaders));
        }
        else
        {
            sendAll(clientSocket, handleGetFaculty(facultyId));
        }
    }
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
    else if (path.find(".html") != string::npos)
    {
        string filePath;
        if (path.find("/components/") == 0)
        {
            filePath = "." + path;
        }
        else
        {
            filePath = "public" + path;
        }
        cout << "LOADING HTML: " << filePath << endl;
        ifstream file(filePath, ios::binary);
        if (file)
        {
            cout << "HTML FOUND" << endl;
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            sendAll(clientSocket, httpResponse(200, "text/html", content));
        }
        else
        {
            cout << "HTML NOT FOUND: " << filePath << endl;
            sendAll(clientSocket, httpResponse(404, "application/json", R"({"error":"File not found"})"));
        }
    }
    else if (path.find(".css") != string::npos || path.find(".js") != string::npos ||
             path.find(".png") != string::npos || path.find(".jpg") != string::npos ||
             path.find(".jpeg") != string::npos || path.find(".ico") != string::npos ||
             path.find(".txt") != string::npos)
    {
        string filePath;
        if (path.find("/css/") == 0 || path.find("/js/") == 0 || path.find("/components/") == 0 || path.find("/assets/") == 0)
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

int main()
{
    const char *portEnv = getenv("PORT");
    if (portEnv)
        PORT = stoi(portEnv);

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
