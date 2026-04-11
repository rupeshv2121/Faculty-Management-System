# Faculty Management System

A modern, responsive web application for managing faculty information with role-based access control (Admin, Faculty, Student).

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

## 🛠 Tech Stack

### Frontend
- **HTML5** - Semantic markup
- **CSS3** - Flexbox, Grid, Media queries for responsiveness
- **Vanilla JavaScript** - No framework dependencies
- **Fetch API** - For HTTP requests

### Backend
- **C++** - Winsock2 for socket programming
- **HTTP/1.1** - REST API implementation
- **Session Management** - Cookie-based authentication

### Data Storage
- **Text File (CSV)** - User and faculty data in `users.txt`

## 📁 Project Structure

```
web_interface/
├── public/
│   ├── index.html          # Homepage
│   ├── landing.html        # Landing page
│   ├── login.html          # Login page
│   ├── dashboard.html      # Main dashboard
│   ├── faculty.html        # Faculty directory
│   ├── profile.html        # User profile
│   └── faculty-detail.html # Individual faculty details
│
├── components/
│   ├── sidebar.html        # Sidebar navigation (reusable)
│   └── profile-dropdown.html # Profile menu (reusable)
│
├── css/
│   ├── style.css           # Main stylesheet
│   ├── landing.css         # Landing page styles
│   └── login.css           # Login page styles
│
├── js/
│   ├── api.js              # API client
│   ├── auth.js             # Authentication utilities
│   ├── common.js           # Global utilities (hamburger, profile dropdown)
│   ├── dashboard.js        # Dashboard controller
│   ├── faculty.js          # Faculty page logic
│   ├── faculty-detail.js   # Faculty detail page logic
│   ├── landing.js          # Landing page logic
│   ├── login.js            # Login page logic
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
│
├── assets/
│   ├── logo.png            # Logo image
│   └── logo.ico            # Favicon
│
├── server.cpp              # C++ backend server
├── server.exe              # Compiled server executable
├── users.txt               # Faculty and user data (CSV format)
└── README.md               # This file
```

## 🚀 Getting Started

### Prerequisites
- Windows OS with Winsock2 support
- MinGW or Visual C++ compiler (to build server.cpp)
- Modern web browser

### Installation

1. **Compile the C++ Server** (if needed):
   ```bash
   g++ -std=gnu++20 server.cpp -o server.exe -lws2_32
   ```

2. **Run the Server**:
   ```bash
   server.exe
   ```
   - Server runs on `localhost:8080` by default

3. **Open in Browser**:
   ```
   http://localhost:8080
   ```

## 📖 Usage

### Demo Credentials

| Role | ID | Password |
|------|----|----------|
| Admin | 101 | admin123 |
| Faculty | 111 | mummy@123 |
| Student | 103 | n123 |

### Admin Dashboard
- View system statistics and faculty counts by department
- Quick action buttons for common tasks
- Add, view, update, or delete faculty records
- Search and filter capabilities

### Faculty Dashboard
- View personal dashboard with colleagues
- Update email and mobile number
- Access quick links to profile and settings

### Student Portal
- Search faculty by name, department, or subject
- View contact information
- Read-only access to faculty details

## 🔌 API Endpoints

### Authentication
- `POST /api/auth/login` - Login with username/password
- `POST /api/auth/logout` - Logout and clear session
- `GET /api/auth/me` - Get current user info

### Faculty
- `GET /api/faculty` - List all faculty with optional search
- `GET /api/faculty/{id}` - Get faculty details
- `POST /api/faculty` - Create new faculty (admin only)
- `PUT /api/faculty/{id}` - Update faculty (admin or self)
- `DELETE /api/faculty/{id}` - Delete faculty (admin only)

### Statistics
- `GET /api/stats/summary` - Get system statistics
- `GET /api/stats/departments` - Get faculty count by department

## 🎨 Design Features

### Color Scheme (Green Theme)
- **Primary Green**: `#2d7a5e`
- **Dark Green**: `#1f5543`
- **Light Green**: `#4a9f7d`
- **Accent**: `#7dd9a8`

