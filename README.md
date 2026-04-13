# Faculty Management System

A modern, responsive web application for managing faculty information with role-based access control (Admin, Faculty, Student). Built with C++ backend and vanilla JavaScript frontend, featuring session-based authentication and real-time data synchronization.

## 📋 Features

- **Role-Based Authentication**: Admin, Faculty, and Student roles with different permissions
- **Faculty Management**: 
  - Admin can create, read, update, and delete faculty records
  - Faculty can update their own information
  - Students have read-only access
- **Real-Time Statistics**: Dashboard with faculty counts by department
- **Responsive Design**: Works seamlessly on desktop, tablet, and mobile devices
- **Search & Filter**: Find faculty by name, department, or subject
- **Modern UI**: Green theme with smooth animations and transitions
- **Modular Architecture**: Component-based structure for easy maintenance
- **Data Validation**: Mobile (10 digits), Email, Name, Username validation

## 🛠 Tech Stack

### Frontend
- **HTML5** - Semantic markup
- **CSS3** - Flexbox, Grid, Media queries for responsiveness
- **Vanilla JavaScript** - No framework dependencies
- **Fetch API** - For HTTP requests with cookie-based sessions

### Backend
- **C++17 Standard** - Modern C++ features
- **Winsock2 (Windows)** - Socket programming for networking
- **Multi-threading** - std::thread for concurrent connections
- **Mutex Protection** - std::mutex for thread-safe operations
- **HTTP/1.1** - REST API implementation with proper headers
- **Session Management** - HttpOnly cookies with secure session tokens

### Data Storage
- **CSV File (users.txt)** - 10 fields per record with atomic operations
- **Format**: id, name, dept, desig, mobile, email, spec, pass, role, username

## 📁 Project Structure

```
web_interface/
├── public/
│   ├── index.html          # Homepage (redirects to dashboard/landing)
│   ├── landing.html        # Landing page with role showcase
│   ├── login.html          # Login page with demo credentials
│   ├── dashboard.html      # Main dashboard container
│   ├── faculty.html        # Faculty directory with search
│   ├── profile.html        # User profile page
│   └── faculty-detail.html # Individual faculty details & editing
│
├── components/
│   ├── sidebar.html        # Sidebar navigation (reusable)
│   └── profile-dropdown.html # Profile menu (reusable)
│
├── css/
│   ├── style.css           # Main stylesheet (responsive)
│   ├── landing.css         # Landing page styles
│   └── login.css           # Login page styles
│
├── js/
│   ├── api.js              # API client with fetch wrapper
│   ├── auth.js             # Authentication utilities
│   ├── common.js           # Global utilities (DOM, nav, alerts)
│   ├── dashboard.js        # Dashboard controller
│   ├── faculty.js          # Faculty page logic
│   ├── faculty-detail.js   # Faculty detail page logic
│   ├── landing.js          # Landing page interactivity
│   ├── login.js            # Login form handling
│   │
│   ├── dashboards/         # Role-specific dashboards
│   │   ├── common.js       # Shared dashboard utilities
│   │   ├── admin-dashboard.js
│   │   ├── faculty-dashboard.js
│   │   └── student-dashboard.js
│   │
│   └── profiles/           # Profile pages
│       ├── common.js       # Profile utilities
│       └── faculty.js      # Faculty profile logic
│       └── profile.js      # Profile logic
│
├── assets/
│   ├── logo.png            # Logo image
│   └── logo.ico            # Favicon
│
├── server.cpp              # C++ backend server (main implementation)
├── server.exe              # Compiled server executable
├── users.txt               # Faculty and user data (CSV format, 10 fields)
└── README.md               # This file
```

## 🚀 Getting Started

### Prerequisites
- **OS**: Windows (Winsock2 required)
- **Compiler**: MinGW with C++17 support or Visual C++
- **Browser**: Modern browser (Chrome, Firefox, Edge, Safari)
- **Port**: 8080 must be available

### Installation & Startup

1. **Navigate to project directory**:
   ```bash
   cd web_interface
   ```

