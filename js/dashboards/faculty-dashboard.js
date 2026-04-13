// Faculty Dashboard Module

async function renderFacultyDashboard(user) {
  document.getElementById("topbarTitle").textContent = "My Dashboard";

  try {
    const [facultyData] = await Promise.all([api.listFaculty()]);
    let myRecord = null;
    let colleagues = [];

    if (user.facultyId) {
      myRecord = await api.getFaculty(user.facultyId);
      colleagues = facultyData.faculty.filter(
        f => f.department === myRecord.department && f.id !== myRecord.id
      );
    }

    document.getElementById("dashboardContent").innerHTML = `
      <div style="margin-bottom:20px;">
        <h2 style="font-size:22px;font-weight:700;">Welcome, ${esc(user.name)}</h2>
        <p style="color:var(--muted);font-size:14px;margin-top:2px;">Your faculty portal — view your profile and department colleagues.</p>
      </div>

      ${myRecord
        ? `
        <!-- Stats row -->
        <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:16px;margin-bottom:24px;">
          <div class="stat-card" style="border-top:3px solid var(--navy);">
            <div class="label">Department</div>
            <div class="value" style="font-size:20px;">${esc(myRecord.department.split(" ")[0])}</div>
            <div class="sub">${esc(myRecord.department)}</div>
          </div>
          <div class="stat-card" style="border-top:3px solid var(--gold);">
            <div class="label">Colleagues</div>
            <div class="value">${colleagues.length}</div>
            <div class="sub">In your department</div>
          </div>
          <div class="stat-card" style="border-top:3px solid #3a8a6a;">
            <div class="label">Experience</div>
            <div class="value">${myRecord.experience}</div>
            <div class="sub">Years of service</div>
          </div>
        </div>

        <!-- Department colleagues -->
        <div class="card">
          <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:16px;">
            <div style="font-size:15px;font-weight:700;color:var(--navy);">Department Colleagues — ${esc(myRecord.department)}</div>
            <a href="faculty.html" style="font-size:12px;color:var(--navy);text-decoration:none;font-weight:600;">View all faculty</a>
          </div>
          ${colleagues.length === 0
          ? renderEmptyState("&#128100;", "No colleagues yet", "No other faculty in your department.")
          : `<div class="table-wrap"><table>
            <thead>
              <tr>
                <th>Name</th>
                <th>Designation</th>
                <th>Subject</th>
                <th></th></tr></thead>
            <tbody>
              ${colleagues.slice(0, 5).map(f => `<tr>
                <td><strong>${esc(f.name)}</strong></td>
                <td>${esc(f.designation)}</td>
                <td>${esc(f.subject)}</td>
                <td>${esc(f.officeHours)}</td>
                <td><a href="faculty-detail.html?id=${f.id}" class="btn btn-outline btn-sm">View</a></td>
              </tr>`).join("")}
            </tbody>
          </table></div>
          ${colleagues.length > 5 ? `<div style="margin-top:12px;padding:12px;background:var(--light);border-radius:4px;font-size:13px;color:var(--muted);text-align:center;"><strong>${colleagues.length - 5}</strong> more colleagues in your department. <a href="faculty.html" style="color:var(--navy);font-weight:600;">View all faculty</a></div>` : ''}`
        }
        </div>
        `
        : renderEmptyState("&#128100;", "No faculty record linked", "Contact the administrator to link your account.")
      }
    `;
  } catch (err) {
    showAlert("alertBox", err.error || "Failed to load faculty dashboard");
  }
}
