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
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 8080

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

bool sendAll(SOCKET sock, const string &data)
{
    size_t sentTotal = 0;
    while (sentTotal < data.size())
    {
        int sent = send(sock, data.c_str() + sentTotal, static_cast<int>(data.size() - sentTotal), 0);
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
    case 405:
        return "Method Not Allowed";
    case 413:
        return "Payload Too Large";
    case 500:
        return "Internal Server Error";
    default:
        return "OK";
    }
}

string makeResponse(int status, const string &contentType, const string &body, const vector<string> &extraHeaders = {})
{
    stringstream ss;
    ss << "HTTP/1.1 " << status << " " << reasonPhrase(status) << "\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Connection: close\r\n";
    for (const string &h : extraHeaders)
        ss << h << "\r\n";
    ss << "\r\n";
    ss << body;
    return ss.str();
}

string jsonEscape(const string &input)
{
    string out;
    out.reserve(input.size());
    for (char c : input)
    {
        switch (c)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

string readFile(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (file)
        return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    ifstream fallback("public/" + filename, ios::binary);
    if (!fallback)
        return "";
    return string((istreambuf_iterator<char>(fallback)), istreambuf_iterator<char>());
}

bool startsWith(const string &value, const string &prefix)
{
    return value.rfind(prefix, 0) == 0;
}

bool endsWith(const string &value, const string &suffix)
{
    if (suffix.size() > value.size())
        return false;
    return equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

void splitPathAndQuery(const string &pathWithQuery, string &pathOnly, string &query)
{
    size_t qPos = pathWithQuery.find('?');
    if (qPos == string::npos)
    {
        pathOnly = pathWithQuery;
        query.clear();
        return;
    }

    pathOnly = pathWithQuery.substr(0, qPos);
    query = pathWithQuery.substr(qPos + 1);
}

map<string, string> parseQueryString(const string &query)
{
    map<string, string> out;
    size_t start = 0;
    while (start < query.size())
    {
        size_t amp = query.find('&', start);
        if (amp == string::npos)
            amp = query.size();

        string token = query.substr(start, amp - start);
        size_t eq = token.find('=');
        if (eq != string::npos)
        {
            string key = urlDecode(token.substr(0, eq));
            string value = urlDecode(token.substr(eq + 1));
            out[key] = value;
        }
        else if (!token.empty())
        {
            out[urlDecode(token)] = "";
        }

        start = amp + 1;
    }

    return out;
}

bool parseJsonStringLiteral(const string &s, size_t &i, string &out)
{
    if (i >= s.size() || s[i] != '"')
        return false;

    ++i;
    out.clear();
    while (i < s.size())
    {
        char c = s[i++];
        if (c == '"')
            return true;

        if (c == '\\')
        {
            if (i >= s.size())
                return false;
            char esc = s[i++];
            switch (esc)
            {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            default:
                out.push_back(esc);
                break;
            }
            continue;
        }

        out.push_back(c);
    }

    return false;
}

map<string, string> parseJsonObjectFlat(const string &body)
{
    map<string, string> obj;
    size_t i = 0;

    auto skipWs = [&]()
    {
        while (i < body.size() && isspace(static_cast<unsigned char>(body[i])))
            ++i;
    };

    skipWs();
    if (i >= body.size() || body[i] != '{')
        return obj;
    ++i;

    while (i < body.size())
    {
        skipWs();
        if (i < body.size() && body[i] == '}')
        {
            ++i;
            break;
        }

        string key;
        if (!parseJsonStringLiteral(body, i, key))
            return map<string, string>{};

        skipWs();
        if (i >= body.size() || body[i] != ':')
            return map<string, string>{};
        ++i;

        skipWs();
        string value;
        if (i < body.size() && body[i] == '"')
        {
            if (!parseJsonStringLiteral(body, i, value))
                return map<string, string>{};
        }
        else
        {
            size_t start = i;
            while (i < body.size() && body[i] != ',' && body[i] != '}')
                ++i;
            value = trim(body.substr(start, i - start));
        }

        obj[key] = value;

        skipWs();
        if (i < body.size() && body[i] == ',')
        {
            ++i;
            continue;
        }
        if (i < body.size() && body[i] == '}')
        {
            ++i;
            break;
        }
    }

    return obj;
}

string getJsonValue(const map<string, string> &obj, const string &key)
{
    auto it = obj.find(key);
    if (it == obj.end())
        return "";
    return trim(it->second);
}

string jsonErrorBody(const string &msg)
{
    return string("{\"error\":\"") + jsonEscape(msg) + "\"}";
}

string makeJsonErrorResponse(int status, const string &msg)
{
    return makeResponse(status, "application/json; charset=utf-8", jsonErrorBody(msg));
}

string contentTypeForPath(const string &path)
{
    if (endsWith(path, ".html"))
        return "text/html; charset=utf-8";
    if (endsWith(path, ".css"))
        return "text/css; charset=utf-8";
    if (endsWith(path, ".js"))
        return "application/javascript; charset=utf-8";
    if (endsWith(path, ".json"))
        return "application/json; charset=utf-8";
    if (endsWith(path, ".txt"))
        return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

string facultyToJson(const Faculty &f)
{
    stringstream ss;
    ss << "{";
    ss << "\"id\":\"" << jsonEscape(f.id) << "\",";
    ss << "\"name\":\"" << jsonEscape(f.name) << "\",";
    ss << "\"department\":\"" << jsonEscape(f.dept) << "\",";
    ss << "\"designation\":\"" << jsonEscape(f.desig) << "\",";
    ss << "\"subject\":\"" << jsonEscape(f.spec) << "\",";
    ss << "\"email\":\"" << jsonEscape(f.email) << "\",";
    ss << "\"mobile\":\"" << jsonEscape(f.mobile) << "\",";
    ss << "\"officeHours\":\"N/A\",";
    ss << "\"qualification\":\"N/A\",";
    ss << "\"experience\":0";
    ss << "}";
    return ss.str();
}

bool findUserBySession(const SessionInfo &s, Faculty &out)
{
    vector<Faculty> list = loadData();
    for (const auto &f : list)
    {
        if (f.id == s.userId && f.role == s.role)
        {
            out = f;
            return true;
        }
    }
    return false;
}

bool authenticateUser(const string &usernameInput, const string &password, Faculty &out)
{
    string username = toLower(trim(usernameInput));
    vector<Faculty> list = loadData();

    for (const auto &f : list)
    {
        if (f.pass != password)
            continue;

        string idLower = toLower(f.id);
        string nameLower = toLower(f.name);
        string emailLower = toLower(f.email);

        if (username == idLower || username == nameLower || username == emailLower)
        {
            out = f;
            return true;
        }
    }

    return false;
}

string nextFacultyId(const vector<Faculty> &list)
{
    int maxId = 100;
    for (const auto &f : list)
    {
        bool numeric = !f.id.empty() && all_of(f.id.begin(), f.id.end(), [](unsigned char c)
                                               { return isdigit(c); });
        if (!numeric)
            continue;
        maxId = max(maxId, stoi(f.id));
    }
    return to_string(maxId + 1);
}

bool splitCsvLine(const string &line, array<string, 9> &fields)
{
    stringstream ss(line);
    for (size_t i = 0; i < fields.size(); ++i)
    {
        if (!getline(ss, fields[i], ','))
            return false;
    }
    return true;
}

bool isValidRole(const string &role)
{
    return role == "admin" || role == "faculty" || role == "student";
}

vector<Faculty> loadData()
{
    lock_guard<mutex> lock(fileMutex);

    vector<Faculty> list;
    ifstream file("users.txt");
    string line;

    while (getline(file, line))
    {
        if (trim(line).empty())
            continue;

        array<string, 9> fields{};
        if (!splitCsvLine(line, fields))
            continue;

        Faculty f;
        f.id = trim(fields[0]);
        f.name = trim(fields[1]);
        f.dept = trim(fields[2]);
        f.desig = trim(fields[3]);
        f.mobile = trim(fields[4]);
        f.email = trim(fields[5]);
        f.spec = trim(fields[6]);
        f.pass = trim(fields[7]);
        f.role = trim(fields[8]);

        if (!isValidRole(f.role))
            continue;

        list.push_back(f);
    }
    return list;
}

bool saveData(const vector<Faculty> &list)
{
    lock_guard<mutex> lock(fileMutex);

    const string tmpPath = "users.txt.tmp";
    ofstream file(tmpPath, ios::trunc);
    if (!file)
        return false;

    for (const auto &f : list)
    {
        file << f.id << "," << f.name << "," << f.dept << ","
             << f.desig << "," << f.mobile << "," << f.email << ","
             << f.spec << "," << f.pass << "," << f.role << "\n";
    }

    file.flush();
    file.close();

    remove("users.txt");
    return rename(tmpPath.c_str(), "users.txt") == 0;
}

bool appendRawFacultyCsvLine(const string &line)
{
    if (line.find('\n') != string::npos || line.find('\r') != string::npos)
        return false;

    array<string, 9> fields{};
    if (!splitCsvLine(line, fields))
        return false;
    if (!isValidRole(trim(fields[8])))
        return false;

    lock_guard<mutex> lock(fileMutex);
    ofstream file("users.txt", ios::app);
    if (!file)
        return false;
    file << line << "\n";
    return static_cast<bool>(file);
}

bool hasCsvUnsafeChars(const string &value)
{
    return value.find(',') != string::npos || value.find('\n') != string::npos || value.find('\r') != string::npos;
}

bool buildCsvLineFromForm(const map<string, string> &form, string &csvLine)
{
    const array<string, 9> keys = {"id", "name", "dept", "desig", "mobile", "email", "spec", "pass", "role"};
    array<string, 9> values{};

    for (size_t i = 0; i < keys.size(); ++i)
    {
        auto it = form.find(keys[i]);
        if (it == form.end())
            return false;

        values[i] = trim(it->second);
        if (values[i].empty() || hasCsvUnsafeChars(values[i]))
            return false;
    }

    if (!isValidRole(values[8]))
        return false;

    csvLine = values[0];
    for (size_t i = 1; i < values.size(); ++i)
        csvLine += "," + values[i];

    return true;
}

bool isOptionalFieldAllowedForAdminUpdate(const string &key)
{
    return key == "name" || key == "dept" || key == "desig" || key == "mobile" ||
           key == "email" || key == "spec" || key == "pass" || key == "role";
}

string urlDecode(const string &s)
{
    string out;
    out.reserve(s.size());

    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '+')
        {
            out.push_back(' ');
        }
        else if (s[i] == '%' && i + 2 < s.size() && isxdigit(static_cast<unsigned char>(s[i + 1])) && isxdigit(static_cast<unsigned char>(s[i + 2])))
        {
            int hi = isdigit(static_cast<unsigned char>(s[i + 1])) ? s[i + 1] - '0' : tolower(static_cast<unsigned char>(s[i + 1])) - 'a' + 10;
            int lo = isdigit(static_cast<unsigned char>(s[i + 2])) ? s[i + 2] - '0' : tolower(static_cast<unsigned char>(s[i + 2])) - 'a' + 10;
            out.push_back(static_cast<char>((hi << 4) | lo));
            i += 2;
        }
        else
        {
            out.push_back(s[i]);
        }
    }

    return out;
}

map<string, string> parseFormBody(const string &body)
{
    map<string, string> kv;
    size_t start = 0;
    while (start < body.size())
    {
        size_t amp = body.find('&', start);
        if (amp == string::npos)
            amp = body.size();

        string token = body.substr(start, amp - start);
        size_t eq = token.find('=');
        if (eq != string::npos)
        {
            string key = urlDecode(token.substr(0, eq));
            string value = urlDecode(token.substr(eq + 1));
            kv[key] = value;
        }

        start = amp + 1;
    }
    return kv;
}

bool receiveHttpRequest(SOCKET clientSocket, string &requestRaw)
{
    static constexpr size_t kMaxRequestBytes = 64 * 1024;
    static constexpr size_t kChunkSize = 4096;

    requestRaw.clear();
    char buffer[kChunkSize];
    size_t headerEndPos = string::npos;
    size_t expectedBodySize = 0;

    while (requestRaw.size() < kMaxRequestBytes)
    {
        int received = recv(clientSocket, buffer, static_cast<int>(kChunkSize), 0);
        if (received <= 0)
            return false;

        requestRaw.append(buffer, static_cast<size_t>(received));

        if (headerEndPos == string::npos)
        {
            headerEndPos = requestRaw.find("\r\n\r\n");
            if (headerEndPos != string::npos)
            {
                size_t headerBodyStart = headerEndPos + 4;
                string headersPart = requestRaw.substr(0, headerEndPos);

                size_t clPos = toLower(headersPart).find("content-length:");
                if (clPos != string::npos)
                {
                    size_t valueStart = clPos + string("content-length:").size();
                    size_t valueEnd = headersPart.find("\r\n", valueStart);
                    string lenText = trim(headersPart.substr(valueStart, valueEnd - valueStart));
                    if (lenText.empty())
                        return false;

                    for (char c : lenText)
                        if (!isdigit(static_cast<unsigned char>(c)))
                            return false;

                    unsigned long long len = 0;
                    try
                    {
                        len = stoull(lenText);
                    }
                    catch (...)
                    {
                        return false;
                    }

                    if (len > kMaxRequestBytes)
                        return false;

                    expectedBodySize = static_cast<size_t>(len);
                }

                if (requestRaw.size() >= headerBodyStart + expectedBodySize)
                    return true;
            }
        }
        else
        {
            if (requestRaw.size() >= (headerEndPos + 4 + expectedBodySize))
                return true;
        }
    }

    return false;
}

bool parseHttpRequest(const string &raw, HttpRequest &req)
{
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == string::npos)
        return false;

    string head = raw.substr(0, headerEnd);
    req.body = raw.substr(headerEnd + 4);

    stringstream ss(head);
    string requestLine;
    if (!getline(ss, requestLine))
        return false;
    if (!requestLine.empty() && requestLine.back() == '\r')
        requestLine.pop_back();

    string httpVersion;
    {
        stringstream rl(requestLine);
        if (!(rl >> req.method >> req.path >> httpVersion))
            return false;
    }

    req.headers.clear();
    string line;
    while (getline(ss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;

        size_t colon = line.find(':');
        if (colon == string::npos)
            continue;

        string key = toLower(trim(line.substr(0, colon)));
        string value = trim(line.substr(colon + 1));
        req.headers[key] = value;
    }

    auto clIt = req.headers.find("content-length");
    if (clIt != req.headers.end())
    {
        unsigned long long declaredLen = 0;
        try
        {
            declaredLen = stoull(clIt->second);
        }
        catch (...)
        {
            return false;
        }

        if (declaredLen != req.body.size())
            return false;
    }

    return true;
}

string htmlEscape(const string &input)
{
    string out;
    out.reserve(input.size());
    for (char c : input)
    {
        switch (c)
        {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

string generateSessionId()
{
    random_device rd;
    mt19937_64 gen((static_cast<unsigned long long>(rd()) << 32) ^ rd());
    uniform_int_distribution<unsigned long long> dist(0, numeric_limits<unsigned long long>::max());

    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 4; ++i)
        ss << setw(16) << dist(gen);
    return ss.str();
}

string getSessionIdFromCookie(const map<string, string> &headers)
{
    auto it = headers.find("cookie");
    if (it == headers.end())
        return "";

    const string &cookie = it->second;
    size_t start = 0;
    while (start < cookie.size())
    {
        size_t semi = cookie.find(';', start);
        if (semi == string::npos)
            semi = cookie.size();

        string kv = trim(cookie.substr(start, semi - start));
        size_t eq = kv.find('=');
        if (eq != string::npos)
        {
            string key = trim(kv.substr(0, eq));
            string value = trim(kv.substr(eq + 1));
            if (key == "session")
                return value;
        }

        start = semi + 1;
    }
    return "";
}

bool getSessionInfo(const HttpRequest &req, SessionInfo &out)
{
    string sid = getSessionIdFromCookie(req.headers);
    if (sid.empty())
        return false;

    lock_guard<mutex> lock(sessionsMutex);
    auto it = sessions.find(sid);
    if (it == sessions.end())
        return false;

    out = it->second;
    return true;
}

void eraseSession(const HttpRequest &req)
{
    string sid = getSessionIdFromCookie(req.headers);
    if (sid.empty())
        return;

    lock_guard<mutex> lock(sessionsMutex);
    sessions.erase(sid);
}

bool mustBeRole(const SessionInfo &s, const string &requiredRole)
{
    return s.role == requiredRole;
}

void handleClient(SOCKET clientSocket)
{
    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);
    getpeername(clientSocket, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
    string clientIP = inet_ntoa(clientAddr.sin_addr);
    int clientPort = ntohs(clientAddr.sin_port);
    cout << "\n[CLIENT] Connected from " << clientIP << ":" << clientPort << "\n";

    string raw;
    if (!receiveHttpRequest(clientSocket, raw))
    {
        sendAll(clientSocket, makeResponse(400, "text/plain; charset=utf-8", "Malformed request"));
        closesocket(clientSocket);
        return;
    }

    HttpRequest req;
    if (!parseHttpRequest(raw, req))
    {
        cout << "[ERROR] Failed to parse HTTP request\n";
        sendAll(clientSocket, makeResponse(400, "text/plain; charset=utf-8", "Invalid HTTP request"));
        closesocket(clientSocket);
        return;
    }

    cout << "[REQUEST] " << req.method << " " << req.path << "\n";

    string pathOnly;
    string queryString;
    splitPathAndQuery(req.path, pathOnly, queryString);
    map<string, string> query = parseQueryString(queryString);

    if (pathOnly.empty())
        pathOnly = "/";

    string responsePayload;

    if (startsWith(pathOnly, "/api/"))
    {
        if (req.method == "POST" && pathOnly == "/api/auth/login")
        {
            auto body = parseJsonObjectFlat(req.body);
            string username = getJsonValue(body, "username");
            string password = getJsonValue(body, "password");

            cout << "  [LOGIN] Username: " << username << "\n";

            if (username.empty() || password.empty())
            {
                cout << "  [LOGIN FAILED] Empty username or password\n";
                responsePayload = makeJsonErrorResponse(400, "Username and password are required");
            }
            else
            {
                Faculty user{};
                if (!authenticateUser(username, password, user))
                {
                    cout << "  [LOGIN FAILED] Invalid credentials\n";
                    responsePayload = makeJsonErrorResponse(401, "Invalid username or password");
                }
                else
                {
                    cout << "  [LOGIN SUCCESS] User: " << user.name << " (" << user.id << ") - Role: " << user.role << "\n";
                    string sid = generateSessionId();
                    {
                        lock_guard<mutex> lock(sessionsMutex);
                        sessions[sid] = SessionInfo{user.role, user.id};
                    }
                    cout << "  [SESSION] Created new session for user " << user.id << "\n";

                    vector<string> headers = {
                        "Set-Cookie: session=" + sid + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=1800"};

                    stringstream bodyJson;
                    bodyJson << "{";
                    bodyJson << "\"id\":\"" << jsonEscape(user.id) << "\",";
                    bodyJson << "\"name\":\"" << jsonEscape(user.name) << "\",";
                    bodyJson << "\"username\":\"" << jsonEscape(user.id) << "\",";
                    bodyJson << "\"role\":\"" << jsonEscape(user.role) << "\"";
                    if (user.role == "faculty")
                        bodyJson << ",\"facultyId\":\"" << jsonEscape(user.id) << "\"";
                    bodyJson << "}";

                    responsePayload = makeResponse(200, "application/json; charset=utf-8", bodyJson.str(), headers);
                }
            }
        }
        else if (req.method == "POST" && pathOnly == "/api/auth/logout")
        {
            cout << "  [LOGOUT] User logging out\n";
            eraseSession(req);
            vector<string> headers = {
                "Set-Cookie: session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0"};
            responsePayload = makeResponse(200, "application/json; charset=utf-8", "{\"ok\":true}", headers);
        }
        else if (req.method == "GET" && pathOnly == "/api/auth/me")
        {
            cout << "  [API] /api/auth/me - Checking authentication\n";
            SessionInfo s;
            if (!getSessionInfo(req, s))
            {
                cout << "       [ERROR] Not authenticated\n";
                responsePayload = makeJsonErrorResponse(401, "Unauthorized");
            }
            else
            {
                Faculty user{};
                if (!findUserBySession(s, user))
                {
                    cout << "       [ERROR] User not found\n";
                    responsePayload = makeJsonErrorResponse(401, "Unauthorized");
                }
                else
                {
                    cout << "       [SUCCESS] User: " << user.name << "\n";
                    stringstream bodyJson;
                    bodyJson << "{";
                    bodyJson << "\"id\":\"" << jsonEscape(user.id) << "\",";
                    bodyJson << "\"name\":\"" << jsonEscape(user.name) << "\",";
                    bodyJson << "\"username\":\"" << jsonEscape(user.id) << "\",";
                    bodyJson << "\"role\":\"" << jsonEscape(user.role) << "\"";
                    if (user.role == "faculty")
                        bodyJson << ",\"facultyId\":\"" << jsonEscape(user.id) << "\"";
                    bodyJson << "}";

                    responsePayload = makeResponse(200, "application/json; charset=utf-8", bodyJson.str());
                }
            }
        }
        else if (req.method == "GET" && pathOnly == "/api/faculty")
        {
            SessionInfo s;
            if (!getSessionInfo(req, s))
            {
                cout << "  [API] /api/faculty - UNAUTHORIZED\n";
                responsePayload = makeJsonErrorResponse(401, "Unauthorized");
            }
            else
            {
                string term = toLower(trim(query.count("search") ? query["search"] : ""));
                cout << "  [API] /api/faculty - User: " << s.userId << "\n";
                if (!term.empty())
                    cout << "       Search term: " << term << "\n";
                vector<Faculty> list = loadData();

                stringstream bodyJson;
                bodyJson << "{\"faculty\":[";
                bool first = true;
                int total = 0;

                for (const auto &f : list)
                {
                    if (f.role != "faculty")
                        continue;

                    if (!term.empty())
                    {
                        string hay = toLower(f.name + " " + f.dept + " " + f.spec + " " + f.desig);
                        if (hay.find(term) == string::npos)
                            continue;
                    }

                    if (!first)
                        bodyJson << ",";
                    first = false;
                    ++total;
                    bodyJson << facultyToJson(f);
                }

                bodyJson << "],\"total\":" << total << "}";
                cout << "       Result: Found " << total << " faculty members\n";
                responsePayload = makeResponse(200, "application/json; charset=utf-8", bodyJson.str());
            }
        }
        else if (startsWith(pathOnly, "/api/faculty/") && pathOnly.size() > string("/api/faculty/").size())
        {
            SessionInfo s;
            if (!getSessionInfo(req, s))
            {
                responsePayload = makeJsonErrorResponse(401, "Unauthorized");
            }
            else
            {
                string id = pathOnly.substr(string("/api/faculty/").size());
                vector<Faculty> list = loadData();
                auto it = find_if(list.begin(), list.end(), [&](const Faculty &f)
                                  { return f.id == id && f.role == "faculty"; });

                if (req.method == "GET")
                {
                    cout << "  [API] GET /api/faculty/" << id << "\n";
                    if (it == list.end())
                    {
                        cout << "       [ERROR] Faculty not found\n";
                        responsePayload = makeJsonErrorResponse(404, "Faculty not found");
                    }
                    else
                    {
                        cout << "       [SUCCESS] Found: " << it->name << "\n";
                        responsePayload = makeResponse(200, "application/json; charset=utf-8", facultyToJson(*it));
                    }
                }
                else if (req.method == "PUT")
                {
                    cout << "  [API] PUT /api/faculty/" << id << " - Updating\n";
                    if (it == list.end())
                    {
                        cout << "       [ERROR] Faculty not found\n";
                        responsePayload = makeJsonErrorResponse(404, "Faculty not found");
                    }
                    else
                    {
                        auto body = parseJsonObjectFlat(req.body);

                        bool isAdmin = s.role == "admin";
                        bool isSelfFaculty = s.role == "faculty" && s.userId == id;

                        if (!isAdmin && !isSelfFaculty)
                        {
                            cout << "       [ERROR] Access denied\n";
                            responsePayload = makeJsonErrorResponse(403, "Access denied");
                        }
                        else
                        {
                            if (isAdmin)
                            {
                                string v;
                                v = getJsonValue(body, "name");
                                if (!v.empty() && !hasCsvUnsafeChars(v))
                                    it->name = v;

                                v = getJsonValue(body, "department");
                                if (!v.empty() && !hasCsvUnsafeChars(v))
                                    it->dept = v;

                                v = getJsonValue(body, "designation");
                                if (!v.empty() && !hasCsvUnsafeChars(v))
                                    it->desig = v;

                                v = getJsonValue(body, "subject");
                                if (!v.empty() && !hasCsvUnsafeChars(v))
                                    it->spec = v;
                            }

                            string email = getJsonValue(body, "email");
                            string mobile = getJsonValue(body, "mobile");
                            if (!email.empty() && !hasCsvUnsafeChars(email))
                                it->email = email;
                            if (!mobile.empty() && !hasCsvUnsafeChars(mobile))
                                it->mobile = mobile;

                            if (!saveData(list))
                            {
                                cout << "       [ERROR] Failed to save\n";
                                responsePayload = makeJsonErrorResponse(500, "Failed to save data");
                            }
                            else
                            {
                                cout << "       [SUCCESS] Faculty updated: " << it->name << "\n";
                                responsePayload = makeResponse(200, "application/json; charset=utf-8", facultyToJson(*it));
                            }
                        }
                    }
                }
                else if (req.method == "DELETE")
                {
                    cout << "  [API] DELETE /api/faculty/" << id << "\n";
                    if (s.role != "admin")
                    {
                        cout << "       [ERROR] Only admins can delete\n";
                        responsePayload = makeJsonErrorResponse(403, "Access denied");
                    }
                    else
                    {
                        if (it == list.end())
                        {
                            cout << "       [ERROR] Faculty not found\n";
                            responsePayload = makeJsonErrorResponse(404, "Faculty not found");
                        }
                        else
                        {
                            string deletedName = it->name;
                            list.erase(it);
                            if (!saveData(list))
                            {
                                cout << "       [ERROR] Failed to save\n";
                                responsePayload = makeJsonErrorResponse(500, "Failed to save data");
                            }
                            else
                            {
                                cout << "       [SUCCESS] Deleted faculty: " << deletedName << "\n";
                                responsePayload = makeResponse(200, "application/json; charset=utf-8", "{\"ok\":true}");
                            }
                        }
                    }
                }
                else
                {
                    responsePayload = makeJsonErrorResponse(405, "Method not allowed");
                }
            }
        }
        else if (req.method == "POST" && pathOnly == "/api/faculty")
        {
            cout << "  [API] POST /api/faculty - Adding faculty\n";
            SessionInfo s;
            if (!getSessionInfo(req, s))
            {
                cout << "       [ERROR] Unauthorized\n";
                responsePayload = makeJsonErrorResponse(401, "Unauthorized");
            }
            else if (s.role != "admin")
            {
                cout << "       [ERROR] Not an admin (role: " << s.role << ")\n";
                responsePayload = makeJsonErrorResponse(403, "Access denied");
            }
            else
            {
                auto body = parseJsonObjectFlat(req.body);
                string name = getJsonValue(body, "name");
                string department = getJsonValue(body, "department");
                string designation = getJsonValue(body, "designation");
                string subject = getJsonValue(body, "subject");
                string email = getJsonValue(body, "email");
                string mobile = getJsonValue(body, "mobile");
                string password = getJsonValue(body, "password");

                if (name.empty() || department.empty() || designation.empty() || subject.empty() || email.empty() || mobile.empty() || password.empty())
                {
                    responsePayload = makeJsonErrorResponse(400, "Missing required fields");
                }
                else if (hasCsvUnsafeChars(name) || hasCsvUnsafeChars(department) || hasCsvUnsafeChars(designation) ||
                         hasCsvUnsafeChars(subject) || hasCsvUnsafeChars(email) || hasCsvUnsafeChars(mobile) || hasCsvUnsafeChars(password))
                {
                    responsePayload = makeJsonErrorResponse(400, "Invalid characters in input");
                }
                else
                {
                    vector<Faculty> list = loadData();

                    Faculty f;
                    f.id = nextFacultyId(list);
                    f.name = name;
                    f.dept = department;
                    f.desig = designation;
                    f.mobile = mobile;
                    f.email = email;
                    f.spec = subject;
                    f.pass = password;
                    f.role = "faculty";

                    list.push_back(f);
                    if (!saveData(list))
                    {
                        cout << "       [ERROR] Failed to save data\n";
                        responsePayload = makeJsonErrorResponse(500, "Failed to save data");
                    }
                    else
                    {
                        cout << "       [SUCCESS] New faculty - ID: " << f.id << ", Name: " << f.name << "\n";
                        responsePayload = makeResponse(201, "application/json; charset=utf-8", facultyToJson(f));
                    }
                }
            }
        }
        else if (req.method == "GET" && pathOnly == "/api/stats/summary")
        {
            cout << "  [API] /api/stats/summary - Fetching stats\n";
            SessionInfo s;
            if (!getSessionInfo(req, s))
            {
                cout << "       [ERROR] Not authenticated\n";
                responsePayload = makeJsonErrorResponse(401, "Unauthorized");
            }
            else
            {
                vector<Faculty> list = loadData();
                int facultyCount = 0;
                int studentCount = 0;
                map<string, int> departments;

                for (const auto &f : list)
                {
                    if (f.role == "faculty")
                    {
                        ++facultyCount;
                        departments[f.dept]++;
                    }
                    if (f.role == "student")
                        ++studentCount;
                }

                int recentlyAdded = min(5, facultyCount);
                stringstream bodyJson;
                bodyJson << "{";
                bodyJson << "\"totalFaculty\":" << facultyCount << ",";
                bodyJson << "\"totalDepartments\":" << static_cast<int>(departments.size()) << ",";
                bodyJson << "\"totalStudents\":" << studentCount << ",";
                bodyJson << "\"recentlyAdded\":" << recentlyAdded;
                bodyJson << "}";
                cout << "       Faculty: " << facultyCount << ", Depts: " << departments.size() << ", Students: " << studentCount << "\n";
                responsePayload = makeResponse(200, "application/json; charset=utf-8", bodyJson.str());
            }
        }
        else if (req.method == "GET" && pathOnly == "/api/stats/departments")
        {
            cout << "  [API] /api/stats/departments - Fetching departments\n";
            SessionInfo s;
            if (!getSessionInfo(req, s))
            {
                cout << "       [ERROR] Not authenticated\n";
                responsePayload = makeJsonErrorResponse(401, "Unauthorized");
            }
            else
            {
                vector<Faculty> list = loadData();
                map<string, int> counts;
                for (const auto &f : list)
                {
                    if (f.role == "faculty")
                        counts[f.dept]++;
                }

                stringstream bodyJson;
                bodyJson << "{\"departments\":[";
                bool first = true;
                for (const auto &kv : counts)
                {
                    if (!first)
                        bodyJson << ",";
                    first = false;
                    bodyJson << "{\"name\":\"" << jsonEscape(kv.first) << "\",\"count\":" << kv.second << "}";
                }
                bodyJson << "]}";
                cout << "       Found " << counts.size() << " departments\n";
                responsePayload = makeResponse(200, "application/json; charset=utf-8", bodyJson.str());
            }
        }
        else
        {
            responsePayload = makeJsonErrorResponse(404, "Route not found");
        }
    }
    else if (req.method == "GET")
    {
        string staticPath = pathOnly;
        if (staticPath == "/" || staticPath == "/index.html")
            staticPath = "/landing.html";

        if (staticPath.find("..") != string::npos)
        {
            responsePayload = makeResponse(400, "text/plain; charset=utf-8", "Bad path");
        }
        else
        {
            string relPath = staticPath.size() > 1 ? staticPath.substr(1) : "";
            if (relPath.empty())
                relPath = "login.html";

            string content = readFile(relPath);
            if (content.empty())
                responsePayload = makeResponse(404, "text/plain; charset=utf-8", "Not Found");
            else
                responsePayload = makeResponse(200, contentTypeForPath(relPath), content);
        }
    }
    else
    {
        responsePayload = makeResponse(404, "text/plain; charset=utf-8", "Route not found");
    }

    sendAll(clientSocket, responsePayload);
    cout << "[DISCONNECT] Connection closed\n\n";
    closesocket(clientSocket);
}

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == INVALID_SOCKET)
    {
        cerr << "socket() failed\n";
        WSACleanup();
        return 1;
    }

    BOOL opt = TRUE;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        cerr << "bind() failed\n";
        closesocket(serverFd);
        WSACleanup();
        return 1;
    }

    if (listen(serverFd, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "listen() failed\n";
        closesocket(serverFd);
        WSACleanup();
        return 1;
    }

    cout << "\n====================================\n";
    cout << "FACULTY MANAGEMENT SYSTEM - SERVER\n";
    cout << "Windows Socket (Winsock2)\n";
    cout << "====================================\n";
    cout << "Port: " << PORT << "\n";
    cout << "Find your IP: ipconfig\n";
    cout << "Access: http://<YOUR_IP>:" << PORT << "\n";
    cout << "====================================\n\n";

    while (true)
    {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientSocket == INVALID_SOCKET)
            continue;

        thread(handleClient, clientSocket).detach();
    }

    closesocket(serverFd);
    WSACleanup();
    return 0;
}
