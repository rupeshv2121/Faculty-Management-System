// Extracted JS from landing.html
const creds = {
  admin: { p1: "admin123" },
  faculty: { p1: "fac123", p2: "fac123" },
  student: { p1: "stu123", p2: "stu123" },
};
const state = { admin: false, faculty: false, student: false };

function toggleCred(role) {
  state[role] = !state[role];
  const show = state[role];
  const btn = document.getElementById(role + "-btn");
  if (btn) btn.textContent = show ? "Hide" : "Reveal";
  const data = creds[role] || {};
  if (data.p1) {
    const el = document.getElementById(role + "-p1");
    if (el) el.textContent = show ? data.p1 : "••••••" + (data.p1.length > 6 ? "••" : "");
  }
  if (data.p2) {
    const el2 = document.getElementById(role + "-p2");
    if (el2) el2.textContent = show ? data.p2 : "••••••";
  }
}

// Export for potential module consumers (no module bundler here)
window.landingCreds = { toggleCred };