2. **Compile the C++ Server** (if needed):
   ```bash
   g++ -std=c++17 -pthread -o server server.cpp -lws2_32
   ```

3. **Run the Server**:
   ```bash
   server.exe
   ```
   Expected output:
   ```
   ====================================
   FACULTY MANAGEMENT SYSTEM - SERVER
   Windows Socket (Winsock2)
   ====================================
   Port: 8080
   Server IP: 192.168.60.238
   Access: http://192.168.60.238:5000
   ====================================
   ```

4. **Open in Browser**:
   ```
   http://localhost:8080
   (or http://<YOUR_IP>:8080 for remote access)
   ```

## 📖 Usage

### Demo Credentials

**Updated for Latest Build:**

| Role | Username | Password |
|------|----------|----------|
| Admin | `admin` | `admin123` |
| Faculty | `prof_izhar` | `default123` |
| Faculty | `prof_nesar` | `default123` |
| Student | `neha` | `neha@123` |

### Admin Dashboard
-  View system statistics (faculty count, departments, students)
-  Add new faculty records with username and password
-  Update any faculty information
-  Delete faculty records
-  Search and filter capabilities
-  Department-wise breakdown

### Faculty Dashboard
-  View personal profile information
-  Update email and mobile number only
-  Browse department colleagues
-  Access full faculty directory
-  Search faculty members

### Student Portal
-  Search faculty by name, department, or subject
-  View contact information
-  Browse faculty details
-  Filter by department
-  Read-only access to all records

## API Endpoints

All API endpoints require active session cookie.

### Authentication
- `POST /api/auth/login` 
  - Body: `{username: string, password: string}`
  - Returns: User object + session cookie
- `POST /api/auth/logout` 
  - Clears session and returns ok
- `GET /api/auth/me` 
  - Returns: Current authenticated user

### Faculty Management
- `GET /api/faculty` 
  - Query: `?search=term` (optional)
  - Returns: Array of faculty + total count
- `GET /api/faculty/{id}` 
  - Returns: Single faculty object
- `POST /api/faculty` 
  - **Admin only**
  - Body: `{name, department, designation, subject, email, mobile, password, username}`
  - Returns: Created faculty object
- `PUT /api/faculty/{id}` 
  - **Admin or self (faculty)**
  - Body: `{email?, mobile?, name?, department?, ...}`
  - Returns: Updated faculty object
- `DELETE /api/faculty/{id}` 
  - **Admin only**
  - Returns: `{ok: true}`

### Statistics
- `GET /api/stats/summary` 
  - Returns: `{totalFaculty, totalDepartments, totalStudents, recentlyAdded}`
- `GET /api/stats/departments` 
  - Returns: `{departments: [{name, count}, ...]}`

## Data Format & Validation

### User Data (users.txt - CSV Format)

**Fields (10 total):**
```
id, name, department, designation, mobile, email, subject, password, role, username
```

**Field Validation:**
| Field | Type | Constraints | Example |
|-------|------|-------------|---------|
| id | string | Numeric, auto-incremented | `1`, `101` |
| name | string | 2-50 chars, letters only | `Prof. Izharuddin` |
| department | string | 2-50 chars, letters + spaces | `Computer Engineering` |
| designation | string | 2-50 chars, letters + spaces | `Professor` |
| mobile | string | **Exactly 10 digits** ✓ | `9412545786` |
| email | string | Valid email format | `izhar@zhcet.ac.in` |
| subject | string | 2-100 chars, alphanumeric | `VLSI Design; Signal Processing` |
| password | string | Minimum 6 characters | `default123` |
| role | string | `admin`, `faculty`, `student` | `faculty` |
| username | string | 3-20 chars, alphanumeric+underscore | `prof_izhar` |

**Example Record:**
```
1,Prof. Izharuddin,Computer Engineering,Professor,9412545786,izharuddin@zhcet.ac.in,VLSI Design; Signal Processing; Signal Security,default123,faculty,prof_izhar
```

## Design Features

