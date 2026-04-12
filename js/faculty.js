// Script for faculty.html moved from inline <script>
let user = null;
let searchTimeout;

document.addEventListener("DOMContentLoaded", async () => {
  setActiveNav("nav-faculty");
  user = await loadUser();

  // Show Add Faculty button only for admin
  const addBtn = document.getElementById("addBtn");
  if (user.role === "admin") {
    addBtn.style.display = "inline-flex";
  } else {
    addBtn.style.display = "none";
  }

  // Search on every character with debounce
  document.getElementById("searchInput").addEventListener("input", e => {
    clearTimeout(searchTimeout);
    searchTimeout = setTimeout(() => {
      loadFaculty();
    }, 300); // Debounce for 300ms
  });

  // Also allow Enter key for immediate search
  document.getElementById("searchInput").addEventListener("keydown", e => {
    if (e.key === "Enter") {
      clearTimeout(searchTimeout);
      loadFaculty();
    }
  });

  await loadFaculty();
});

async function loadFaculty() {
  const search = document.getElementById("searchInput").value.trim();
  document.getElementById("facultyBody").innerHTML = '<tr><td colspan="7"><div class="spinner"></div></td></tr>';
  try {
    const { faculty, total } = await api.listFaculty(search || undefined);
    // document.getElementById("totalBadge").textContent = total + " records";
    renderTable(faculty);
  } catch (err) {
    showAlert("alertBox", err.error || "Failed to load faculty");
  }
}

function renderTable(faculty) {
  const body = document.getElementById("facultyBody");
  if (faculty.length === 0) {
    body.innerHTML = `<tr><td colspan="7">
      <div class="empty-state">
        <div class="icon">&#128100;</div>
        <h3>No faculty found</h3>
        <p>Try a different search term${user.role === "admin" ? " or add a new record" : ""}.</p>
      </div>
    </td></tr>`;
    return;
  }

  body.innerHTML = faculty.map(f => {
    let actions = `<a href="faculty-detail.html?id=${f.id}" class="btn btn-outline btn-sm">View</a>`;
    if (user.role === "admin") {
      actions += ` <button class="btn btn-danger btn-sm" onclick="confirmDelete('${f.id}','${esc(f.name)}')">Delete</button>`;
    } else if (user.role === "faculty" && user.facultyId === f.id) {
      actions += ` <a href="faculty-detail.html?id=${f.id}" class="btn btn-gold btn-sm">Edit</a>`;
    }
    return `<tr>
      <td data-label="Name"><strong>${esc(f.name)}</strong></td>
      <td data-label="Department"><span class="badge badge-dept">${esc(f.department)}</span></td>
      <td data-label="Designation">${esc(f.designation)}</td>
      <td data-label="Subject">${esc(f.subject)}</td>
      <td data-label="Email"><a href="mailto:${esc(f.email)}" style="color:var(--navy);">${esc(f.email)}</a></td>
      <td data-label="Mobile">${esc(f.mobile)}</td>
      <td data-label="Actions" style="white-space:nowrap;">${actions}</td>
    </tr>`;
  }).join("");
}

function openAddModal() {
  document.getElementById("addModal").classList.remove("hidden");
  document.getElementById("modalAlert").style.display = "none";
}
function closeAddModal() { document.getElementById("addModal").classList.add("hidden"); }

async function saveFaculty() {
  const btn = document.getElementById("saveBtn");
  const alertEl = document.getElementById("modalAlert");
  alertEl.style.display = "none";

  const body = {
    name: document.getElementById("f-name").value.trim(),
    department: document.getElementById("f-dept").value.trim(),
    designation: document.getElementById("f-desig").value.trim(),
    subject: document.getElementById("f-subj").value.trim(),
    email: document.getElementById("f-email").value.trim(),
    mobile: document.getElementById("f-mobile").value.trim(),
    qualification: document.getElementById("f-qual").value.trim(),
    experience: parseFloat(document.getElementById("f-exp").value) || 0,
    username: document.getElementById("f-user").value.trim(),
    password: document.getElementById("f-pass").value,
  };

  // Validation checks
  if (!body.name || !body.department || !body.designation || !body.subject ||
    !body.email || !body.mobile || !body.username || !body.password) {
    alertEl.textContent = "Please fill in all required fields.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Full Name (letters, spaces, hyphens only, 2-50 chars)
  if (!/^[a-zA-Z\s\-']+$/.test(body.name) || body.name.length < 2 || body.name.length > 50) {
    alertEl.textContent = "Full Name must contain only letters and be 2-50 characters long.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Email format
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(body.email)) {
    alertEl.textContent = "Please enter a valid email address (e.g., name@domain.com).";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Mobile (digits only, exactly 10 digits)
  if (!/^\d+$/.test(body.mobile) || body.mobile.length !== 10) {
    alertEl.textContent = "Mobile number must contain only digits and be exactly 10 digits long.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Department (2-50 chars, letters and spaces)
  if (!/^[a-zA-Z\s\-]+$/.test(body.department) || body.department.length < 2 || body.department.length > 50) {
    alertEl.textContent = "Department must be 2-50 characters and contain only letters.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Designation (2-50 chars, letters and spaces)
  if (!/^[a-zA-Z\s\-]+$/.test(body.designation) || body.designation.length < 2 || body.designation.length > 50) {
    alertEl.textContent = "Designation must be 2-50 characters and contain only letters.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Subject (2-50 chars, alphanumeric and spaces)
  if (!/^[a-zA-Z0-9\s\-,;]+$/.test(body.subject) || body.subject.length < 2 || body.subject.length > 100) {
    alertEl.textContent = "Subject must be 2-100 characters and contain only letters, numbers, and basic punctuation.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Experience (non-negative number, max 60 years)
  if (isNaN(body.experience) || body.experience < 0 || body.experience > 60) {
    alertEl.textContent = "Experience must be a number between 0 and 60 years.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Qualification (optional, but if provided should be reasonable length)
  if (body.qualification && body.qualification.length > 100) {
    alertEl.textContent = "Qualification cannot exceed 100 characters.";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Username (3-20 chars, alphanumeric and underscore)
  if (!/^[a-zA-Z0-9_]{3,20}$/.test(body.username)) {
    alertEl.textContent = "Faculty ID or username must be 3-20 characters (letters, numbers, underscore only).";
    alertEl.style.display = "flex";
    return;
  }

  // Validate Password (minimum 6 characters)
  if (body.password.length < 6) {
    alertEl.textContent = "Password must be at least 6 characters long.";
    alertEl.style.display = "flex";
    return;
  }

  btn.disabled = true; btn.textContent = "Saving...";
  try {
    await api.createFaculty(body);
    closeAddModal();
    showAlert("alertBox", "Faculty added successfully!", "success");
    await loadFaculty();
  } catch (err) {
    alertEl.textContent = err.error || "Failed to save faculty";
    alertEl.style.display = "flex";
  } finally {
    btn.disabled = false; btn.textContent = "Save Faculty";
  }
}

async function confirmDelete(id, name) {
  if (!confirm(`Delete "${name}"? This cannot be undone.`)) return;
  try {
    await api.deleteFaculty(id);
    showAlert("alertBox", name + " deleted successfully.", "success");
    await loadFaculty();
  } catch (err) {
    showAlert("alertBox", err.error || "Failed to delete faculty");
  }
}

// esc() function is now in js/common.js - shared globally
