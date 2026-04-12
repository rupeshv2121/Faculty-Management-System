let currentUser = null;

async function loadUser() {
    try {
        currentUser = await api.me();
        renderUserInfo(currentUser);
        return currentUser;
    } catch (e) {
        if (e.status === 401) {
            window.location.href = "/login.html";
        }
        throw e;
    }
}

function renderUserInfo(user) {
    // Store user to localStorage for profile dropdown
    localStorage.setItem("currentUser", JSON.stringify(user));

    const nameEl = document.getElementById("sidebar-name");
    const badgeEl = document.getElementById("sidebar-badge");
    if (nameEl) nameEl.textContent = user.name;
    if (badgeEl) {
        badgeEl.textContent = user.role.charAt(0).toUpperCase() + user.role.slice(1);
        badgeEl.className = "role-badge " + user.role;
    }

    // Update profile dropdown with user data
    setTimeout(() => {
        if (typeof updateProfileDropdown === 'function') {
            updateProfileDropdown();
        }
    }, 100);
}

function setActiveNav(id) {
    document.querySelectorAll(".nav-item").forEach(el => el.classList.remove("active"));
    const el = document.getElementById(id);
    if (el) el.classList.add("active");
}

function showAlert(containerId, msg, type = "error") {
    const el = document.getElementById(containerId);
    if (!el) return;
    el.className = "alert alert-" + type;
    el.textContent = msg;
    el.style.display = "flex";
    setTimeout(() => { el.style.display = "none"; }, 4000);
}

function hideAlert(containerId) {
    const el = document.getElementById(containerId);
    if (el) el.style.display = "none";
}

async function doLogout() {
    try { await api.logout(); } catch { }
    window.location.href = "/login.html";
}

// Hamburger is set up by setupHamburger() in common.js
// No need for duplicate setup here