const gridSize = 10, gridCols = 100, gridRows = 100;
const boxCols = 9, boxRows = 9;

const grid = document.getElementById('grid');
const createBoxBtn = document.getElementById('createBox');
const linesSvg = document.getElementById('linesSvg');

grid.style.gridTemplateColumns = `repeat(${gridCols}, ${gridSize}px)`;
grid.style.gridTemplateRows = `repeat(${gridRows}, ${gridSize}px)`;
grid.style.width = gridCols * gridSize + 'px';
grid.style.height = gridRows * gridSize + 'px';

for (let i = 0; i < gridCols * gridRows; i++) {
    const cell = document.createElement('div');

    cell.classList.add('grid-cell');
    cell.style.width = cell.style.height = gridSize + 'px';
    grid.appendChild(cell);
}

let boxes = [];

let usedYPositions = [];

// function getAvailableYforOR(preferredY, minGap = 60) {
//   // Avoid vertical overlaps by shifting down until we find an empty spot
//   while (usedYPositions.some(y => Math.abs(y - preferredY) < minGap)) {
//     preferredY += minGap;
//   }
//   usedYPositions.push(preferredY);
//   return preferredY;
// }
function getAvailableY(preferredY, minGap = 60) {
    const step = 10; // How many pixels to check at a time
    
    // Keep checking as long as this position is invalid
    while (usedYPositions.some(y => Math.abs(y - preferredY) < minGap)) {
        // If it's invalid, move it down by 'step' and try again
        preferredY += step;
    }
    
    // We found a valid spot
    usedYPositions.push(preferredY);
    return preferredY;
}

