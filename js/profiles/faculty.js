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
    // Fetch faculty data
    faculty = await api.getFaculty(user.facultyId);

    // Render the faculty card in view mode
    renderFacultyCard(false);
  } catch (err) {
    console.error("Faculty load error:", err);
    content.innerHTML =
      renderAccountCard(user) +
      `<div class="alert alert-error">Could not load faculty record: ${esc(err.error || "Unknown error")}</div>`;
  }
}

/**
 * Render faculty profile view or edit mode
 */
function renderFacultyCard(editMode = false) {
  const content = document.getElementById("content");
  const topbarActions = document.getElementById("topbarActions");
  let html = renderAccountCard(window.currentUser);

  // Manage Edit/Cancel buttons in topbar
  if (topbarActions) {
    let editBtn = document.getElementById("profileEditBtn");
    let cancelBtn = document.getElementById("profileCancelBtn");

    // Remove both buttons first
    if (editBtn) editBtn.remove();
    if (cancelBtn) cancelBtn.remove();

    if (!editMode) {
      // View Mode: Add Edit button
      editBtn = document.createElement("button");
      editBtn.id = "profileEditBtn";
      editBtn.className = "btn btn-primary";
      editBtn.style.marginRight = "12px";
      editBtn.textContent = "Edit";
      editBtn.onclick = () => renderFacultyCard(true);
      topbarActions.insertBefore(editBtn, topbarActions.firstChild);
    } else {
      // Edit Mode: Add Cancel button
      cancelBtn = document.createElement("button");
      cancelBtn.id = "profileCancelBtn";
      cancelBtn.className = "btn btn-outline";
      cancelBtn.style.marginRight = "8px";
      cancelBtn.textContent = "Cancel";
      cancelBtn.onclick = () => renderFacultyCard(false);
      topbarActions.insertBefore(cancelBtn, topbarActions.firstChild);
    }
  }

  if (!editMode) {
    // View Mode - Display faculty details
    html += `
      <div class="card detail-card">
        <div class="detail-header">
          <div class="detail-avatar" style="width:56px;height:56px;border-radius:50%;border:2px solid var(--navy);background:var(--navy);color:#fff;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:700;">${faculty.name.charAt(0)}</div>
          <div>
            <h2 style="font-size:24px;font-weight:700;margin:0;">${esc(faculty.name)}</h2>
            <div style="font-size:14px;color:var(--muted);margin-top:4px;">${esc(faculty.designation)} &middot; ${esc(faculty.department)}</div>
          </div>
        </div>

        <div style="font-size:13px;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);font-weight:700;margin:24px 0 16px 0;">Academic Information</div>
        <div class="detail-grid" style="margin-bottom:20px;">
          <div class="detail-field"><div class="field-label">Department</div><div class="field-value">${esc(faculty.department)}</div></div>
          <div class="detail-field"><div class="field-label">Designation</div><div class="field-value">${esc(faculty.designation)}</div></div>
          <div class="detail-field"><div class="field-label">Subject</div><div class="field-value">${esc(faculty.subject)}</div></div>
          <div class="detail-field"><div class="field-label">Qualification</div><div class="field-value">${esc(faculty.qualification)}</div></div>
          <div class="detail-field"><div class="field-label">Experience</div><div class="field-value">${faculty.experience} year(s)</div></div>
          <div class="detail-field"><div class="field-label">Office Hours</div><div class="field-value">${esc(faculty.officeHours)}</div></div>
        </div>

        <div style="font-size:13px;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);font-weight:700;margin:24px 0 16px 0;">Contact Information</div>
        <div class="detail-grid">
          <div class="detail-field"><div class="field-label">Email</div><div class="field-value"><a href="mailto:${esc(faculty.email)}" style="color:var(--navy);">${esc(faculty.email)}</a></div></div>
          <div class="detail-field"><div class="field-label">Mobile</div><div class="field-value">${esc(faculty.mobile)}</div></div>
        </div>
      </div>
    `;
  } else {
    // Edit Mode - Display edit form
    html += `
      <div class="card">
        <div id="formAlert" class="alert alert-error" style="display:none;"></div>
        <div style="font-size:13px;text-transform:uppercase;letter-spacing:.5px;color:var(--muted);font-weight:700;margin-bottom:12px;">Edit Contact Information</div>
        <p style="font-size:13px;color:var(--muted);margin-bottom:20px;">You can update your email and mobile number. For other changes, contact the administrator.</p>
        
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

  // Validate email format
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(body.email)) {
    alertEl.textContent = "Please enter a valid email address (e.g., name@example.com).";
    alertEl.style.display = "flex";
    return;
  }

  // Validate mobile format (basic check - must be numbers)
  if (!/^\d+$/.test(body.mobile) || body.mobile.length < 10) {
    alertEl.textContent = "Please enter a valid mobile number (at least 10 digits).";
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
