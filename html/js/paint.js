let painting = false;
let lastX = 0;
let lastY = 0;
let brushColor = "#000000";
let brushSize = 2;
let currentTool = "brush"; // Current selected tool: brush, eraser, text
let textPositions = []; // Store text elements for re-rendering after dithering
let isTextPlacementMode = false;
let tempTextElement = null;

function initPaintTools() {
  // Setup event listeners
  document.getElementById('brush-mode').addEventListener('click', () => {
    setActiveTool('brush');
    brushColor = document.getElementById('brush-color').value;
  });
  
  document.getElementById('eraser-mode').addEventListener('click', () => {
    setActiveTool('eraser');
    brushColor = "#FFFFFF";
  });

  document.getElementById('text-mode').addEventListener('click', () => {
    setActiveTool('text');
  });
  
  document.getElementById('brush-color').addEventListener('change', (e) => {
    brushColor = e.target.value;
  });
  
  document.getElementById('brush-size').addEventListener('change', (e) => {
    brushSize = parseInt(e.target.value);
  });

  document.getElementById('add-text-btn').addEventListener('click', startTextPlacement);
  
  setupCanvasForPainting();

  // Override the existing clear_canvas function to clear our text positions too
  const originalClearCanvas = window.clear_canvas;
  window.clear_canvas = function() {
    if(originalClearCanvas()) {
      textPositions = []; // Clear stored text positions
    }
  };

  // Override the existing convert_dithering function to preserve text
  const originalConvertDithering = window.convert_dithering;
  window.convert_dithering = function() {
    originalConvertDithering();
    // Redraw text after dithering
    redrawTextElements();
  };
}

function setActiveTool(tool) {
  currentTool = tool;
  
  // Update UI to reflect active tool
  document.getElementById('brush-mode').classList.toggle('active', tool === 'brush');
  document.getElementById('eraser-mode').classList.toggle('active', tool === 'eraser');
  document.getElementById('text-mode').classList.toggle('active', tool === 'text');
  
  // Show/hide text tools
  document.querySelectorAll('.text-tools').forEach(el => {
    el.style.display = tool === 'text' ? 'flex' : 'none';
  });

  // Cancel any pending text placement
  cancelTextPlacement();
}

function setupCanvasForPainting() {
  canvas.addEventListener('mousedown', startPaint);
  canvas.addEventListener('mousemove', paint);
  canvas.addEventListener('mouseup', endPaint);
  canvas.addEventListener('mouseleave', endPaint);
  canvas.addEventListener('click', handleCanvasClick);
  
  // Touch support
  canvas.addEventListener('touchstart', handleTouchStart);
  canvas.addEventListener('touchmove', handleTouchMove);
  canvas.addEventListener('touchend', endPaint);
}

function startPaint(e) {
  if (currentTool !== 'text') {
    painting = true;
    draw(e);
  }
}

function endPaint() {
  painting = false;
  lastX = 0;
  lastY = 0;
}

function paint(e) {
  if (!painting || currentTool === 'text') return;
  draw(e);
}

function draw(e) {
  const rect = canvas.getBoundingClientRect();
  const scaleX = canvas.width / rect.width;
  const scaleY = canvas.height / rect.height;
  const x = (e.clientX - rect.left) * scaleX;
  const y = (e.clientY - rect.top) * scaleY;
  
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  ctx.strokeStyle = brushColor;
  ctx.lineWidth = brushSize;
  
  ctx.beginPath();
  
  if (lastX === 0 && lastY === 0) {
    // For the first point, just do a dot
    ctx.moveTo(x, y);
    ctx.lineTo(x+0.1, y+0.1);
  } else {
    // Connect to the previous point
    ctx.moveTo(lastX, lastY);
    ctx.lineTo(x, y);
  }
  
  ctx.stroke();
  
  lastX = x;
  lastY = y;
}

function handleCanvasClick(e) {
  if (currentTool === 'text' && isTextPlacementMode) {
    placeText(e);
  }
}

// Improve touch handling for text placement
function handleTouchStart(e) {
    e.preventDefault();
    const touch = e.touches[0];
    
    // If in text placement mode, handle as a click
    if (currentTool === 'text' && isTextPlacementMode) {
        const mouseEvent = new MouseEvent('click', {
            clientX: touch.clientX,
            clientY: touch.clientY
        });
        canvas.dispatchEvent(mouseEvent);
        return;
    }
    
    // Otherwise handle as normal drawing
    const mouseEvent = new MouseEvent('mousedown', {
        clientX: touch.clientX,
        clientY: touch.clientY
    });
    canvas.dispatchEvent(mouseEvent);
}

// Also improve the text placement button to ensure it works on mobile
function startTextPlacement() {
    const text = document.getElementById('text-input').value.trim();
    if (!text) {
        alert('请输入文字内容');
        return;
    }

    isTextPlacementMode = true;

    // Add visual feedback
    document.querySelector('.canvas-tooltip').style.display = 'block';
    document.querySelector('.canvas-tooltip').innerText = '点击画布放置文字';
    canvas.classList.add('text-placement-mode');
}

function cancelTextPlacement() {
    isTextPlacementMode = false;
    canvas.classList.remove('text-placement-mode');
    
    if (tempTextElement) {
        tempTextElement.remove();
        tempTextElement = null;
    }
}

function placeText(e) {
  const rect = canvas.getBoundingClientRect();
  const scaleX = canvas.width / rect.width;
  const scaleY = canvas.height / rect.height;
  const x = (e.clientX - rect.left) * scaleX;
  const y = (e.clientY - rect.top) * scaleY;

  const text = document.getElementById('text-input').value;
  const fontFamily = document.getElementById('font-family').value;
  const fontSize = document.getElementById('font-size').value;
  
  // Draw text on canvas
  ctx.font = `${fontSize}px ${fontFamily}`;
  ctx.fillStyle = brushColor;
  ctx.fillText(text, x, y);
  
  // Store text info for redrawing later
  textPositions.push({
    text,
    x,
    y,
    font: `${fontSize}px ${fontFamily}`,
    color: brushColor
  });
  
  // Reset
  document.getElementById('text-input').value = '';
  isTextPlacementMode = false;
  canvas.classList.remove('text-placement-mode');
  document.querySelector('.canvas-tooltip').style.display = 'none';
}

function redrawTextElements() {
  // Redraw all text elements after dithering
  textPositions.forEach(item => {
    ctx.font = item.font;
    ctx.fillStyle = item.color;
    ctx.fillText(item.text, item.x, item.y);
  });
}

function handleTouchMove(e) {
  e.preventDefault();
  const touch = e.touches[0];
  const mouseEvent = new MouseEvent('mousemove', {
    clientX: touch.clientX,
    clientY: touch.clientY
  });
  canvas.dispatchEvent(mouseEvent);
}

// Initialize paint functionality when the page loads
document.addEventListener('DOMContentLoaded', () => {
  setTimeout(initPaintTools, 500); // Delay to ensure canvas is initialized
});