function generateCircuitFromExpression(expr) {
    // Clear previous circuit
    usedYPositions = [];
    
    boxes.forEach(group => group.forEach(el => el.remove()));
    boxes = [];
    linesSvg.innerHTML = "";
    connections = [];

    // --- NEW LOGIC: Detect SOP vs POS ---
    // Heuristic: If expression starts with '(', assume POS.
    const isPOS = expr.trim().startsWith('(');
    const firstLevelGateType = isPOS ? "OR" : "AND";  // <-- NEW
    const finalGateType = isPOS ? "AND" : "OR";      // <-- NEW
    // --- END NEW LOGIC ---

    let terms;
    
    // --- 1. PARSE EXPRESSION ---
    const inputLaneMap = new Map(); // Stores { box, laneX } for each literal
    const uniqueBaseVars = new Set();
    const uniqueLiterals = new Set();

    // First, find all unique literals regardless of format
    // This regex finds all variables, e.g., A, B', C
    const allLitsRegex = /[A-Z]\'?|[a-z]\'?/g;
    const allLitsMatch = expr.match(allLitsRegex);
    if (allLitsMatch) {
        allLitsMatch.forEach(lit => {
            uniqueBaseVars.add(lit[0].toUpperCase());
            uniqueLiterals.add(lit.toUpperCase());
        });
    }

    // Now, parse the terms based on format
    if (isPOS) {
        // POS: (A+B)(C'+D) -> terms = ["A+B", "C'+D"]
        const termMatches = expr.match(/\((.*?)\)/g);
        if (!termMatches) {
            console.error("Invalid POS format. Expected format: (A+B)(C+D)");
            return;
        }
        // Strip parens: "(A+B)" -> "A+B"
        terms = termMatches.map(t => t.substring(1, t.length - 1));
    } else {
        // SOP: AB+C'D -> terms = ["AB", "C'D"]
        terms = expr.split('+').map(t => t.trim());
    }
    // --- END PARSING ---

    const startX = 100;
    const startY = 50;
    const xSpacing = 150;
    const literalSpacing = 35;
    let currentVarY = startY; // This will now define the top row Y
    let currentLaneX = startX + 100; 
    const laneWidth = 2 * gridSize; 

    // --- 2. BUILD INPUT LANES (MODIFIED FOR HORIZONTAL LAYOUT) ---
    let currentVarX = startX - 80; // <-- NEW: Horizontal position tracker
    const varBoxY = currentVarY + 30; // <-- NEW: Y-row for variables (e.g., A, B)
    const notBoxY = currentVarY + 120; // <-- NEW: Y-row for NOT gates (e.g., A', B')
    const varGroupSpacing = 100; // <-- NEW: Horizontal space between A, B, C groups

    Array.from(uniqueBaseVars).sort().forEach(varName => {
        // Create the base variable box (e.g., 'A')
        const varBox = createVariableBox(varName, currentVarX, varBoxY); // <-- MODIFIED
        varBox.dataset.type = 'variable'; // <-- NEW
        const varLaneX = currentLaneX;
        inputLaneMap.set(varName, { box: varBox, laneX: varLaneX });
        currentLaneX += laneWidth;

        // Check if we also need a NOT gate for this variable
        const notLitName = varName + "'";
        if (uniqueLiterals.has(notLitName)) {
            const notLaneX = currentLaneX;
            const notBox = createNotGateBox("NOT", notLaneX, notBoxY); // <-- MODIFIED
            notBox.dataset.type = 'not'; // <-- NEW
            
            inputLaneMap.set(notLitName, { box: notBox, laneX: notLaneX });
            currentLaneX += laneWidth;
            
            // Auto-connect the base var to the NOT gate
            // Assume boxes are 60px (6*gridSize) wide, so center is 30 (3*gridSize).
            const notInputLane = currentVarX + (gridSize * 3); // <-- MODIFIED: X for vertical wire
            drawAutoConnection(varBox, notBox, 0, 1, notInputLane); 
        }
        // Move horizontally for the next variable
        currentVarX += varGroupSpacing; // <-- MODIFIED
    });
    // --- END INPUT LANES ---

    // --- NEW: Find bottom of input blocks to start gates ---
    // Check if any NOT gates were created.
    const hasNotGates = Array.from(uniqueLiterals).some(lit => lit.includes("'"));
    // 60 is assumed box height (6 * gridSize)
    const inputBlockBottomY = hasNotGates ? notBoxY + (gridSize * 6) : varBoxY + (gridSize * 6);
    const ySpacing = 90; // (This variable might exist above, just ensure it's defined)
    const firstGateY = inputBlockBottomY + ySpacing; // Start gates below inputs
    // --- END NEW ---

    // Pre-populate usedYPositions with all input boxes
    usedYPositions = []; // Start fresh
    inputLaneMap.forEach(litInfo => {
        usedYPositions.push(parseFloat(litInfo.box.style.top));
    });

    const outputGates = [];
    // --- 3. SMART GATE PLACEMENT LOGIC ---
    terms.forEach((term, i) => {
        let literals;
        
        // --- MODIFIED: Parse literals based on format ---
        if (isPOS) {
            // term is "A+B", split by '+'
            literals = term.split('+').map(t => t.trim().toUpperCase());
        } else {
            // term is "AB", find all literals
            literals = term.match(allLitsRegex);
        }
        // --- END MODIFIED ---

        if (!literals || (isPOS && literals[0] === '')) return; // Handle empty terms like () or ""

        const literalYs = literals
            .map(lit => {
                const litInfo = inputLaneMap.get(lit.toUpperCase()); // Ensure uppercase
                return litInfo ? parseFloat(litInfo.box.style.top) : 0;
            })
            .filter(y => !isNaN(y) && y > 0);

        if (literalYs.length === 0) return;

        // --- MODIFIED: No longer use avgY for horizontal layout ---
        // const avgY =
        //     literalYs.reduce((sum, y) => sum + y, 0) / literalYs.length;
        // const baseGateY = avgY - 30;
        // ---
        
        // --- NEW: Place gates sequentially starting from the top of the gate area
        // We use 'firstGateY' as the starting point, and getAvailableY will stack them
        const baseGateY = firstGateY; 
        // --- END NEW ---
        
        // --- MODIFIED: Base gateX on var position, not lane position ---
        // 'currentVarX' is the X-position *after* the last variable group.
        // This is a good place to start drawing gates.
        const gateX = currentVarX + 90; 
        // --- END MODIFIED ---

        if (literals.length === 1) {
                const litInfo = inputLaneMap.get(literals[0].toUpperCase());
            if (litInfo) outputGates.push(litInfo);
        } else {
            const gateY = getAvailableY(baseGateY, 100);
            // --- MODIFIED: Use dynamic gate type ---
            const gateBox = createGateBox(firstLevelGateType, gateX, gateY);
            gateBox.dataset.type = 'gate'; // <-- NEW
            // --- END MODIFIED ---

            const sortedLits = literals
                .map(lit => inputLaneMap.get(lit.toUpperCase()))
                .filter(Boolean)
                .sort(
                    (a, b) =>
                        parseFloat(a.box.style.top) - parseFloat(b.box.style.top)
                );

            sortedLits.forEach((litInfo, idx) => {
                drawAutoConnection(litInfo.box, gateBox, idx, sortedLits.length, litInfo.laneX);
            });

            outputGates.push({ box: gateBox, laneX: gateX + 100 });
        }
    });

    // --- 4. FINAL GATE (if multiple outputs) ---
    if (outputGates.length > 1) {
        const avgY =
            outputGates.reduce((sum, gateInfo) => sum + parseFloat(gateInfo.box.style.top), 0) /
            outputGates.length;
        
        const maxInputBoxX = Math.max(
            ...outputGates.map(gateInfo => {
                return parseFloat(gateInfo.box.style.left) + gateInfo.box.offsetWidth;
            })
        );

        const finalGateX = maxInputBoxX + xSpacing; // <-- MODIFIED (renamed variable)
        
        // --- MODIFIED: Use dynamic gate type ---
        const finalGateBox = createGateBox(finalGateType, finalGateX, avgY);   
        finalGateBox.dataset.type = 'gate'; // <-- NEW
        // --- END MODIFIED ---
        
        const sortedOutputs = outputGates
            .slice()
            .sort((a, b) => parseFloat(a.box.style.top) - parseFloat(b.box.style.top));

        const total = sortedOutputs.length;
        const half = Math.ceil(total / 2);

        const laneXs = [];
        for (let i = 0; i < half; i++) {
            laneXs.push(finalGateX - (xSpacing / 2) - (i * laneWidth));
        }

        const mirroredLaneXs =
            total % 2 === 0
                ? [...laneXs, ...laneXs.slice().reverse()] 
                : [...laneXs, ...laneXs.slice(0, -1).reverse()];

        sortedOutputs.forEach((gateInfo, i) => {
            const laneX = mirroredLaneXs[i];
            // --- MODIFIED: Connect to new final gate ---
            drawAutoConnection(gateInfo.box, finalGateBox, i, total, laneX);
            // --- END MODIFIED ---
        });
    }
}

