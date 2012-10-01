/*
 * Copyright (C) Research In Motion Limited, 2012. All rights reserved.
 */

(function (){
var _gradientColorR = 255,
    _gradientColorG = 0,
    _gradientColorB = 0,
    _selectedColorR = 255,
    _selectedColorG = 0,
    _selectedColorB = 0,
    GRADIENT_SQUARE_SIZE = 360,
    PALETTE_WIDTH = 100,
    PADDING = 30,
    _mouseDown,
    HUE = [
        [255,   0,   0 ], // 0, Red,
        [255, 255,   0 ], // 1, Yellow,
        [  0, 255,   0 ], // 2, Green,
        [  0, 255, 255 ], // 3, Cyan,
        [  0,   0, 255 ], // 4, Blue,
        [255,   0, 255 ], // 5, Magenta,
        [255,   0,   0]]; // 6, Red.

var hexToR = function (h) {return parseInt((cutHex(h)).substring(0, 2), 16);};

var hexToG = function (h) {return parseInt((cutHex(h)).substring(2, 4), 16);};

var hexToB = function (h) {return parseInt((cutHex(h)).substring(4, 6), 16);};

var cutHex = function (h) {return (h.charAt(0) == "#") ? h.substring(1, 7) : h;};

var rgbToHex = function(R, G, B) {return toHex(R) + toHex(G) + toHex(B); };

var toHex = function (n) {
    n = parseInt(n, 10);
    if (isNaN(n)) {
        return "00";
    }
    n = Math.max(0, Math.min(n, 255));
    return "0123456789ABCDEF".charAt((n - n % 16 ) / 16) + "0123456789ABCDEF".charAt(n % 16);
};

var setColor = function () {
    var elem = document.getElementById("myCanvas");
    var context = elem.getContext("2d");
    if (!context) {
        return;
    }
    var red = document.getElementById("rangeR");
    var green = document.getElementById("rangeG");
    var blue = document.getElementById("rangeB");
    _selectedColorR = _gradientColorR = red.value;
    _selectedColorG = _gradientColorG = green.value;
    _selectedColorB = _gradientColorB = blue.value;
    var color = "rgb(" + red.value + "," + green.value + "," + blue.value + ")";
    updateColorNumber(red.value,  green.value, blue.value);
    drawSingleColorGradient(context, color);
    drawSelectedColor(context, color);
};

var updateColorNumber = function (R, G, B) {
    document.getElementById("rangeRvalue").innerText = R;
    document.getElementById("rangeGvalue").innerText = G;
    document.getElementById("rangeBvalue").innerText = B;
};

var updateRange = function (R, G, B) {
    var red = document.getElementById("rangeR");
    red.value = R;
    var green = document.getElementById("rangeG");
    green.value = G;
    var blue = document.getElementById("rangeB");
    blue.value = B;
}

var getMousePos = function (canvas, evt) {
    // get canvas position
    var obj = canvas;
    var top = 0;
    var left = 0;
    do {
        top += obj.offsetTop;
        left += obj.offsetLeft;
    } while (obj = obj.offsetParent);
    // return relative mouse position
    var mouseX = evt.clientX - left + window.pageXOffset;
    var mouseY = evt.clientY - top + window.pageYOffset;
    return {
        x: mouseX,
        y: mouseY
    };
};

var drawSingleColorGradient = function (context, color) {
    var singleColorgradient = context.createLinearGradient(PALETTE_WIDTH + PADDING * 2, 0, PALETTE_WIDTH + PADDING * 2 + GRADIENT_SQUARE_SIZE, 0);
    singleColorgradient.addColorStop(0, "black");
    singleColorgradient.addColorStop(0.5, color);
    singleColorgradient.addColorStop(1, "white");
    context.fillStyle = singleColorgradient;
    context.fillRect(PALETTE_WIDTH + PADDING * 2, PADDING, GRADIENT_SQUARE_SIZE, GRADIENT_SQUARE_SIZE);
};

var drawSelectedColor = function (context, color) {
    var colorSquareSize = 100;
    context.beginPath();
    context.fillStyle = color;
    context.fillRect(PALETTE_WIDTH + PADDING * 3 + GRADIENT_SQUARE_SIZE, GRADIENT_SQUARE_SIZE / 2 - colorSquareSize / 2, colorSquareSize, colorSquareSize);
    context.strokeRect(PALETTE_WIDTH + PADDING * 3 + GRADIENT_SQUARE_SIZE, GRADIENT_SQUARE_SIZE / 2 - colorSquareSize / 2, colorSquareSize, colorSquareSize);
};

var drawSelectedGradientAndColor = function (evt) {
    var elem = document.getElementById("myCanvas");
    var context = elem.getContext("2d");
    var mousePos = getMousePos(elem, evt);
    var color = undefined;

    if (_mouseDown &&
        mousePos &&
        mousePos.x > PADDING &&
        mousePos.x < PADDING + PALETTE_WIDTH &&
        mousePos.y > PADDING &&
        mousePos.y < PADDING + GRADIENT_SQUARE_SIZE) {
        /*
         * Palette is 360x100 and is offset by padding
         * from top and bottom
         */
        var y = mousePos.y - PADDING;
        var currentHue = (y % 60) / 60;
        var currentColor = Math.floor (y / 60);
        var nextColor = (currentColor + 1) > 5 ? 0 : currentColor + 1;
        _gradientColorR = Math.abs(HUE[currentColor][0] - Math.round(Math.abs(HUE[nextColor][0] - HUE[currentColor][0]) * currentHue));
        _gradientColorG = Math.abs(HUE[currentColor][1] - Math.round(Math.abs(HUE[nextColor][1] - HUE[currentColor][1]) * currentHue));
        _gradientColorB = Math.abs(HUE[currentColor][2] - Math.round(Math.abs(HUE[nextColor][2] - HUE[currentColor][2]) * currentHue));
        color = "rgb(" + _gradientColorR + "," + _gradientColorG + "," + _gradientColorB + ")";
    }

    if (color) {
        drawSingleColorGradient(context, color);
        drawSelectedColor(context, color);
        updateRange(_gradientColorR, _gradientColorG, _gradientColorB);
        updateColorNumber(_gradientColorR, _gradientColorG, _gradientColorB);
    }

    // Draw selected color
    if (_mouseDown &&
        mousePos &&
        mousePos.x > PALETTE_WIDTH + PADDING * 2 &&
        mousePos.x < PALETTE_WIDTH + PADDING * 2 + GRADIENT_SQUARE_SIZE &&
        mousePos.y > PADDING &&
        mousePos.y < PADDING + GRADIENT_SQUARE_SIZE) {
        var x = mousePos.x - PALETTE_WIDTH - PADDING * 2;
        var gradientLength = GRADIENT_SQUARE_SIZE / 2;
        if (x > GRADIENT_SQUARE_SIZE / 2) {
            _selectedColorR = Math.round((255 - _gradientColorR) * (x - gradientLength) / gradientLength) + _gradientColorR;
            _selectedColorG = Math.round((255 - _gradientColorG) * (x - gradientLength) / gradientLength) + _gradientColorG;
            _selectedColorB = Math.round((255 - _gradientColorB) * (x - gradientLength) / gradientLength) + _gradientColorB;
        }
        if (x < GRADIENT_SQUARE_SIZE / 2) {
            _selectedColorR = Math.round(_gradientColorR * x / gradientLength);
            _selectedColorG = Math.round(_gradientColorG * x / gradientLength);
            _selectedColorB = Math.round(_gradientColorB * x / gradientLength);
        }
        color = "rgb(" + _selectedColorR + "," + _selectedColorG + "," + _selectedColorB + ")";
     }

    if (color) {
        drawSelectedColor(context, color);
        updateRange(_selectedColorR, _selectedColorG, _selectedColorB);
        updateColorNumber(_selectedColorR, _selectedColorG, _selectedColorB);
        var inputValue = document.getElementById("input");
        inputValue.value = color;
    }
};

var initCanvas = function (input) {
    var elem = document.getElementById("myCanvas");
    var context = elem.getContext("2d");
    if (!context) {
        return;
    }
    // The hue spectrum used by HSV color picker charts.
    var color;
    // Create the linear gradient: sx, sy, dx, dy.
    // That's the start (x,y) coordinates, followed by the destination (x,y).
    var gradient = context.createLinearGradient(PADDING, PADDING, PADDING, PADDING + GRADIENT_SQUARE_SIZE);
    // Add the color stops.
    for (var i = 0; i <= 6; i++) {
        color = 'rgb(' + HUE[i][0] + ', ' + HUE[i][1] + ', ' + HUE[i][2] + ')';
        gradient.addColorStop(i / 6, color);
    }
    // Use the gradient for the fillStyle.
    context.fillStyle = gradient;
    // Now let's draw a rectangle.
    context.fillRect(PADDING, PADDING, PALETTE_WIDTH, GRADIENT_SQUARE_SIZE);

    _mouseDown = false;

    elem.addEventListener("mousedown", function () { _mouseDown = true; }, false);
    elem.addEventListener("mouseup", function (evt) { drawSelectedGradientAndColor(evt); _mouseDown = false; }, false);
    elem.addEventListener("mousemove", drawSelectedGradientAndColor, false);

    if (!input) {
        // Initialize with a dark blue.
        _selectedColorR = _gradientColorR = hexToR("#001337");
        _selectedColorG = _gradientColorG = hexToG("#001337");
        _selectedColorB = _gradientColorB = hexToB("#001337");
    } else {
        _selectedColorR = _gradientColorR = hexToR(input);
        _selectedColorG = _gradientColorG = hexToG(input);
        _selectedColorB = _gradientColorB = hexToB(input);
    }
    drawSingleColorGradient(context, "rgb(" + _selectedColorR + "," + _selectedColorG + "," + _selectedColorB + ")");
    drawSelectedColor(context, "rgb(" + _selectedColorR + "," + _selectedColorG + "," + _selectedColorB + ")");
};

var ok = function () {
    var result = "#" + rgbToHex(_selectedColorR, _selectedColorG, _selectedColorB);
    window.setValueAndClosePopup(result, window.popUp);
}

var cancel = function () {
    window.setValueAndClosePopup("-1", window.popUp);
}

var layout = function() {
    var popup = document.createElement("div"),
        select = document.createElement("div");
    popup.className = "popup-area";
    popup.id = "popup-area";
    select.className = "select-area";
    select.id = "select-area";
    popup.appendChild(select);

    var canvasArea = document.createElement("div"),
        canvas = document.createElement("canvas");

    canvasArea.appendChild(canvas);
    canvasArea.className = "canvas";
    canvas.id = "myCanvas";
    canvas.setAttribute("width", GRADIENT_SQUARE_SIZE * 2);
    canvas.setAttribute("height", GRADIENT_SQUARE_SIZE + PADDING);

    var rangeArea = document.createElement("div");
    rangeArea.className = "range";
    var indexes = {R : 1, G : 2, B : 3};
    for (var i in indexes) {
        var range = document.createElement("input");
        // FIXME: i18n may come later.
        rangeArea.appendChild(document.createTextNode("  " + i + ":  "));
        range.setAttribute("type", "range");
        range.setAttribute("min", "0");
        range.setAttribute("max", "255");
        range.setAttribute("id", "range" + i);
        range.addEventListener("change", setColor, false);
        rangeArea.appendChild(range);
        var rangeValue = document.createElement("div");
        rangeValue.setAttribute("id", "range" + i + "value");
        rangeValue.appendChild(document.createTextNode(range.value));
        rangeValue.className = "range-value";
        rangeArea.appendChild(rangeValue);
        canvasArea.appendChild(rangeArea);
    }
    select.appendChild(canvasArea);
    var buttons = document.createElement("div"),
        cancelButton = document.createElement("button"),
        okButton = document.createElement("button"),
        buttonDivider = document.createElement("div");
    buttons.className = "popup-buttons";
    cancelButton.className = "popup-button";
    cancelButton.addEventListener("click", cancel);
    cancelButton.appendChild(document.createTextNode("Cancel"));
    buttons.appendChild(cancelButton);
    buttonDivider.className = "popup-button-divider";
    buttons.appendChild(buttonDivider);
    okButton.className = "popup-button";
    okButton.addEventListener("click", ok);
    okButton.appendChild(document.createTextNode("OK"));
    buttons.appendChild(okButton);
    select.appendChild(buttons);

    document.body.appendChild(popup);
};

var show = function (inputColor) {
    layout();
    initCanvas(inputColor);
    updateRange(_selectedColorR, _selectedColorG, _selectedColorB);
    updateColorNumber(_selectedColorR, _selectedColorG, _selectedColorB);
};

window.popupcontrol = window.popupcontrol || {};
window.popupcontrol.show = show;
}());
