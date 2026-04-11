/**
 * Main Dashboard Controller
 * Imports and orchestrates role-specific dashboard modules
 */

let user = null;

document.addEventListener("DOMContentLoaded", async () => {
  setActiveNav("nav-dashboard");

  try {
    user = await loadUser();
  } catch (err) {
    console.error("Failed to load user:", err);
    document.getElementById("dashboardContent").innerHTML = `
      <div style="padding: 40px 20px; text-align: center; color: var(--muted);">
        <p style="font-size: 16px; margin-bottom: 10px;">Unable to load dashboard</p>
        <p style="font-size: 14px;">Backend server may not be running. Please start the server.</p>
        <p style="font-size: 12px; margin-top: 20px; color: var(--text);">Error: ${err.message || 'Unknown error'}</p>
      </div>
    `;
    return;
  }

  // Route to appropriate dashboard based on user role
  switch (user.role) {
    case "admin":
      await renderAdminDashboard(user);
      break;
    case "faculty":
      await renderFacultyDashboard(user);
      break;
    case "student":
      await renderStudentDashboard(user);
      break;
    default:
      showAlert("alertBox", "Unknown user role");
  }
});