function createVariableBox(label,x,y) {
    const mainBox = document.createElement('div');
    mainBox.classList.add('box');
    mainBox.style.width = 3 * gridSize + 'px';
    mainBox.style.height = 3 * gridSize + 'px';
    mainBox.style.left = x + 'px';
    mainBox.style.top = y + 'px';
    mainBox.textContent = label;
    mainBox.style.display = "flex";
    mainBox.style.alignItems = "center";
    mainBox.style.justifyContent = "center";
    mainBox.style.fontWeight = "bold";

    grid.appendChild(mainBox);
    boxes.push([mainBox]);
    return mainBox;
}
/** Helper to create a labeled gate box */
function createGateBox(label, x, y) {
    const mainBox = document.createElement('div');
    mainBox.classList.add('box');
    mainBox.style.width = boxCols * gridSize + 'px';
    mainBox.style.height = boxRows * gridSize + 'px';
    mainBox.style.left = x + 'px';
    mainBox.style.top = y + 'px';
    mainBox.textContent = label;
    mainBox.style.display = "flex";
    mainBox.style.alignItems = "center";
    mainBox.style.justifyContent = "center";
    mainBox.style.fontWeight = "bold";

    grid.appendChild(mainBox);
    boxes.push([mainBox]);
    return mainBox;
}

function createNotGateBox(label, x, y) {
    const box = document.createElement('div');
    box.classList.add('box', 'not-box');
    box.style.width = 35 + 'px';   // smaller than gates
    box.style.height = 35 + 'px';
    box.style.left = x + 'px';
    box.style.top = y + 'px';
    box.textContent = label;
    box.style.fontSize = '12px';
    box.style.display = "flex";
    box.style.alignItems = "center";
    box.style.justifyContent = "center";
    box.style.fontWeight = "bold";

    grid.appendChild(box);
    boxes.push([box]);
    return box;
}

