let rules = [];
const serviceOptions = [
  "Calibrate_Sensors",
  "Read_Light_Level",
  "Read_Temperature",
  "Set_Alarm_LED",
  "Set_Cooling_Indicator",
  "Trigger_Buzzer"
];

const nodeHost = document.getElementById("node-host");
const nodePort = document.getElementById("node-port");
const btnConnect = document.getElementById("btn-connect");
const statusPill = document.getElementById("node-status-pill");
const serviceList = document.getElementById("service-list");
const serviceCount = document.getElementById("service-count");
const rulesContainer = document.getElementById("rules-container");
const btnAddSingle = document.getElementById("btn-add-single");
const btnAddOrder = document.getElementById("btn-add-order");
const btnAddCond = document.getElementById("btn-add-condition");
const btnClear = document.getElementById("btn-clear-rules");
const btnRun = document.getElementById("btn-run-app");
const traceLog = document.getElementById("trace-log");
const btnClearTrace = document.getElementById("btn-clear-trace");

document.addEventListener("DOMContentLoaded", () => {
  scanServices();

  btnConnect.addEventListener("click", () => {
    updateNodes();
  });

  btnAddSingle.addEventListener("click", () => addSingleRule());
  btnAddOrder.addEventListener("click", () => addOrderRule());
  btnAddCond.addEventListener("click", () => addConditionRule());
  btnClear.addEventListener("click", () => {
    rules = [];
    renderRules();
  });

  btnRun.addEventListener("click", runApplication);
  btnClearTrace.addEventListener("click", () => {
    traceLog.innerHTML = `<div class="empty">Log cleared.</div>`;
  });

  renderRules();
});

async function scanServices() {
  try {
    const res = await fetch("/api/discovery");
    const data = await res.json();
    const services = data.services || [];
    const node = (data.nodes && data.nodes[0]) || {};

    if (node.status === "ONLINE") {
      statusPill.textContent = "Online";
      statusPill.className = "pill pill-online";
    } else {
      statusPill.textContent = "Offline";
      statusPill.className = "pill pill-offline";
    }

    if (services.length > 0) {
      services.forEach((s) => {
        if (!serviceOptions.includes(s.service_name)) {
          serviceOptions.push(s.service_name);
        }
      });
    }

    renderServices(services);
  } catch (e) {
    statusPill.textContent = "Offline";
    statusPill.className = "pill pill-offline";
  }
}

async function updateNodes() {
  const host = nodeHost.value.trim() || "127.0.0.1";
  const port = parseInt(nodePort.value.trim(), 10) || 8080;
  statusPill.textContent = "...";
  await fetch("/api/config_nodes", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ nodes: [{ rpi_id: "RPi-1", host, port }] })
  });
  await scanServices();
}

function renderServices(services) {
  serviceCount.textContent = `${services.length} found`;
  if (services.length === 0) {
    serviceList.innerHTML = `<div class="empty">No services detected. Start the RPi server.</div>`;
    return;
  }

  serviceList.innerHTML = "";
  services.forEach((s) => {
    const params = s.param_names && s.param_names.length > 0 ? s.param_names.join("/") : "None";
    const vector = `(${s.rpi_id}, ${s.service_name}, ${s.input_count}, ${params})`;
    const item = document.createElement("div");
    item.className = "vector-item";
    item.innerHTML = `
      <div class="vector-code">${vector}</div>
      <div class="vector-desc">${s.description}</div>
    `;
    serviceList.appendChild(item);
  });
}

function addSingleRule(srv = "Calibrate_Sensors", arg = "") {
  rules.push({
    id: "r_" + Date.now() + Math.random(),
    type: "single",
    srv,
    arg
  });
  renderRules();
}

function addOrderRule(srvA = "Calibrate_Sensors", argA = "", srvB = "Read_Temperature", argB = "C") {
  rules.push({
    id: "r_" + Date.now() + Math.random(),
    type: "order",
    srvA,
    argA,
    srvB,
    argB
  });
  renderRules();
}

function addConditionRule(srvA = "Read_Light_Level", op = "<", val = 30, srvB = "Set_Alarm_LED", argB = "1") {
  rules.push({
    id: "r_" + Date.now() + Math.random(),
    type: "condition",
    srvA,
    op,
    val,
    srvB,
    argB
  });
  renderRules();
}

