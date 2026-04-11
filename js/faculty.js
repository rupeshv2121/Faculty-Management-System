// Script for faculty.html moved from inline <script>
let user = null;

document.addEventListener("DOMContentLoaded", async () => {
  setActiveNav("nav-faculty");
  user = await loadUser();
  if (user.role === "admin") document.getElementById("addBtn").style.display = "inline-flex";

  document.getElementById("searchInput").addEventListener("keydown", e => {
    if (e.key === "Enter") loadFaculty();
  });

  await loadFaculty();
});

async function loadFaculty() {
  const search = document.getElementById("searchInput").value.trim();
  document.getElementById("facultyBody").innerHTML = '<tr><td colspan="7"><div class="spinner"></div></td></tr>';
  try {
    const { faculty, total } = await api.listFaculty(search || undefined);
    document.getElementById("totalBadge").textContent = total + " records";
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
      <td><strong>${esc(f.name)}</strong></td>
      <td><span class="badge badge-dept">${esc(f.department)}</span></td>
      <td>${esc(f.designation)}</td>
      <td>${esc(f.subject)}</td>
      <td><a href="mailto:${esc(f.email)}" style="color:var(--navy);">${esc(f.email)}</a></td>
      <td>${esc(f.mobile)}</td>
      <td style="white-space:nowrap;">${actions}</td>
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
    officeHours: document.getElementById("f-hours").value.trim(),
    qualification: document.getElementById("f-qual").value.trim(),
    experience: parseFloat(document.getElementById("f-exp").value) || 0,
    username: document.getElementById("f-user").value.trim(),
    password: document.getElementById("f-pass").value,
  };

  if (!body.name || !body.department || !body.designation || !body.subject ||
    !body.email || !body.mobile || !body.username || !body.password) {
    alertEl.textContent = "Please fill in all required fields.";
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