/** Helper to connect two boxes visually */
function drawAutoConnection(boxA, boxB, inputIndex, totalInputs, forcedMidX) {
    const rectA = boxA.getBoundingClientRect();
    const rectB = boxB.getBoundingClientRect();
    const gridRect = grid.getBoundingClientRect(); // Assumes 'grid' is globally accessible

    let x1, y1, x2, y2;
    
    const typeA = boxA.dataset.type;
    const typeB = boxB.dataset.type;

    // --- 1. Get Start Point (x1, y1) based on boxA type ---
    if (typeA === 'variable' || typeA === 'not') {
        // Start from bottom-center
        x1 = rectA.left - gridRect.left + rectA.width / 2;
        y1 = rectA.bottom - gridRect.top;
    } else {
        // Default: Start from right-center (for gate-to-gate)
        x1 = rectA.right - gridRect.left;
        y1 = rectA.top - gridRect.top + rectA.height / 2;
    }

    // --- 2. Get End Point (x2, y2) based on boxB type ---
    if (typeB === 'not') {
        // End at top-center
        x2 = rectB.left - gridRect.left + rectB.width / 2;
        y2 = rectB.top - gridRect.top;
    } else {
        // Default: End at left-side, fanned out (for gates)
        x2 = rectB.left - gridRect.left;
        const spacing = rectB.height / (totalInputs + 1);
        y2 = (rectB.top - gridRect.top) + (spacing * (inputIndex + 1));
    }
    
    // --- 3. Create the Path ---
    let pathData;
    const midX = forcedMidX; // Use the dedicated vertical lane X

    if (typeA === 'variable' && typeB === 'not') {
        // Special case: Variable to its NOT gate (straight down)
        // We know they are x-aligned, so x1 and x2 are the same.
        pathData = `
          M ${x1} ${y1}  
          V ${y2}
        `;
        // No connection dot needed for this simple line
    } else {
        // General case (e.g., Var -> Gate, Not -> Gate, Gate -> Gate)
        const yBuffer = 20; // How far to drop down before turning
        
        if (typeA === 'variable' || typeA === 'not') {
            // Start from bottom: M -> V -> H -> V -> H
            pathData = `
              M ${x1} ${y1}          
              V ${y1 + yBuffer}    
              H ${midX}            
              V ${y2}              
              H ${x2}              
            `;
        } else {
            // Start from right (gate-to-gate): M -> H -> V -> H
            pathData = `
              M ${x1} ${y1}    
              H ${midX}      
              V ${y2}        
              H ${x2}        
            `;
        }
        
        // Add connection dot for gate inputs
        const dot = document.createElementNS("http://www.w3.org/2000/svg", "circle");
        dot.setAttribute("cx", x2);
        dot.setAttribute("cy", y2);
        dot.setAttribute("r", "3");
        dot.setAttribute("fill", "#222");
        linesSvg.appendChild(dot); // Assumes 'linesSvg' is global
    }

    const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    path.setAttribute("d", pathData);
    path.setAttribute("stroke", "#222");
    path.setAttribute("fill", "none");
    path.setAttribute("stroke-width", "2");
    path.setAttribute("stroke-linejoin", "round");
    path.setAttribute("stroke-linecap", "round");
    path.setAttribute("class", "circuit-line");
    linesSvg.appendChild(path); // Assumes 'linesSvg' is global
}


function addWireBridges() {
    const paths = Array.from(linesSvg.querySelectorAll(".circuit-line"));
    const tolerance = 4; // closeness threshold for detecting intersections

    // Remove old bridges
    linesSvg.querySelectorAll(".wire-bridge").forEach(b => b.remove());

    for (let i = 0; i < paths.length; i++) {
    for (let j = i + 1; j < paths.length; j++) {
        const path1 = paths[i];
        const path2 = paths[j];
        const points1 = path1.getAttribute("d").match(/[-\d.]+/g).map(Number);
        const points2 = path2.getAttribute("d").match(/[-\d.]+/g).map(Number);

        // Very basic detection: if vertical and horizontal segments cross
        for (let k = 0; k < points1.length - 3; k += 2) {
        const x1a = points1[k], y1a = points1[k + 1];
        const x1b = points1[k + 2], y1b = points1[k + 3];

        const vertical1 = x1a === x1b;

        for (let l = 0; l < points2.length - 3; l += 2) {
            const x2a = points2[l], y2a = points2[l + 1];
            const x2b = points2[l + 2], y2b = points2[l + 3];
            const vertical2 = x2a === x2b;

            if (vertical1 !== vertical2) {
            const vx = vertical1 ? x1a : x2a;
            const hy = vertical1 ? y2a : y1a;

            // Check if intersection point lies within both segment bounds
            if (
                vx > Math.min(x1a, x1b) - tolerance &&
                vx < Math.max(x1a, x1b) + tolerance &&
                hy > Math.min(y2a, y2b) - tolerance &&
                hy < Math.max(y2a, y2b) + tolerance &&
                hy > Math.min(y1a, y1b) - tolerance &&
                hy < Math.max(y1a, y1b) + tolerance &&
                vx > Math.min(x2a, x2b) - tolerance &&
                vx < Math.max(x2a, x2b) + tolerance
            ) {
                // Draw small arc (bridge)
                const bridge = document.createElementNS("http://www.w3.org/2000/svg", "path");
                const r = 5;
                const pathBridge = `M ${vx - r} ${hy} A ${r} ${r} 0 0 1 ${vx + r} ${hy}`;
                bridge.setAttribute("d", pathBridge);
                bridge.setAttribute("stroke", "#222");
                bridge.setAttribute("fill", "none");
                bridge.setAttribute("stroke-width", "2");
                bridge.setAttribute("class", "wire-bridge");
                linesSvg.appendChild(bridge);
            }
            }
        }
        }
    }
    }
}

window.addEventListener("message", (event) => {
    if (event.data.type === "GENERATE_CIRCUIT") {
    const expr = event.data.expression;
    console.log("Received expression:", expr);
    generateCircuitFromExpression(expr);
    }
});