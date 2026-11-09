let rules = [];

// Logical microservice catalog partitioned across RPi-1 and RPi-2
let serviceOptions = [
  { rpi_id: "RPi-1", name: "Read_Temperature", label: "[RPi-1] Read_Temperature (Dial)" },
  { rpi_id: "RPi-1", name: "Set_Cooling_Indicator", label: "[RPi-1] Set_Cooling_Indicator (Green LED)" },
  { rpi_id: "RPi-1", name: "Calibrate_Sensors", label: "[RPi-1] Calibrate_Sensors (Green LED Flash)" },
  { rpi_id: "RPi-2", name: "Read_Light_Level", label: "[RPi-2] Read_Light_Level (LDR)" },
  { rpi_id: "RPi-2", name: "Set_Alarm_LED", label: "[RPi-2] Set_Alarm_LED (Red LED)" },
  { rpi_id: "RPi-2", name: "Calibrate_Sensors", label: "[RPi-2] Calibrate_Sensors (Red LED Flash)" }
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
const errorBox = document.getElementById("composer-error-box");
const errorTitle = document.getElementById("error-title");
const errorDesc = document.getElementById("error-desc");
const btnCloseError = document.getElementById("btn-close-error");

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
    hideErrorBox();
    renderRules();
  });

  if (btnCloseError) {
    btnCloseError.addEventListener("click", hideErrorBox);
  }

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
      statusPill.textContent = "NODE: ONLINE";
      statusPill.className = "pill pill-online";
    } else {
      statusPill.textContent = "NODE: OFFLINE";
      statusPill.className = "pill pill-offline";
    }

    if (services.length > 0) {
      updateServiceOptions(services);
    }

    renderServices(services);
  } catch (e) {
    statusPill.textContent = "NODE: OFFLINE";
    statusPill.className = "pill pill-offline";
  }
}

function updateServiceOptions(services) {
  services.forEach((s) => {
    const exists = serviceOptions.some(
      (opt) => opt.rpi_id === s.rpi_id && opt.name === s.service_name
    );
    if (!exists) {
      serviceOptions.push({
        rpi_id: s.rpi_id || "RPi-1",
        name: s.service_name,
        label: `[${s.rpi_id || "RPi"}] ${s.service_name}`
      });
    }
  });
}

async function updateNodes() {
  const host = nodeHost.value.trim() || "127.0.0.1";
  const port = parseInt(nodePort.value.trim(), 10) || 8080;

  statusPill.textContent = "CONNECTING...";

  await fetch("/api/config_nodes", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      nodes: [{ rpi_id: "RPi", host, port }]
    })
  });
  await scanServices();
}

