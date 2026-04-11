// Student Dashboard Module

async function renderStudentDashboard(user) {
  document.getElementById("topbarTitle").textContent = "Student Portal";

  try {
    const [stats, deptData, facultyData] = await Promise.all([
      api.stats(),
      api.departments(),
      api.listFaculty()
    ]);

    document.getElementById("dashboardContent").innerHTML = `
      <div style="margin-bottom:20px;">
        <h2 style="font-size:22px;font-weight:700;">Welcome, ${esc(user.name)}</h2>
        <p style="color:var(--muted);font-size:14px;margin-top:2px;">Find and search for faculty members and their contact information.</p>
      </div>

      <!-- Search (prominent) -->
      <div class="card" style="margin-bottom:24px;border-left:4px solid var(--gold);background:linear-gradient(135deg,#fff 80%,#fdf6ea);">
        <div style="font-size:15px;font-weight:700;color:var(--navy);margin-bottom:10px;">Search Faculty</div>
        <div style="display:flex;gap:10px;">
          <input class="search-input" id="studentSearch" style="max-width:100%;flex:1;" placeholder="Search by name, department, or subject..." onkeydown="if(event.key==='Enter')searchStudentFaculty()">
          <button class="btn btn-primary" onclick="searchStudentFaculty()">Search</button>
        </div>
        <div id="searchResults" style="margin-top:14px;"></div>
      </div>

      <!-- Stats -->
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-bottom:24px;" class="stat-grid">
        <div class="stat-card" style="border-top:3px solid var(--navy);">
          <div class="label">Total Faculty</div>
          <div class="value">${stats.totalFaculty}</div>
          <div class="sub">Available to contact</div>
        </div>
        <div class="stat-card" style="border-top:3px solid var(--gold);">
          <div class="label">Departments</div>
          <div class="value">${stats.totalDepartments}</div>
          <div class="sub">Academic departments</div>
        </div>
      </div>

      <!-- Browse by department -->
      <div style="margin-bottom:16px;">
        <div style="font-size:13px;font-weight:700;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);margin-bottom:12px;">Browse by Department</div>
        <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(160px,1fr));gap:12px;margin-bottom:24px;">
          ${deptData.departments
        .map(d => `
            <a href="faculty.html?search=${encodeURIComponent(d.name)}" class="dept-card">
              <div class="dept-count">${d.count}</div>
              <div class="dept-name">${esc(d.name)}</div>
              <div class="dept-label">faculty member${d.count !== 1 ? "s" : ""}</div>
            </a>
          `)
        .join("")}
        </div>
      </div>

      <!-- All Faculty Table -->
      <div class="card" style="padding:0;">
        <div style="padding:16px 20px 0;display:flex;align-items:center;justify-content:space-between;">
          <div style="font-size:15px;font-weight:700;color:var(--navy);">All Faculty</div>
          <span style="font-size:13px;color:var(--muted);">${facultyData.total} records</span>
        </div>
        <div class="table-wrap" style="margin-top:12px;">
          <table>
            <thead><tr><th>Name</th><th>Department</th><th>Subject</th><th>Office Hours</th><th>Email</th><th></th></tr></thead>
            <tbody>
              ${facultyData.faculty
        .map(f => `<tr>
                <td><strong>${esc(f.name)}</strong></td>
                <td><span class="badge badge-dept">${esc(f.department)}</span></td>
                <td>${esc(f.subject)}</td>
                <td style="font-size:12px;color:var(--muted);">${esc(f.officeHours)}</td>
                <td><a href="mailto:${esc(f.email)}" style="color:var(--navy);font-size:13px;">${esc(f.email)}</a></td>
                <td><a href="faculty-detail.html?id=${f.id}" class="btn btn-outline btn-sm">View</a></td>
              </tr>`)
        .join("")}
            </tbody>
          </table>
        </div>
      </div>
    `;
  } catch (err) {
    showAlert("alertBox", err.error || "Failed to load student dashboard");
  }
}

/**
 * Search faculty function for student dashboard
 */
async function searchStudentFaculty() {
  const q = document.getElementById("studentSearch").value.trim();
  if (!q) return;
  const resultsEl = document.getElementById("searchResults");
  resultsEl.innerHTML = '<div class="spinner" style="width:24px;height:24px;border-width:2px;margin:8px 0;"></div>';
  try {
    const { faculty, total } = await api.listFaculty(q);
    if (total === 0) {
      resultsEl.innerHTML = `<div style="color:var(--muted);font-size:13px;padding:8px 0;">No results for "${esc(q)}"</div>`;
      return;
    }
    resultsEl.innerHTML = `
      <div style="font-size:12px;color:var(--muted);margin-bottom:8px;">${total} result${total !== 1 ? "s" : ""} for "${esc(q)}"</div>
      ${faculty
        .map(f => `
        <div style="display:flex;align-items:center;gap:12px;padding:10px;background:var(--bg);border-radius:6px;margin-bottom:6px;">
          <div style="width:38px;height:38px;border-radius:50%;background:var(--navy);color:#222;border:1px solid var(--muted);display:flex;align-items:center;justify-content:center;font-weight:700;font-size:15px;flex-shrink:0;">${f.name.charAt(
          0
        )}</div>
          <div style="flex:1;">
            <div style="font-weight:600;font-size:14px;">${esc(f.name)}</div>
            <div style="font-size:12px;color:var(--muted);">${esc(f.designation)} &middot; ${esc(f.department)} &middot; ${esc(f.subject)}</div>
            <div style="font-size:12px;margin-top:2px;">&#9993; <a href="mailto:${esc(f.email)}" style="color:var(--navy);">${esc(f.email)}</a> &nbsp; &#128222; ${esc(f.mobile)}</div>
          </div>
          <a href="faculty-detail.html?id=${f.id}" class="btn btn-outline btn-sm">View</a>
        </div>
      `)
        .join("")}
    `;
  } catch {
    resultsEl.innerHTML = '<div style="color:var(--danger);font-size:13px;">Search failed. Please try again.</div>';
  }
}
