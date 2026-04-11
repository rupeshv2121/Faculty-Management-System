const API_BASE = "/api";

async function apiFetch(method, path, body = null) {
    const opts = {
        method,
        credentials: "include",
        headers: { "Content-Type": "application/json" },
    };
    if (body) opts.body = JSON.stringify(body);
    const res = await fetch(API_BASE + path, opts);
    let data;
    try { data = await res.json(); } catch { data = {}; }
    if (!res.ok) throw { status: res.status, error: data.error || "Request failed" };
    return data;
}

const api = {
    // Auth
    login: (body) => apiFetch("POST", "/auth/login", body),
    logout: () => apiFetch("POST", "/auth/logout"),
    me: () => apiFetch("GET", "/auth/me"),

    // Faculty
    listFaculty: (search) => apiFetch("GET", "/faculty" + (search ? `?search=${encodeURIComponent(search)}` : "")),
    getFaculty: (id) => apiFetch("GET", `/faculty/${id}`),
    createFaculty: (body) => apiFetch("POST", "/faculty", body),
    updateFaculty: (id, b) => apiFetch("PUT", `/faculty/${id}`, b),
    deleteFaculty: (id) => apiFetch("DELETE", `/faculty/${id}`),

    // Stats
    stats: () => apiFetch("GET", "/stats/summary"),
    departments: () => apiFetch("GET", "/stats/departments"),
};
