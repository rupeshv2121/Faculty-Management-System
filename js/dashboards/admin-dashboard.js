// Admin Dashboard Module

async function renderAdminDashboard(user) {
  document.getElementById("topbarTitle").textContent = "Admin Dashboard";

  try {
    const [stats, deptData, facultyData] = await Promise.all([
      api.stats(),
      api.departments(),
      api.listFaculty()
    ]);

    // Main dashboard content
    document.getElementById("dashboardContent").innerHTML = `
      <!-- Welcome -->
      <div style="margin-bottom:20px;">
        <h2 style="font-size:22px;font-weight:700;">Welcome back, ${esc(user.name)}</h2>
        <p style="color:var(--muted);font-size:14px;margin-top:2px;">Here is the system overview for today.</p>
      </div>

      <!-- Stats -->
      <div class="stat-grid" style="margin-bottom:24px;">
        <div class="stat-card" style="border-top:3px solid var(--navy);">
          <div class="label">Total Faculty</div>
          <div class="value">${stats.totalFaculty}</div>
          <div class="sub">Active members</div>
        </div>
        <div class="stat-card" style="border-top:3px solid var(--gold);">
          <div class="label">Departments</div>
          <div class="value">${stats.totalDepartments}</div>
          <div class="sub">Academic units</div>
        </div>
        <div class="stat-card" style="border-top:3px solid #3a8a6a;">
          <div class="label">Students</div>
          <div class="value">${stats.totalStudents}</div>
          <div class="sub">Registered accounts</div>
        </div>
        <div class="stat-card" style="border-top:3px solid #6a4a9a;">
          <div class="label">Recently Added</div>
          <div class="value">${stats.recentlyAdded}</div>
          <div class="sub">New entries</div>
        </div>
      </div>

      <!-- Quick Actions -->
      <div style="margin-bottom:24px;">
        <div style="font-size:13px;font-weight:700;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);margin-bottom:12px;">Quick Actions</div>
        <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));gap:12px;">
          <a href="faculty.html" class="quick-action">
            <span class="qa-icon"><i class="fa-solid fa-plus"></i></span>
            <span class="qa-label">Add Faculty</span>
            <span class="qa-sub">Create new record</span>
          </a>
          <a href="faculty.html" class="quick-action">
            <span class="qa-icon"><i class="fa-solid fa-users"></i></span>
            <span class="qa-label">All Faculty</span>
            <span class="qa-sub">Browse directory</span>
          </a>
          <a href="faculty.html?search=" class="quick-action">
            <span class="qa-icon"><i class="fa-solid fa-search"></i></span>
            <span class="qa-label">Search</span>
            <span class="qa-sub">Find by name/dept</span>
          </a>
          <a href="profile.html" class="quick-action">
            <span class="qa-icon"><i class="fa-solid fa-cog"></i></span>
            <span class="qa-label">Settings</span>
            <span class="qa-sub">Account info</span>
          </a>
        </div>
      </div>

      <!-- Charts row -->
      <div style="display:grid;grid-template-columns:1fr;gap:20px;margin-bottom:24px;" id="chartsRow" class="charts-row-responsive">
        <div class="card">
          <div style="font-size:15px;font-weight:700;color:var(--navy);margin-bottom:16px;">Faculty by Department</div>
          <div class="chart-bars" id="chartBars"></div>
          <div style="display:flex;gap:10px;margin-top:4px;flex-wrap:wrap;" id="chartLabels"></div>
        </div>
        <div class="card">
          <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:12px;">
            <div style="font-size:15px;font-weight:700;color:var(--navy);">Recent Faculty</div>
            <a href="faculty.html" style="font-size:12px;color:var(--navy);text-decoration:none;font-weight:600; ">View all</a>
          </div>
          <div id="recentList"></div>
        </div>
      </div>
    `;

    // Render department chart
    renderDepartmentChart(deptData.departments, "chartBars", "chartLabels");

    // Render recent faculty list
    const recentList = document.getElementById("recentList");
    if (recentList) {
      recentList.innerHTML = facultyData.faculty.slice(0, 6).map(f => `
        <div style="display:flex;align-items:center;gap:10px;padding:9px 0;border-bottom:1px solid var(--border);">
          <div style="width:32px;height:32px;border-radius:50%; border:1px solid var(--muted);background:var(--navy);color:#000;display:flex;align-items:center;justify-content:center;font-weight:700;font-size:13px;flex-shrink:0;">${f.name.charAt(0)}</div>
          <div style="flex:1;overflow:hidden;">
            <div style="font-weight:600;font-size:13px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">${esc(f.name)}</div>
            <div style="font-size:11px;color:var(--muted);">${esc(f.department)}</div>
          </div>
          <a href="faculty-detail.html?id=${f.id}" class="btn btn-outline btn-sm">View</a>
        </div>
      `).join("");
    }

  } catch (err) {
    showAlert("alertBox", err.error || "Failed to load admin dashboard");
  }
}
