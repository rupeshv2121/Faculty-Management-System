// login page behavior extracted from inline script in login.html
// depends on existing js/api.js which provides `api` methods

// If already logged in, redirect
api
  .me()
  .then(() => (window.location.href = "/dashboard.html"))
  .catch(() => { });

document.getElementById("loginForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const btn = document.getElementById("loginBtn");
  const alert = document.getElementById("alertBox");
  if (alert) alert.style.display = "none";
  if (btn) {
    btn.disabled = true;
    btn.textContent = "Signing in...";
  }

  try {
    await api.login({
      username: document.getElementById("username").value.trim(),
      password: document.getElementById("password").value,
    });
    window.location.href = "/dashboard.html";
  } catch (err) {
    if (alert) {
      alert.textContent = err.error || "Invalid username or password";
      alert.style.display = "flex";
    }
    if (btn) {
      btn.disabled = false;
      btn.textContent = "Sign In";
    }
  }
});
