// --- Global Variable ---
let minimizedExpr = ""; 

// --- UI Element References ---
const modeSelector = document.getElementById('modeSelector');
const evaluatorContainer = document.getElementById('evaluatorContainer');
const expressionInput = document.getElementById('expressionInput');
const mintermInput = document.getElementById('mintermInput');
const outputDiv = document.getElementById('output');
const circuitDiag = document.getElementById('circuit_diagram');

// --- Button References ---
const btnShowExpr = document.getElementById('btnShowExpr');
const btnShowMinterm = document.getElementById('btnShowMinterm');
const btnBack = document.getElementById('btnBack');
const btnEvaluate = document.getElementById('btnEvaluate');
const btnGenerateCircuit = document.getElementById('btnGenerateCircuit');

// --- Navigation Logic ---

// When "Evaluate Expression" is clicked:
btnShowExpr.addEventListener('click', () => {
  modeSelector.classList.add('hidden');
  evaluatorContainer.classList.remove('hidden');
  expressionInput.classList.remove('hidden');
  mintermInput.classList.add('hidden');
});

// When "Evaluate Minterms/Maxterms" is clicked:
btnShowMinterm.addEventListener('click', () => {
  modeSelector.classList.add('hidden');
  evaluatorContainer.classList.remove('hidden');
  expressionInput.classList.add('hidden');
  mintermInput.classList.remove('hidden');
});

// When "Back" is clicked:
btnBack.addEventListener('click', () => {
  modeSelector.classList.remove('hidden');
  evaluatorContainer.classList.add('hidden');
  // Clear old output
  outputDiv.innerHTML = "";
  outputDiv.classList.add('hidden');
  circuitDiag.classList.add('hidden');
  document.getElementById('circuitFrame').style.display = 'none';
});

btnGenerateCircuit.addEventListener('click', () => {
    if (!minimizedExpr) {
        alert("Please evaluate an expression first!");
        return;
    }

    // 1. Show the main circuit container
    circuitDiag.classList.remove('hidden');
    
    // 2. Find and show the iframe inside it
    const circuitFrame = document.getElementById('circuitFrame');
    circuitFrame.style.display = 'block'; // Make the iframe visible

    // 3. Send the expression to the iframe
    circuitFrame.contentWindow.postMessage(
        { type: "GENERATE_CIRCUIT", expression: minimizedExpr },
        "*"
    );
});

// --- Evaluation Logic ---

// The main "Evaluate" button
btnEvaluate.addEventListener('click', () => {
  let payload = "";
  let isValid = true;
  outputDiv.classList.remove('hidden');
  // 1. Check which input view is active
  if (!expressionInput.classList.contains('hidden')) {
    // --- Mode 1/2: Expression (SOP/POS) ---

    // Get the value from the NEW dropdown (1 for SOP, 2 for POS)
    const exprType = document.getElementById('expressionType').value;
    const expr = document.getElementById('inputBoxExpr').value.trim();
    
    if (!expr) {
      outputDiv.innerHTML = `<p style="color:#ca8a04;">⚠️ Please enter an expression.</p>`;
      isValid = false;
    } else {
      // Build the new payload: "1\n[expression]" or "2\n[expression]"
      payload = `${exprType}\n${expr}`;
    }
  } else {
    // --- Mode 3/4: Minterm/Maxterm ---
    
    // This code reads "3" or "4" from the HTML you changed
    const inputType = document.getElementById('inputType').value; 
    const varList = document.getElementById('varList').value.trim();
    const termList = document.getElementById('termList').value.trim();

    if (!varList || !termList) {
      outputDiv.innerHTML = `<p style="color:#ca8a04;">⚠️ Please enter variables and terms.</p>`;
      isValid = false;
    } else {
      // Build the payload: "3\n[vars]\n[terms]" or "4\n[vars]\n[terms]"
      payload = `${inputType}\n${varList}\n${termList}`;
    }
  }

  // 2. If input is valid, call the backend
  if (isValid) {
    runBackendEvaluation(payload);
  }
});

/**
 * Sends the formatted payload to the backend server.
 */
async function runBackendEvaluation(payload) {
  document.getElementById('btnGenerateCircuit').classList.add('hidden');
  
  outputDiv.classList.remove('hidden');
  outputDiv.innerHTML = `<p style="color:#0284c7;">⏳ Evaluating...</p>`;

  try {
    const response = await fetch("http://localhost:5000/evaluate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ input: payload })
    });

    const serverResponse = await response.json();
    if (serverResponse.error) {
      outputDiv.innerHTML = `<p style="color:#dc2626;">Server Error: ${serverResponse.error}</p>`;
      return;
    }

    const jsonData = JSON.parse(serverResponse.output);
    renderOutput(jsonData);

  } catch (error) {
    outputDiv.innerHTML = `<p style="color:#dc2626;">⚠️ Error connecting to server or parsing data.</p>`;
    console.error(error);
  }
}

/**
 * Renders the JSON data from the server into the #output div.
 */
function renderOutput(data) {
  outputDiv.style.opacity = "0";

  setTimeout(() => {
    if (data.error) {
      outputDiv.innerHTML = `<p style="color: #dc2626;">⚠️ Error: ${data.error}</p>`;
    } else {
      minimizedExpr = data.minimized; // Store for circuit generator
      
      let html = "";
      
      // Minterm/Maxterm input only returns a minimized expression.
      // Expression input returns the full set.
      
      if (data.postfix) {
        html += `<h3>Postfix Expression</h3><p>${data.postfix}</p>`;
      }
      
      html += `<h3>Minimised Expression</h3><p>${data.minimized || "None"}</p>`;
      
      if (data.headers && data.rows) {
        html += `<h3>Truth Table</h3><table class="truth-table">`;
        html += '<thead><tr>';
        for (const header of data.headers) html += `<th>${header}</th>`;
        html += '</tr></thead><tbody>';
        for (const row of data.rows) {
          html += '<tr>';
          for (const cell of row) html += `<td>${cell}</td>`;
          html += '</tr>';
        }
        html += '</tbody></table>';
      }
      
      outputDiv.innerHTML = html;
    }
    // Show or hide the circuit button based on success
    if (data.error || !minimizedExpr) {
      document.getElementById('btnGenerateCircuit').classList.add('hidden');
    } else {
      document.getElementById('btnGenerateCircuit').classList.remove('hidden');
    }
    outputDiv.style.opacity = "1";
  }, 200);
}

// /**
//  * Sends the minimized expression to the circuit.html iframe.
//  */
// function openCircuitView() {
//   const circuitFrame = document.getElementById('circuitFrame');
  
//   if (!minimizedExpr) {
//     alert("Please evaluate an expression first!");
//     return;
//   }

//   circuitFrame.style.display = 'block';

//   circuitFrame.contentWindow.postMessage(
//     { type: "GENERATE_CIRCUIT", expression: minimizedExpr },
//     "*"
//   );
// }