### Color Scheme (Green Theme)
- **Primary Green**: `#2d7a5e` (main buttons, active states)
- **Dark Green**: `#1f5543` (headers, dark elements)
- **Light Green**: `#4a9f7d` (hover states, accents)
- **Accent**: `#7dd9a8` (success states, highlights)
- **Neutral**: `#f5f5f5` (backgrounds), `#333` (text)

### Responsive Breakpoints
- **Desktop**: 1025px+ (full layout with persistent sidebar)
- **Tablet**: 768px - 1024px (sidebar toggles, multi-column grids)
- **Mobile**: 480px - 767px (single column, compact spacing)
- **Phone**: <480px (minimal layout, touch-optimized)

### Key UI Components
- **Hamburger Menu**: Toggles sidebar on screens <1025px
- **Profile Dropdown**: User menu with logout (top-right)
- **Search Bar**: Faculty search with live filtering
- **Data Tables**: Responsive, horizontal scroll on mobile
- **Modal Forms**: Add/edit faculty with validation
- **Alert System**: Success, error, warning messages
- **Dashboard Cards**: Statistics and quick actions

## Security Features

-  **Session Authentication**: HttpOnly cookies, 30-min timeout
-  **Role-Based Access Control (RBAC)**: Admin/Faculty/Student enforcement
-  **CSRF Protection**: Session validation on state-changing requests
-  **XSS Prevention**: HTML entity escaping, JSON sanitization
-  **Input Validation**: Type checking, length limits, pattern matching
-  **CSV Injection Prevention**: Character validation, escaping
-  **Thread Safety**: Mutex-protected concurrent operations
-  **Error Handling**: Graceful failures, informative messages

## Backend Architecture

### C++ Server Features
- **Threading Model**: One thread per client connection
- **Session Management**: In-memory map with mutex protection
- **HTTP Parsing**: Full HTTP/1.1 request parsing
- **JSON Handling**: Custom flat JSON object parser
- **File I/O**: Atomic read/write with file locking
- **Logging**: Console output for all API operations with client IP
- **Performance**: Non-blocking socket operations

### Thread Safety
```cpp
map<string, SessionInfo> sessions;     // In-memory sessions
mutex sessionsMutex;                    // Protects sessions map
mutex fileMutex;                        // Protects file operations

// Usage pattern:
{
    lock_guard<mutex> lock(sessionsMutex);
    sessions[sid] = SessionInfo{role, userId};
}
```

## JavaScript Architecture

