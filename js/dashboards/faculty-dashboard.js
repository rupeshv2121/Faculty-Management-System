// Faculty Dashboard Module

async function renderFacultyDashboard(user) {
  document.getElementById("topbarTitle").textContent = "My Dashboard";

  // Add Edit My Info button before profile dropdown
  const profileContainer = document.getElementById("profileDropdownContainer");
  if (profileContainer && !document.getElementById("editInfoBtn")) {
    const btn = document.createElement("a");
    btn.id = "editInfoBtn";
    btn.href = "profile.html";
    btn.className = "btn btn-primary";
    btn.textContent = "Edit My Info";
    profileContainer.parentElement.insertBefore(btn, profileContainer);
  }

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
        <!-- My Profile Card -->
        <div class="card" style="margin-bottom:24px;border-left:4px solid var(--navy);">
          <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:16px;">
            <div style="font-size:15px;font-weight:700;color:black;">My Profile</div>
            <a href="profile.html" class="btn btn-outline btn-sm">Edit Contact Info</a>
          </div>
          <div style="display:flex;align-items:center;gap:16px;margin-bottom:16px;">
            <div style="width:56px;height:56px;border-radius:50%;border:1px solid var(--muted);background:var(--navy);color:#222;display:flex;align-items:center;justify-content:center;font-size:22px;font-weight:700;flex-shrink:0;">${myRecord.name.charAt(
          0
        )}</div>
            <div>
              <div style="font-size:18px;font-weight:700;">${esc(myRecord.name)}</div>
              <div style="font-size:13px;color:var(--muted);">${esc(myRecord.designation)} &middot; ${esc(
          myRecord.department
        )}</div>
            </div>
          </div>
          <div class="detail-grid" style="margin-bottom:14px;">
            <div class="detail-field"><div class="field-label">Subject</div><div class="field-value">${esc(myRecord.subject)}</div></div>
            <div class="detail-field"><div class="field-label">Qualification</div><div class="field-value">${esc(myRecord.qualification)}</div></div>
            <div class="detail-field"><div class="field-label">Experience</div><div class="field-value">${myRecord.experience} year(s)</div></div>
            <div class="detail-field"><div class="field-label">Office Hours</div><div class="field-value">${esc(myRecord.officeHours)}</div></div>
          </div>
          <div style="font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);margin-bottom:8px;">Contact (you can update these)</div>
          <div class="contact-info-row">
            <div class="contact-chip">&#9993; ${esc(myRecord.email)}</div>
            <div class="contact-chip">&#128222; ${esc(myRecord.mobile)}</div>
          </div>
        </div>

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
            <thead><tr><th>Name</th><th>Designation</th><th>Subject</th><th>Office Hours</th><th></th></tr></thead>
            <tbody>
              ${colleagues.map(f => `<tr>
                <td><strong>${esc(f.name)}</strong></td>
                <td>${esc(f.designation)}</td>
                <td>${esc(f.subject)}</td>
                <td>${esc(f.officeHours)}</td>
                <td><a href="faculty-detail.html?id=${f.id}" class="btn btn-outline btn-sm">View</a></td>
              </tr>`).join("")}
            </tbody>
          </table></div>`
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
