// Faculty Profile Module

let faculty = null;

/**
 * Load and display faculty profile
 */
async function loadFacultyProfile(user) {
  const content = document.getElementById("content");

  if (!user.facultyId) {
    content.innerHTML = renderAccountCard(user);
    return;
  }

  try {
    faculty = await api.getFaculty(user.facultyId);
    renderFacultyCard(false);
  } catch (err) {
    content.innerHTML =
      renderAccountCard(user) +
      `<div class="alert alert-error">Could not load faculty record: ${esc(err.error || "")}</div>`;
  }
}

/**
 * Render faculty profile view or edit mode
 */
function renderFacultyCard(editMode = false) {
  const content = document.getElementById("content");
  let html = renderAccountCard(window.currentUser);

  if (!editMode) {
    html += `
      <div class="card">
        <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:20px;">
          <div style="font-weight:700;color:var(--navy);font-size:15px;">My Faculty Record</div>
          <button class="btn btn-primary btn-sm" onclick="renderFacultyCard(true)">Edit Contact Info</button>
        </div>
        <div style="font-size:13px;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);font-weight:700;margin-bottom:12px;">Academic Information</div>
        <div class="detail-grid" style="margin-bottom:20px;">
          <div class="detail-field"><div class="field-label">Department</div><div class="field-value">${esc(faculty.department)}</div></div>
          <div class="detail-field"><div class="field-label">Designation</div><div class="field-value">${esc(faculty.designation)}</div></div>
          <div class="detail-field"><div class="field-label">Subject</div><div class="field-value">${esc(faculty.subject)}</div></div>
          <div class="detail-field"><div class="field-label">Qualification</div><div class="field-value">${esc(faculty.qualification)}</div></div>
          <div class="detail-field"><div class="field-label">Experience</div><div class="field-value">${faculty.experience} year(s)</div></div>
          <div class="detail-field"><div class="field-label">Office Hours</div><div class="field-value">${esc(faculty.officeHours)}</div></div>
        </div>
        <div style="font-size:13px;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);font-weight:700;margin-bottom:12px;">Contact Information</div>
        <div class="detail-grid">
          <div class="detail-field"><div class="field-label">Email</div><div class="field-value"><a href="mailto:${esc(faculty.email)}" style="color:var(--navy);">${esc(faculty.email)}</a></div></div>
          <div class="detail-field"><div class="field-label">Mobile</div><div class="field-value">${esc(faculty.mobile)}</div></div>
        </div>
      </div>
    `;
  } else {
    html += `
      <div class="card">
        <div style="font-weight:700;color:var(--navy);font-size:15px;margin-bottom:8px;">Update Contact Information</div>
        <p style="font-size:13px;color:var(--muted);margin-bottom:16px;">You can update your email and mobile number. For other changes, contact the administrator.</p>
        <div id="formAlert" class="alert alert-error" style="display:none;"></div>
        <div class="form-grid">
          <div class="form-group">
            <label>Email</label>
            <input class="form-control" id="p-email" type="email" value="${esc(faculty.email)}">
          </div>
          <div class="form-group">
            <label>Mobile</label>
            <input class="form-control" id="p-mobile" value="${esc(faculty.mobile)}">
          </div>
        </div>
        <div class="form-actions">
          <button class="btn btn-outline" onclick="renderFacultyCard(false)">Cancel</button>
          <button class="btn btn-primary" id="saveBtn" onclick="saveFacultyContact()">Save Changes</button>
        </div>
      </div>
    `;
  }

  content.innerHTML = html;
}

/**
 * Save faculty contact info
 */
async function saveFacultyContact() {
  const btn = document.getElementById("saveBtn");
  const alertEl = document.getElementById("formAlert");
  alertEl.style.display = "none";

  const body = {
    email: document.getElementById("p-email").value.trim(),
    mobile: document.getElementById("p-mobile").value.trim(),
  };

  if (!body.email || !body.mobile) {
    alertEl.textContent = "Email and mobile are required.";
    alertEl.style.display = "flex";
    return;
  }

  btn.disabled = true;
  btn.textContent = "Saving...";

  try {
    faculty = await api.updateFaculty(window.currentUser.facultyId, body);
    renderFacultyCard(false);
    showAlert("alertBox", "Contact information updated!", "success");
  } catch (err) {
    alertEl.textContent = err.error || "Update failed.";
    alertEl.style.display = "flex";
  } finally {
    btn.disabled = false;
    btn.textContent = "Save Changes";
  }
}