### Responsive Breakpoints
- **Desktop**: 1025px+ (full layout with sidebar)
- **Tablet**: 768px - 1024px (sidebar toggles, grids collapse)
- **Mobile**: 480px - 767px (compact layout)
- **Phone**: <480px (minimal spacing, touch-optimized)

### Key Components
- **Hamburger Menu**: Toggles sidebar on mobile
- **Profile Dropdown**: User menu with logout
- **Search Bar**: Faculty search with live results
- **Data Tables**: Sortable, responsive tables
- **Charts**: Department faculty distribution

## 📝 User Data Format

`users.txt` (CSV format):
```
id,name,department,designation,mobile,email,subject,password,role
101,Rupesh,CSE,Professor,9999999999,r@mail.com,AI,admin123,admin
111,mummy,CSE,Professor,8888884897,m@gmail.com,AI,mummy@123,faculty
103,Neha,IT,Student,7777777777,n@mail.com,Data,n123,student
```

## 🔐 Security Features

- Session-based authentication with HTTP-only cookies
- Role-based access control (RBAC)
- CSRF protection
- HTML entity escaping for XSS prevention
- Input validation and sanitization
- CSV injection prevention

## 📱 Responsive Design Highlights

- **Mobile-First Approach**: Optimized for all screen sizes
- **Hamburger Navigation**: Hidden sidebar on mobile, toggleable
- **Flexible Grids**: Auto-collapsing layouts using CSS Grid
- **Optimized Tables**: Horizontal scroll on small screens
- **Touch-Friendly Buttons**: Proper sizing and spacing
- **Readable Typography**: Adaptive font sizes

## 🛠 Development

### Project Modularization
- Inline scripts extracted to separate modules
- One file per page controller
- Shared utilities in `common.js`
- Role-specific dashboards in `dashboards/`
- Reusable components: `sidebar.html`, `profile-dropdown.html`

### Global Utility Functions
| Function | Purpose |
|----------|---------|
| `esc()` | HTML entity escaping |
| `loadUser()` | Load current user |
| `renderUserInfo()` | Update user info |
| `setActiveNav()` | Set active nav item |
| `showAlert()` | Display alerts |
| `setupHamburger()` | Initialize hamburger menu |
| `initProfileDropdown()` | Initialize profile menu |
| `updateProfileDropdown()` | Update profile display |

### API Client Functions
Located in `js/api.js`:
- `api.login(username, password)`
- `api.logout()`
- `api.me()`
- `api.listFaculty(search)`
- `api.getFaculty(id)`
- `api.createFaculty(data)`
- `api.updateFaculty(id, data)`
- `api.deleteFaculty(id)`
- `api.stats()`
- `api.departments()`

## 🐛 Troubleshooting

### Server won't start
- Ensure port 8080 is not in use
- Check firewall settings
- Run with administrator privileges

### API requests failing
- Verify server is running on localhost:8080
- Check browser console for errors
- Ensure JSON is properly formatted

### Profile icon not showing
- Clear browser cache
- Check if `profile-dropdown.html` loads successfully
- Verify localStorage has user data

### Hamburger menu not working
- Hard refresh the page (Ctrl+Shift+R)
- Check browser console for JavaScript errors
- Ensure sidebar.html loads properly

## 📄 Files to Remove (Cleanup)

The following files can be safely removed:
- `MODULARIZATION.md` - Old documentation (use `README.md` instead)
- `cookies.txt` - Test file, not needed
- `server.exe` - Compiled output, rebuild from `server.cpp` if needed

## 🔄 Version History

### Current Version (v1.0)
- ✅ Complete faculty management system
- ✅ Role-based authentication
- ✅ Responsive design
- ✅ Modularized code
- ✅ Green theme UI
- ✅ Search and filter functionality
- ✅ Mobile hamburger menu
- ✅ Profile dropdown menu

## 📞 Support

For issues or questions:
1. Check the troubleshooting section
2. Review console logs in browser DevTools
3. Verify server is running with correct data

## 📜 License

Internal project for ZHCET Aligarh Faculty Management

## 👨‍💻 Contributors

- Developed as a comprehensive faculty management solution
- Built with vanilla JavaScript and C++ backend
- Responsive design for all devices

---

**Last Updated**: April 2026
**Status**: Production Ready ✅