function renderServices(services) {
  serviceCount.textContent = `${services.length} found`;
  if (services.length === 0) {
    serviceList.innerHTML = `<div class="empty">No services detected. Start the RPi server on port 8080.</div>`;
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

/* ==========================================================================
   Argument Validation & Error / Alarm Box Functions
   ========================================================================== */

function getCleanServiceName(srvKey) {
  if (!srvKey) return "";
  return srvKey.includes(":") ? srvKey.split(":")[1] : srvKey;
}

function validateServiceArgument(serviceKey, rawArg) {
  const sName = getCleanServiceName(serviceKey);
  const arg = (rawArg || "").trim();

  if (sName === "Set_Alarm_LED" || sName === "Set_Cooling_Indicator") {
    if (arg === "") {
      return `Missing parameter for ${sName}. Expected '0' (OFF) or '1' (ON).`;
    }
    if (arg !== "0" && arg !== "1") {
      return `Invalid argument '${rawArg}' for ${sName}. Expected '0' (OFF) or '1' (ON).`;
    }
  } else if (sName === "Read_Temperature") {
    if (arg !== "" && arg !== "C" && arg !== "F" && arg !== "c" && arg !== "f") {
      return `Invalid scale '${rawArg}' for Read_Temperature. Expected 'C' (Celsius) or 'F' (Fahrenheit).`;
    }
  }
  return null;
}

function validateAllRules() {
  clearValidationErrors();
  let firstError = null;

  for (let i = 0; i < rules.length; i++) {
    const r = rules[i];
    const ruleNum = i + 1;

    if (r.type === "single") {
      const err = validateServiceArgument(r.srv, r.arg);
      if (err) {
        firstError = {
          ruleNum,
          fieldId: `input-arg-${r.id}`,
          message: `Rule #${ruleNum}: ${err}`
        };
        break;
      }
    } else if (r.type === "order") {
      const errA = validateServiceArgument(r.srvA, r.argA);
      if (errA) {
        firstError = {
          ruleNum,
          fieldId: `input-argA-${r.id}`,
          message: `Rule #${ruleNum} (First Service): ${errA}`
        };
        break;
      }
      const errB = validateServiceArgument(r.srvB, r.argB);
      if (errB) {
        firstError = {
          ruleNum,
          fieldId: `input-argB-${r.id}`,
          message: `Rule #${ruleNum} (Second Service): ${errB}`
        };
        break;
      }
    } else if (r.type === "condition") {
      const valStr = String(r.val !== undefined ? r.val : "").trim();
      if (valStr === "" || isNaN(parseFloat(valStr))) {
        firstError = {
          ruleNum,
          fieldId: `input-val-${r.id}`,
          message: `Rule #${ruleNum}: Condition threshold '${r.val}' must be a valid numeric value.`
        };
        break;
      }
      const errB = validateServiceArgument(r.srvB, r.argB);
      if (errB) {
        firstError = {
          ruleNum,
          fieldId: `input-argB-${r.id}`,
          message: `Rule #${ruleNum} (Action Service): ${errB}`
        };
        break;
      }
    }
  }

  if (firstError) {
    showErrorBox("Invalid Service Argument Detected", firstError.message, firstError.fieldId);
    return false;
  }

  hideErrorBox();
  return true;
}

function showErrorBox(title, message, fieldId = null) {
  if (errorBox && errorDesc) {
    if (errorTitle) errorTitle.textContent = title;
    errorDesc.textContent = message;
    errorBox.style.display = "flex";
  }

  if (fieldId) {
    const el = document.getElementById(fieldId);
    if (el) {
      el.classList.add("input-invalid");
      el.focus();
    }
  }

  appendLog(`[VALIDATION ALARM] ${message}`, "log-error");
}

function hideErrorBox() {
  if (errorBox) {
    errorBox.style.display = "none";
  }
  clearValidationErrors();
}

function clearValidationErrors() {
  document.querySelectorAll(".input-invalid").forEach((el) => {
    el.classList.remove("input-invalid");
  });
}

/* ==========================================================================
   Rule Composition & Rendering
   ========================================================================== */

function addSingleRule(srv = "RPi-1:Read_Temperature", arg = "C") {
  hideErrorBox();
  rules.push({
    id: "r_" + Date.now() + Math.random().toString(36).substr(2, 4),
    type: "single",
    srv,
    arg
  });
  renderRules();
}

function addOrderRule(
  srvA = "RPi-1:Read_Temperature",
  argA = "C",
  srvB = "RPi-2:Set_Alarm_LED",
  argB = "1"
) {
  hideErrorBox();
  rules.push({
    id: "r_" + Date.now() + Math.random().toString(36).substr(2, 4),
    type: "order",
    srvA,
    argA,
    srvB,
    argB
  });
  renderRules();
}

function addConditionRule(
  srvA = "RPi-2:Read_Light_Level",
  op = "<",
  val = 30,
  srvB = "RPi-1:Set_Cooling_Indicator",
  argB = "1"
) {
  hideErrorBox();
  rules.push({
    id: "r_" + Date.now() + Math.random().toString(36).substr(2, 4),
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
        <input type="text" id="input-arg-${r.id}" placeholder="arg" value="${r.arg || ""}" style="width: 65px;" oninput="updateRule('${r.id}', 'arg', this.value)">
        <button class="btn-del" onclick="delRule('${r.id}')" title="Delete">×</button>
      `;
    } else if (r.type === "order") {
      row.className = "rule-row";
      row.innerHTML = `
        <span class="rule-tag">RUN</span>
        ${buildDropdown(r.id, "srvA", r.srvA)}
        <input type="text" id="input-argA-${r.id}" placeholder="arg" value="${r.argA || ""}" style="width: 65px;" oninput="updateRule('${r.id}', 'argA', this.value)">
        <span class="rule-tag">THEN</span>
        ${buildDropdown(r.id, "srvB", r.srvB)}
        <input type="text" id="input-argB-${r.id}" placeholder="arg" value="${r.argB || ""}" style="width: 65px;" oninput="updateRule('${r.id}', 'argB', this.value)">
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
        <input type="number" id="input-val-${r.id}" value="${r.val}" style="width: 75px;" oninput="updateRule('${r.id}', 'val', this.value)">
        <span class="rule-tag">DO</span>
        ${buildDropdown(r.id, "srvB", r.srvB)}
        <input type="text" id="input-argB-${r.id}" placeholder="arg" value="${r.argB || ""}" style="width: 65px;" oninput="updateRule('${r.id}', 'argB', this.value)">
        <button class="btn-del" onclick="delRule('${r.id}')" title="Delete">×</button>
      `;
    }
    rulesContainer.appendChild(row);
  });
}

function buildDropdown(ruleId, field, currentVal) {
  let html = `<select onchange="updateRule('${ruleId}', '${field}', this.value)">`;

  const rpi1Services = serviceOptions.filter((s) => s.rpi_id === "RPi-1");
  const rpi2Services = serviceOptions.filter((s) => s.rpi_id === "RPi-2");

  if (rpi1Services.length > 0) {
    html += `<optgroup label="RPi-1 (Temperature & Cooling)">`;
    rpi1Services.forEach((s) => {
      const key = `${s.rpi_id}:${s.name}`;
      const isSelected =
        currentVal === key || (currentVal === s.name && s.rpi_id === "RPi-1");
      html += `<option value="${key}" ${isSelected ? "selected" : ""}>[RPi-1] ${s.name}</option>`;
    });
    html += `</optgroup>`;
  }

  if (rpi2Services.length > 0) {
    html += `<optgroup label="RPi-2 (Light & Alarm)">`;
    rpi2Services.forEach((s) => {
      const key = `${s.rpi_id}:${s.name}`;
      const isSelected =
        currentVal === key || (currentVal === s.name && s.rpi_id === "RPi-2");
      html += `<option value="${key}" ${isSelected ? "selected" : ""}>[RPi-2] ${s.name}</option>`;
    });
    html += `</optgroup>`;
  }

  html += `</select>`;
  return html;
}

function resolveService(srvKey) {
  let rpi_id = "RPi-1";
  let name = srvKey || "";

  if (srvKey && srvKey.includes(":")) {
    const parts = srvKey.split(":");
    rpi_id = parts[0];
    name = parts[1];
  } else {
    if (name === "Read_Light_Level" || name === "Set_Alarm_LED") {
      rpi_id = "RPi-2";
    } else {
      rpi_id = "RPi-1";
    }
  }

  const host = nodeHost.value.trim() || "127.0.0.1";
  const port = parseInt(nodePort.value.trim(), 10) || 8080;

  return { host, port, name, rpi_id };
}

function updateRule(id, field, val) {
  const r = rules.find((x) => x.id === id);
  if (r) r[field] = val;

  // Clear visual invalid indicator on the input that is currently being edited
  let targetFieldId = "";
  if (field === "arg") targetFieldId = `input-arg-${id}`;
  else if (field === "argA") targetFieldId = `input-argA-${id}`;
  else if (field === "argB") targetFieldId = `input-argB-${id}`;
  else if (field === "val") targetFieldId = `input-val-${id}`;

  if (targetFieldId) {
    const el = document.getElementById(targetFieldId);
    if (el) el.classList.remove("input-invalid");
  }

  // If no more invalid inputs exist on page, dismiss error box
  if (document.querySelectorAll(".input-invalid").length === 0 && errorBox) {
    errorBox.style.display = "none";
  }
}

function delRule(id) {
  rules = rules.filter((x) => x.id !== id);
  hideErrorBox();
  renderRules();
}

async function runApplication() {
  if (rules.length === 0) {
    showErrorBox("No Rules Defined", "Add at least one rule to your composition before running.");
    return;
  }

  // Run comprehensive argument validation before sending requests
  if (!validateAllRules()) {
    return;
  }

  btnRun.disabled = true;
  btnRun.textContent = "Executing...";

  const payload = {
    rules: rules.map((r, i) => {
      if (r.type === "single") {
        const tgt = resolveService(r.srv);
        const paramStr = (r.arg || "").trim();
        const descArg = paramStr ? `(${paramStr})` : "";
        return {
          type: "single",
          description: `Single Service #${i + 1}: [${tgt.rpi_id}] ${tgt.name}${descArg}`,
          service: {
            host: tgt.host,
            port: tgt.port,
            rpi_id: tgt.rpi_id,
            name: tgt.name,
            param: paramStr
          },
          service_a: {
            host: tgt.host,
            port: tgt.port,
            rpi_id: tgt.rpi_id,
            name: tgt.name,
            param: paramStr
          }
        };
      } else if (r.type === "order") {
        const tgtA = resolveService(r.srvA);
        const tgtB = resolveService(r.srvB);
        const paramAStr = (r.argA || "").trim();
        const paramBStr = (r.argB || "").trim();
        const descA = paramAStr ? `(${paramAStr})` : "";
        const descB = paramBStr ? `(${paramBStr})` : "";
        return {
          type: "order",
          description: `Order Rule #${i + 1}: [${tgtA.rpi_id}] ${tgtA.name}${descA} -> [${tgtB.rpi_id}] ${tgtB.name}${descB}`,
          service_a: {
            host: tgtA.host,
            port: tgtA.port,
            rpi_id: tgtA.rpi_id,
            name: tgtA.name,
            param: paramAStr
          },
          service_b: {
            host: tgtB.host,
            port: tgtB.port,
            rpi_id: tgtB.rpi_id,
            name: tgtB.name,
            param: paramBStr
          }
        };
      } else {
        const tgtA = resolveService(r.srvA);
        const tgtB = resolveService(r.srvB);
        const paramBStr = (r.argB || "").trim();
        const descB = paramBStr ? `(${paramBStr})` : "";
        return {
          type: "condition",
          description: `Condition Rule #${i + 1}: IF [${tgtA.rpi_id}] ${tgtA.name} ${r.op} ${r.val} DO [${tgtB.rpi_id}] ${tgtB.name}${descB}`,
          service_a: {
            host: tgtA.host,
            port: tgtA.port,
            rpi_id: tgtA.rpi_id,
            name: tgtA.name,
            param: ""
          },
          operator: r.op,
          threshold: parseFloat(r.val),
          service_b: {
            host: tgtB.host,
            port: tgtB.port,
            rpi_id: tgtB.rpi_id,
            name: tgtB.name,
            param: paramBStr
          }
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

    // If any execution error occurred on the RPi node, display it in the error box
    const failedStep = (result.trace || []).find(
      (s) => s.status && (s.status.includes("FAILED") || s.status.includes("ERROR"))
    );
    if (failedStep) {
      const errMsg = failedStep.error || `Execution failed at Step #${failedStep.step_index}`;
      showErrorBox("RPi Execution Failure", errMsg);
    }
  } catch (e) {
    showErrorBox("Connection Error", `Failed to communicate with Edge Orchestrator: ${e.message}`);
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
    if (step.status.includes("FAILED") || step.status.includes("ERROR"))
      css = "log-error";
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
        const rpiTag = a.rpi_id ? `[${a.rpi_id}] ` : "";
        const argStr = a.input ? `(${a.input})` : "()";
        text += `-> Call ${rpiTag}${a.service}${argStr} = ${valStr} (${(a.result && a.result.latency_ms) || 0}ms)\n`;
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