function renderRules() {
  if (rules.length === 0) {
    rulesContainer.innerHTML = `<div class="empty">No rules added. Click "+ Single Service", "+ Order Rule", or "+ Condition Rule".</div>`;
    return;
  }

  rulesContainer.innerHTML = "";
  rules.forEach((r, i) => {
    const row = document.createElement("div");

    if (r.type === "single") {
      row.className = "rule-row single";
      row.innerHTML = `
        <span class="rule-tag">RUN</span>
        ${buildDropdown(r.id, "srv", r.srv)}
        <input type="text" placeholder="arg" value="${r.arg || ""}" style="width: 60px;" onchange="updateRule('${r.id}', 'arg', this.value)">
        <button class="btn-del" onclick="delRule('${r.id}')" title="Delete">×</button>
      `;
    } else if (r.type === "order") {
      row.className = "rule-row";
      row.innerHTML = `
        <span class="rule-tag">RUN</span>
        ${buildDropdown(r.id, "srvA", r.srvA)}
        <input type="text" placeholder="arg" value="${r.argA || ""}" style="width: 60px;" onchange="updateRule('${r.id}', 'argA', this.value)">
        <span class="rule-tag">THEN</span>
        ${buildDropdown(r.id, "srvB", r.srvB)}
        <input type="text" placeholder="arg" value="${r.argB || ""}" style="width: 60px;" onchange="updateRule('${r.id}', 'argB', this.value)">
        <button class="btn-del" onclick="delRule('${r.id}')" title="Delete">×</button>
      `;
    } else {
      row.className = "rule-row cond";
      row.innerHTML = `
        <span class="rule-tag">IF</span>
        ${buildDropdown(r.id, "srvA", r.srvA)}
        <select onchange="updateRule('${r.id}', 'op', this.value)">
          <option value="<" ${r.op === "<" ? "selected" : ""}>&lt;</option>
          <option value=">" ${r.op === ">" ? "selected" : ""}>&gt;</option>
          <option value="==" ${r.op === "==" ? "selected" : ""}>==</option>
          <option value="!=" ${r.op === "!=" ? "selected" : ""}>!=</option>
        </select>
        <input type="number" value="${r.val}" style="width: 70px;" onchange="updateRule('${r.id}', 'val', this.value)">
        <span class="rule-tag">DO</span>
        ${buildDropdown(r.id, "srvB", r.srvB)}
        <input type="text" placeholder="arg" value="${r.argB || ""}" style="width: 60px;" onchange="updateRule('${r.id}', 'argB', this.value)">
        <button class="btn-del" onclick="delRule('${r.id}')" title="Delete">×</button>
      `;
    }
    rulesContainer.appendChild(row);
  });
}

function buildDropdown(ruleId, field, currentVal) {
  let html = `<select onchange="updateRule('${ruleId}', '${field}', this.value)">`;
  serviceOptions.forEach((opt) => {
    html += `<option value="${opt}" ${opt === currentVal ? "selected" : ""}>${opt}</option>`;
  });
  html += `</select>`;
  return html;
}

function updateRule(id, field, val) {
  const r = rules.find((x) => x.id === id);
  if (r) r[field] = val;
}

function delRule(id) {
  rules = rules.filter((x) => x.id !== id);
  renderRules();
}

async function runApplication() {
  if (rules.length === 0) {
    alert("Add at least one rule first.");
    return;
  }

  const host = nodeHost.value.trim() || "127.0.0.1";
  const port = parseInt(nodePort.value.trim(), 10) || 8080;

  btnRun.disabled = true;
  btnRun.textContent = "Executing...";

  const payload = {
    rules: rules.map((r, i) => {
      if (r.type === "single") {
        const paramStr = r.arg || "";
        const descArg = paramStr ? `(${paramStr})` : "";
        return {
          type: "single",
          description: `Single Service #${i + 1}: ${r.srv}${descArg}`,
          service: { host, port, name: r.srv, param: paramStr },
          service_a: { host, port, name: r.srv, param: paramStr }
        };
      } else if (r.type === "order") {
        const paramAStr = r.argA || "";
        const paramBStr = r.argB || "";
        const descA = paramAStr ? `(${paramAStr})` : "";
        const descB = paramBStr ? `(${paramBStr})` : "";
        return {
          type: "order",
          description: `Order Rule #${i + 1}: ${r.srvA}${descA} -> ${r.srvB}${descB}`,
          service_a: { host, port, name: r.srvA, param: paramAStr },
          service_b: { host, port, name: r.srvB, param: paramBStr }
        };
      } else {
        return {
          type: "condition",
          description: `Condition Rule #${i + 1}: IF ${r.srvA} ${r.op} ${r.val} DO ${r.srvB}${r.argB ? `(${r.argB})` : ""}`,
          service_a: { host, port, name: r.srvA, param: "" },
          operator: r.op,
          threshold: parseFloat(r.val),
          service_b: { host, port, name: r.srvB, param: r.argB || "" }
        };
      }
    })
  };

  try {
    const res = await fetch("/api/execute", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    const result = await res.json();
    displayResults(result);
  } catch (e) {
    appendLog(`[ERROR] Failed to execute: ${e.message}`, "log-error");
  } finally {
    btnRun.disabled = false;
    btnRun.textContent = "▶ Run Application";
  }
}

function displayResults(data) {
  traceLog.innerHTML = "";
  (data.trace || []).forEach((step) => {
    let css = "log-success";
    if (step.status.includes("FAILED") || step.status.includes("ERROR")) css = "log-error";
    if (step.status.includes("SKIPPED")) css = "log-skip";

    let text = `[Rule ${step.step_index}] ${step.description}\n`;
    text += `Status: ${step.status}\n`;

    if (step.condition_eval) {
      const c = step.condition_eval;
      text += `Condition: ${c.value} ${c.operator} ${c.threshold} => ${c.met ? "TRUE (Executed)" : "FALSE (Skipped)"}\n`;
    }

    if (step.actions) {
      step.actions.forEach((a) => {
        let valStr = "";
        if (a.result && a.result.return_value !== undefined) {
          valStr = JSON.stringify(a.result.return_value);
        } else if (a.result && a.result.message) {
          valStr = `ERROR: ${a.result.message}`;
        } else {
          valStr = "undefined";
        }
        text += `-> Call ${a.service}(${a.input}) = ${valStr} (${(a.result && a.result.latency_ms) || 0}ms)\n`;
      });
    }

    if (step.error) {
      text += `-> Detail: ${step.error}\n`;
    }

    appendLog(text, css);
  });
}

function appendLog(text, css = "") {
  const line = document.createElement("div");
  line.className = `log-line ${css}`;
  line.style.whiteSpace = "pre-line";
  line.textContent = text;
  traceLog.appendChild(line);
}
