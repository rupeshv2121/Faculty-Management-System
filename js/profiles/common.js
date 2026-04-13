// Profile page utilities (common functions)
// esc() function is now in js/common.js - shared globally

/** Render account info card for all roles */
function renderAccountCard(user) {
  return `
    <div class="card" style="margin-bottom:20px;">
      <div style="display:flex;align-items:center;gap:16px;margin-bottom:20px;">
        <div style="width:64px;height:64px;border-radius:50%;background:var(--navy);color:#222;border:2px solid #f0f0f0;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:700;">
          ${user.name.charAt(0).toUpperCase()}
        </div>
        <div>
          <h2 style="font-size:18px;font-weight:700;">${esc(user.name)}</h2>
          <span class="role-badge ${user.role}" style="margin-top:4px;">${user.role.toUpperCase()}</span>
        </div>
      </div>
      <div class="detail-grid">
        <div class="detail-field"><div class="field-label">Faculty ID or Username</div><div class="field-value">${esc(user.username)}</div></div>
        <div class="detail-field"><div class="field-label">Role</div><div class="field-value">${esc(user.role)}</div></div>
      </div>
    </div>
  `;
}

/** Render empty state for non-faculty roles */
function renderNonFacultyCard(role) {
  const messages = {
    admin: { icon: "", title: "Administrator Account", desc: "You can manage all faculty records and view system analytics." },
    student: { icon: "", title: "Student Account", desc: "You can view and search the faculty directory from the sidebar." }
  };
  const msg = messages[role] || messages.student;
  return `
    <div class="card">
      <div class="empty-state">
        <div class="icon">${msg.icon}</div>
        <h3>${msg.title}</h3>
        <p>${msg.desc}</p>
      </div>
    </div>
  `;
}
