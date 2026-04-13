// Faculty detail page behavior extracted from inline script in faculty-detail.html

const params = new URLSearchParams(location.search);
const facultyId = params.get("id");
let user = null;
let faculty = null;
let editMode = false;

document.addEventListener("DOMContentLoaded", async () => {
  setActiveNav("nav-faculty");
  user = await loadUser();
  if (!facultyId) {
    document.getElementById("content").innerHTML =
      "<p>No faculty ID specified.</p>";
    return;
  }
  await loadFaculty();
});

async function loadFaculty() {
  try {
    faculty = await api.getFaculty(facultyId);
    // document.getElementById("pageTitle").textContent = faculty.name;
    renderView();
  } catch (err) {
    document.getElementById("content").innerHTML =
      `<div class="empty-state"><h3>Faculty not found</h3><p>${esc(err.error || "")}</p></div>`;
  }
}

function canEdit() {
  return (
    user.role === "admin" ||
    (user.role === "faculty" && user.facultyId === facultyId)
  );
}

function renderView() {
  editMode = false;
  const actions = document.getElementById("topbarActions");

  if (canEdit()) {
    actions.innerHTML =
      '<button class="btn btn-primary" onclick="renderEdit()">Edit</button>';
  } else {
    actions.innerHTML = "";
  }

  document.getElementById("content").innerHTML = `
    <div class="card detail-card">
      <div class="detail-header">
        <div class="detail-avatar">${faculty.name.charAt(0)}</div>
        <div>
          <h2 class="detail-name">${esc(faculty.name)}</h2>
          <div class="detail-subtitle">${esc(faculty.designation)} &middot; ${esc(faculty.department)}</div>
        </div>
      </div>

      <div class="section-title">Academic Information</div>
      <div class="detail-grid detail-grid-spaced">
        ${field("Department", faculty.department)}
        ${field("Designation", faculty.designation)}
        ${field("Subject", faculty.subject)}
        ${field("Qualification", faculty.qualification)}
        ${field("Experience", faculty.experience + " year(s)")}
      </div>

      <div class="section-title">Contact Information</div>
      <div class="detail-grid">
        ${field(
    "Email",
    `<a href="mailto:${esc(faculty.email)}" class="link-navy">${esc(faculty.email)}</a>`
  )}
        ${field("Mobile", faculty.mobile)}
      </div>
    </div>
  `;
}

function field(label, value) {
  return `<div class="detail-field">
    <div class="field-label">${label}</div>
    <div class="field-value">${value}</div>
  </div>`;
}

function renderEdit() {
  editMode = true;
  const isAdmin = user.role === "admin";
  const actions = document.getElementById("topbarActions");
  actions.innerHTML = `
    <button class="btn btn-outline" onclick="renderView()">Cancel</button>
    <button class="btn btn-primary" id="saveBtn" onclick="saveEdit()">Save Changes</button>
  `;

  document.getElementById("content").innerHTML = `
    <div class="card">
      <div id="formAlert" class="alert alert-error alert-hidden"></div>
      <div class="section-title">
        ${isAdmin ? "Edit All Fields" : "Update Contact Information"}
      </div>
      ${isAdmin
      ? `<div class="form-grid">
        <div class="form-group"><label>Full Name</label><input class="form-control" id="e-name" value="${esc(faculty.name)}"></div>
        <div class="form-group"><label>Department</label><input class="form-control" id="e-dept" value="${esc(faculty.department)}"></div>
        <div class="form-group"><label>Designation</label><input class="form-control" id="e-desig" value="${esc(faculty.designation)}"></div>
        <div class="form-group"><label>Subject</label><input class="form-control" id="e-subj" value="${esc(faculty.subject)}"></div>
        <div class="form-group"><label>Qualification</label><input class="form-control" id="e-qual" value="${esc(faculty.qualification)}"></div>
        <div class="form-group"><label>Experience (years)</label><input class="form-control" id="e-exp" type="number" min="0" max="50" value="${faculty.experience}"></div>
      </div>`
      : `<div class="edit-note">As a faculty member, you can only update your email and mobile number.</div>`
    }
      <div class="form-grid ${isAdmin ? "form-grid-top" : "form-grid-top-none"}">
        <div class="form-group"><label>Email</label><input class="form-control" id="e-email" type="email" value="${esc(faculty.email)}"></div>
        <div class="form-group"><label>Mobile</label><input class="form-control" id="e-mobile" value="${esc(faculty.mobile)}"></div>
      </div>
    </div>
  `;
}

async function saveEdit() {
  const btn = document.getElementById("saveBtn");
  const alertEl = document.getElementById("formAlert");
  alertEl.classList.add("alert-hidden");

  const isAdmin = user.role === "admin";
  const body = {
    email: document.getElementById("e-email").value.trim(),
    mobile: document.getElementById("e-mobile").value.trim(),
  };
  if (isAdmin) {
    body.name = document.getElementById("e-name").value.trim();
    body.department = document.getElementById("e-dept").value.trim();
    body.designation = document.getElementById("e-desig").value.trim();
    body.subject = document.getElementById("e-subj").value.trim();
    body.qualification = document.getElementById("e-qual").value.trim();
    body.experience = parseFloat(document.getElementById("e-exp").value) || 0;
  }

  btn.disabled = true;
  btn.textContent = "Saving...";
  try {
    faculty = await api.updateFaculty(facultyId, body);
    document.getElementById("pageTitle").textContent = faculty.name;
    renderView();
    showAlert("alertBox", "Faculty updated successfully!", "success");
  } catch (err) {
    alertEl.textContent = err.error || "Failed to update faculty";
    alertEl.classList.remove("alert-hidden");
  } finally {
    btn.disabled = false;
    btn.textContent = "Save Changes";
  }
}