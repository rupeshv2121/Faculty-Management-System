// Profile Controller - Main profile page logic

let user = null;

document.addEventListener("DOMContentLoaded", async () => {
  setActiveNav("nav-profile");
  user = await loadUser();
  window.currentUser = user; // Store for module access

  const content = document.getElementById("content");

  // Load based on role
  if (user.role === "faculty") {
    // Load faculty profile module
    try {
      await loadFacultyProfile(user);
    } catch (err) {
      console.error("Error loading faculty profile:", err);
      content.innerHTML = renderAccountCard(user) + renderNonFacultyCard(user.role);
    }
  } else {
    // Admin or student - show account info only
    content.innerHTML = renderAccountCard(user) + renderNonFacultyCard(user.role);
  }
});