### Module Organization
- **api.js**: Centralized API client with error handling
- **auth.js**: Authentication utilities and checks
- **common.js**: Global DOM utilities, navigation, alerts
- **dashboard.js**: Dashboard controller with data loading
- **faculty.js**: Faculty listing, search, CRUD operations
- **dashboards/**: Role-specific dashboard implementations
- **profiles/**: User profile management

### Data Flow
```
User Input → Validation → API Call → Server → Response → UI Update
                ↓
           Error Display ← Error Handling
```

## File Summaries

### Backend
#### **server.cpp** (Main Application)
- **Lines**: ~1,300
- **Purpose**: Windows Winsock2 HTTP server with REST API
- **Key Functions**:
  - `main()`: Server initialization and socket setup
  - `handleClient()`: Per-client request handler with logging
  - `parseHttpRequest()`: HTTP request parsing
  - `authenticateUser()`: Login with ID/Username support
  - `loadData()`: CSV file parsing with mutex protection
  - `saveData()`: Atomic file write operations
  - `facultyToJson()`: Faculty object to JSON serialization
  - Various API handlers: `/api/auth/*`, `/api/faculty/*`, `/api/stats/*`
- **Features**: Thread-safe, non-blocking, comprehensive logging

### Frontend - HTML
#### **login.html**
- **Purpose**: User authentication page
- **Demo Credentials**: Admin, Faculty, Student samples
- **Features**: Form validation, error display, back button

#### **landing.html**
- **Purpose**: Home page with role showcase and feature highlights
- **Sections**: Hero, Features, Roles, How It Works, Tech Stack
- **Demo Credentials**: Reveal buttons for each role

#### **dashboard.html**
- **Purpose**: Main dashboard container
- **Dynamic Content**: Loads role-specific dashboard (admin/faculty/student)
- **Components**: Sidebar, top-bar, main content area

#### **faculty.html**
- **Purpose**: Faculty directory with search and filtering
- **Features**: Search by name/dept/subject, admin add/delete buttons
- **Displays**: Faculty name, dept, designation, email, mobile

#### **public/profile.html**
- **Purpose**: User profile and settings
- **Features**: View profile, change password, logout

#### **public/faculty-detail.html**
- **Purpose**: Individual faculty view and edit page
- **Role-Based**: Admin can edit all fields, faculty can edit own email/mobile

### Frontend - CSS
#### **css/style.css** (1,500+ lines)
- **Green Theme**: Primary colors, consistent branding
- **Responsive Grid**: Mobile-first design
- **Components**: Buttons, forms, cards, tables, modals, alerts
- **Animations**: Smooth transitions, hover effects
- **Breakpoints**: 480px, 768px, 1025px

#### **css/landing.css**
- Landing page specific styles (hero, sections, role cards)

#### **css/login.css**
- Login page styles (centered box, form layout)

### Frontend - JavaScript
#### **js/api.js** (30 lines)
- **Purpose**: Centralized API client
- **Methods**: login, logout, me, listFaculty, getFaculty, createFaculty, updateFaculty, deleteFaculty
- **Pattern**: All requests use fetch with credentials: 'include'

#### **js/auth.js** (50 lines)
- **Purpose**: Authentication utilities
- **Functions**: loadUser(), isAuthenticated(), hasRole()

#### **js/common.js** (150 lines)
- **Purpose**: Global utilities
- **Functions**: esc() (HTML escape), trim(), loadUser(), setActiveNav(), showAlert(), setupHamburger(), initProfileDropdown()

#### **js/login.js** (30 lines)
- **Purpose**: Login form handler
- **Features**: Form submission, error display, redirect on success

#### **js/faculty.js** (200 lines)
- **Purpose**: Faculty directory controller
- **Features**: Load, render, search, add, delete faculty
- **Validation**: Mobile (exactly 10), email, name, username patterns

#### **js/faculty-detail.js** (150 lines)
- **Purpose**: Individual faculty page
- **Features**: Load faculty, display editable form (role-based)
- **Updates**: Save changes via PUT, handle validation

#### **js/dashboard.js** (50 lines)
- **Purpose**: Dashboard container controller
- **Features**: Load role-specific dashboard, user authentication check

#### **js/landing.js** (100 lines)
- **Purpose**: Landing page interactivity
- **Features**: Credential reveal, smooth scrolling, role tabs

#### **js/dashboards/admin-dashboard.js** (200 lines)
- **Purpose**: Admin-only dashboard
- **Features**: Statistics, quick actions, recent faculty, department charts

#### **js/dashboards/faculty-dashboard.js** (150 lines)
- **Purpose**: Faculty-only dashboard
- **Features**: Personal stats, department colleagues, profile link

#### **js/dashboards/student-dashboard.js** (100 lines)
- **Purpose**: Student portal
- **Features**: Faculty search, browse by department

#### **js/profiles/faculty.js** (180 lines)
- **Purpose**: Faculty profile editing
- **Features**: Display profile, edit email/mobile, save changes
- **Validation**: Email format, mobile (exactly 10 digits)

### Data
#### **users.txt** (CSV Format)
- **Records**: 35+ faculty and admin accounts
- **Fields**: id, name, department, designation, mobile, email, subject, password, role, username
- **Format**: Comma-separated with atomic write protection
- **Features**: Supports role-based filtering and search

#### **components/sidebar.html**
- Navigation menu (role-aware links)

#### **components/profile-dropdown.html**
- User menu with logout

## 📜 License

Internal project for ZHCET Aligarh Faculty Management

## 👨‍💻 Contributors

- Developed as a comprehensive faculty management solution
- Built with vanilla JavaScript and C++ backend
- Responsive design for all devices

---