// Global Shared Utilities
// Used across all modules

/**
 * Escape HTML special characters to prevent XSS attacks
 * Safely converts user data and text to HTML
 */
function esc(str) {
    const d = document.createElement("div");
    d.textContent = str ?? "";
    return d.innerHTML;
}

/**
 * Show alert message in a container
 * @param {string} containerId - ID of alert container element
 * @param {string} msg - Alert message text
 * @param {string} type - Alert type: 'error' or 'success' (default: 'error')
 */
function showAlert(containerId, msg, type = "error") {
    const el = document.getElementById(containerId);
    if (!el) return;
    el.className = "alert alert-" + type;
    el.textContent = msg;
    el.style.display = "flex";
    setTimeout(() => { el.style.display = "none"; }, 4000);
}

/**
 * Hide alert message in a container
 * @param {string} containerId - ID of alert container element
 */
function hideAlert(containerId) {
    const el = document.getElementById(containerId);
    if (el) el.style.display = "none";
}

/**
 * Set active navigation item
 * @param {string} id - ID of nav item to activate
 */
function setActiveNav(id) {
    document.querySelectorAll(".nav-item").forEach(el => el.classList.remove("active"));
    const el = document.getElementById(id);
    if (el) el.classList.add("active");
}

/**
 * Load current user and render their info in sidebar
 */
async function loadUser() {
    try {
        const currentUser = await api.me();
        renderUserInfo(currentUser);
        return currentUser;
    } catch (e) {
        if (e.status === 401) {
            window.location.href = "/login.html";
        }
        throw e;
    }
}

/**
 * Render user info in sidebar (legacy - kept for compatibility)
 * User info now displayed in profile dropdown
 * @param {object} user - User object with name and role
 */
function renderUserInfo(user) {
    // Store user in localStorage for profile dropdown
    localStorage.setItem('currentUser', JSON.stringify(user));
    // Update profile dropdown if it exists
    const dropdownName = document.getElementById('dropdownName');
    if (dropdownName) {
        dropdownName.textContent = user.name || "User";
    }
}

/**
 * Logout user and redirect to login page
 */
async function doLogout() {
    try { await api.logout(); } catch { }
    window.location.href = "/login.html";
}

/**
 * Toggle mobile sidebar
 */
function setupHamburger() {
    const hamburger = document.getElementById("hamburger");
    const sidebar = document.querySelector(".sidebar");
    if (hamburger && sidebar) {
        // Remove any existing listeners
        hamburger.onclick = null;
        hamburger.addEventListener("click", (e) => {
            e.stopPropagation();
            sidebar.classList.toggle("open");
        });
    }
}

// Also close sidebar when clicking outside
document.addEventListener("click", () => {
    const sidebar = document.querySelector(".sidebar");
    if (sidebar) {
        sidebar.classList.remove("open");
    }
});

// Setup hamburger on DOMContentLoaded as fallback
document.addEventListener("DOMContentLoaded", setupHamburger);

/**
 * Initialize profile dropdown
 */
function initProfileDropdown() {
    const profileBtn = document.getElementById("profileBtn");
    const profileDropdown = document.getElementById("profileDropdown");

    if (!profileBtn || !profileDropdown) return;

    // Toggle dropdown on button click
    profileBtn.addEventListener("click", (e) => {
        e.stopPropagation();
        profileDropdown.classList.toggle("open");
    });

    // Close dropdown when clicking outside
    document.addEventListener("click", (e) => {
        if (!e.target.closest(".profile-menu")) {
            profileDropdown.classList.remove("open");
        }
    });

    // Close dropdown when pressing Escape
    document.addEventListener("keydown", (e) => {
        if (e.key === "Escape") {
            profileDropdown.classList.remove("open");
        }
    });
}

/**
 * Update profile dropdown with user info
 */
function updateProfileDropdown() {
    try {
        const user = JSON.parse(localStorage.getItem("currentUser"));
        if (!user) return;

        // Update profile avatar initials
        const profileAvatar = document.getElementById("profileAvatar");
        const dropdownAvatar = document.getElementById("dropdownAvatar");
        const dropdownName = document.getElementById("dropdownName");
        const dropdownRole = document.getElementById("dropdownRole");

        const initials = (user.name || "U").split(" ")
            .map(n => n.charAt(0))
            .join("")
            .toUpperCase()
            .substring(0, 2);

        if (profileAvatar) profileAvatar.textContent = initials;
        if (dropdownAvatar) dropdownAvatar.textContent = initials;
        if (dropdownName) dropdownName.textContent = user.name || "User";
        if (dropdownRole) dropdownRole.textContent = user.role || "User";
    } catch (e) {
        // User data not available yet
    }
}
