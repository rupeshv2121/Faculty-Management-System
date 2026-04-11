// Common utilities for all dashboard modules
// esc() function is now in js/common.js - shared globally

/**
 * Chart colors palette - Green theme
 */
const CHART_COLORS = ["#2d7a5e", "#4a9f7d", "#7dd9a8", "#1f5543", "#5a9d7d", "#22c55e"];

/**
 * Render a horizontal bar chart for department faculty count
 */
function renderDepartmentChart(departments, containerId, labelsId) {
  const max = Math.max(...departments.map(d => d.count), 1);
  const barsEl = document.getElementById(containerId);
  const labelsEl = document.getElementById(labelsId);

  if (!barsEl || !labelsEl) return;

  barsEl.innerHTML = "";
  labelsEl.innerHTML = "";

  departments.forEach((dept, i) => {
    const pct = (dept.count / max) * 100;
    const col = CHART_COLORS[i % CHART_COLORS.length];

    // Create bar
    const barGroup = document.createElement("div");
    barGroup.className = "chart-bar-group";
    barGroup.innerHTML = `
      <div class="chart-bar" style="height:${pct}%;background:${col};">
        <div class="tooltip">${dept.count}</div>
      </div>
      <div class="chart-label">${dept.name.split(" ")[0]}</div>
    `;
    barsEl.appendChild(barGroup);

    // Create legend label
    const label = document.createElement("div");
    label.style.cssText = "display:flex;align-items:center;gap:5px;font-size:11px;";
    label.innerHTML = `
      <span style="width:10px;height:10px;border-radius:2px;background:${col};display:inline-block;flex-shrink:0;"></span>
      ${dept.name} (${dept.count})
    `;
    labelsEl.appendChild(label);
  });
}

/**
 * Render empty state with icon and message
 */
function renderEmptyState(icon, heading, message) {
  return `
    <div class="empty-state" style="padding:40px;text-align:center;">
      <div class="icon" style="font-size:48px;margin-bottom:16px;">${icon}</div>
      <h3 style="margin-bottom:8px;">${heading}</h3>
      <p style="color:var(--muted);">${message}</p>
    </div>
  `;
}
