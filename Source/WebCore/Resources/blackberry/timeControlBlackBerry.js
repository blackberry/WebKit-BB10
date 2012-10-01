/*
 * Copyright (C) Research In Motion Limited, 2012. All rights reserved.
 */
(function (){
var _lastPosX,
    _lastPosY,
    _numRows,
    _numColumns,
    _currentColumn,
    _moveStart,
    _minimumDate,
    _maximumDate,
    _initialDate,
    _currentDate,
    _offsetHour,
    _offsetMin,
    _offset,
    _type,
    _previousOrientation = 0,
    _viewportWidth,
    _viewportHeight,
    BORDER_PADDING = 30,
    WINDOW_PADDING = 80,
    ROW_HEIGHT = 140,
    DAY_IN_MILLIS = 86400000,
    CELL_ID_PREFIX = "celldiv",
    HOUR = "hour",
    MIN = "min",
    DAY = "day",
    WEEK = "week",
    MONTH = "month",
    YEAR = "year",
    DATETIME = "datetime",
    COLUMN_TYPES = { "Date": ["day", "month", "year"],
        "DateTime": ["datetime", "hour", "min"],
        "DateTimeLocal": ["datetime", "hour", "min"],
        "Time": ["hour", "min"],
        "Month": ["month", "year"]
        };

var formatLabels = function () {
    // FIXME: this may change with different locale.
    MONTH_LABELS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    MONTH_FULL_LABELS = ["Janury", "February", "March", "April", "May", "June",
                       "July", "August", "September", "October","November", "December"];
    DAYS_OF_THE_WEEK = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
}

var parseDateString = function (current) {
    var result;
    var now = new Date();
    // Initial timezone offset.
    _offsetHour = 0;
    _offsetMin = 0;
    _offset = 0;
    if (current) {
        if (_type === "Time") {
            result = current.match(/(\d+):(\d+)/);
            return new Date(Date.UTC(now.getFullYear(), now.getMonth(), now.getDate(), Number(result[1]), Number(result[2])));
        } else if (_type === "Date") {
            result = current.match(/(\d+)-(\d+)-(\d+)/);
            return new Date(Date.UTC(Number(result[1]), Number(result[2]) - 1, Number(result[3])));
        } else if (_type === "Month") {
            result = current.match(/(\d+)-(\d+)/);
            return new Date(Date.UTC(Number(result[1]), Number(result[2]) - 1));
        } else {
            result = current.match(/(\d+)-(\d+)-(\d+)[T ](\d+):(\d+)/);
            var UTCTime = new Date(Date.UTC(Number(result[1]), Number(result[2]) - 1, Number(result[3]), Number(result[4]), Number(result[5])));
            if (result = current.match(/(\d+)-(\d+)-(\d+)[T ](\d+):(\d+)+(\d+):(\d+)/)) {
                _offsetHour = result[6];
                _offsetMin = result[7];
                _offset = (Number(_offsetHour) * 60 + Number(_offsetMin)) * 60000;
            } else if (result = current.match(/(\d+)-(\d+)-(\d+)[T ](\d+):(\d+)-(\d+):(\d+)/)) {
                _offsetHour = result[6];
                _offsetMin = result[7];
                _offset = -(Number(_offsetHour) * 60 + Number(_offsetMin)) * 60000;
            }
            return new Date(UTCTime.getTime() + _offset);
        }
    } else {
        switch (_type) {
            case "Time":
            case "DateTime":
            case "DateTimeLocal":
                return new Date(Date.UTC(now.getFullYear(), now.getMonth(), now.getDate(), now.getHours(), now.getMinutes()));
            case "Date":
                return new Date(Date.UTC(now.getFullYear(), now.getMonth(), now.getDate()));
            case "Month":
                return new Date(Date.UTC(now.getFullYear(), now.getMonth()));
        }
    }
}

var serializeDate = function (year, month, day, hour, minute) {
    var yearString = String(year);
    if (yearString.length < 4) {
            yearString = ("000" + yearString).substr(-4, 4);
    }
    switch (_type) {
        case "Time":
            return ("0" + (hour)).substr(-2, 2) + ":" + ("0" + minute).substr(-2, 2);
        case "DateTime": {
            if (_offset === 0) {
                return yearString + "-" + ("0" + (month)).substr(-2, 2) + "-" + ("0" + day).substr(-2, 2) + "T" + ("0" + (hour)).substr(-2, 2) + ":" + ("0" + minute).substr(-2, 2) + "Z";
            } else if (_offset > 0) {
                return yearString + "-" + ("0" + (month)).substr(-2, 2) + "-" + ("0" + day).substr(-2, 2) + "T" + ("0" + (hour)).substr(-2, 2) + ":" + ("0" + minute).substr(-2, 2) + "+" + ("0" + (_offsetHour)).substr(-2, 2) + ":" + ("0" + _offsetMin).substr(-2, 2);
            } else {
                return yearString + "-" + ("0" + (month)).substr(-2, 2) + "-" + ("0" + day).substr(-2, 2) + "T" + ("0" + (hour)).substr(-2, 2) + ":" + ("0" + minute).substr(-2, 2) + "-" + ("0" + (_offsetHour)).substr(-2, 2) + ":" + ("0" + _offsetMin).substr(-2, 2);
            }
        }
        case"DateTimeLocal":
            return yearString + "-" + ("0" + (month)).substr(-2, 2) + "-" + ("0" + day).substr(-2, 2) + "T" + ("0" + (hour)).substr(-2, 2) + ":" + ("0" + minute).substr(-2, 2);
        case "Date":
            return yearString + "-" + ("0" + (month)).substr(-2, 2) + "-" + ("0" + day).substr(-2, 2);
        case "Month":
            return yearString + "-" + ("0" + (month)).substr(-2, 2);
    }
}

var getResult = function (type) {
    switch (type) {
    case "Date":
        var returnValue = new Date(getFocusValue(2), getFocusMonth(1) - 1, getFocusDay(0));
        if (returnValue > _maximumDate) {
            returnValue = _maximumDate;
        } else if (returnValue < _minimumDate) {
            returnValue  = _minimumDate;
        }
        return serializeDate(returnValue.getUTCFullYear(), returnValue.getUTCMonth() + 1, returnValue.getUTCDate());
    case "Month":
        var returnValue = new Date(getFocusValue(1), getFocusMonth(0) - 1);
        if (returnValue > _maximumDate) {
            returnValue = _maximumDate;
        } else if (returnValue < _minimumDate) {
            returnValue  = _minimumDate;
        }
        return serializeDate(returnValue.getUTCFullYear(), returnValue.getUTCMonth() + 1);
    case "DateTime":
    case "DateTimeLocal":
        var day = getFocusDateTime(0);
        var returnValue = new Date(day.getUTCFullYear(), day.getUTCMonth(), day.getUTCDate(), getFocusValue(1), getFocusValue(2));
        returnValue = new Date(returnValue.getTime() - _offset);
        if (returnValue > _maximumDate) {
            returnValue = _maximumDate;
        } else if (returnValue < _minimumDate) {
            returnValue  = _minimumDate;
        }
        return serializeDate(returnValue.getUTCFullYear(), returnValue.getUTCMonth() + 1, returnValue.getUTCDate(), getFocusValue(1), getFocusValue(2));
    case "Time":
        var returnValue = new Date(_initialDate.getUTCFullYear(), _initialDate.getUTCMonth(), _initialDate.getUTCDate(), getFocusValue(0), getFocusValue(1));
        if (returnValue > _maximumDate) {
            returnValue = _maximumDate;
        } else if (returnValue < _minimumDate) {
            returnValue  = _minimumDate;
        }
        return serializeDate(returnValue.getUTCFullYear(), returnValue.getUTCMonth() + 1, returnValue.getUTCDate(), returnValue.getUTCHours(), returnValue.getUTCMinutes());
    }
}

var ok = function () {
    window.setValueAndClosePopup(getResult(_type), window.popUp);
}

var cancel = function () {
    window.setValueAndClosePopup("-1", window.popUp);
}

var getWeekDay = function (year, month, day) {
    var newDate = new Date(year, month - 1, day);
    return newDate.getUTCDay();
}

var checkLimits = function () {
    // Hard limits of type=date. See WebCore/platform/DateComponents.h.
    _minimumDate = new Date(-62135596800000.0);
    _maximumDate = new Date(8640000000000000.0);
}

var getCoords = function (e) {
    if (_moveStart) {
        x = e.pageX;
        y = e.pageY;
        offsetX = _lastPosX - x;
        offsetY = _lastPosY - y;

        for (i = 0; i < _numColumns; i++) {
            if ((_lastPosX > (0 + i * _viewportWidth / _numColumns)) && (_lastPosX < (0 + (i + 1) * _viewportWidth / _numColumns))) {
                _currentColumn = i;
                break;
            }
        }
        // Only scroll in desigated area
        if ( y < ROW_HEIGHT * (_numRows)) {
            scrollColumn(offsetY, _currentColumn);
        }
        _lastPosX = x;
        _lastPosY = y;
    }
}

var reAlignRows = function() {
    for (j = 0; j < _numColumns; j++) {
        var topCellPos = _numRows * ROW_HEIGHT;
        for (i = 0; i < _numRows; i ++) {
            var celldiv = document.getElementById(CELL_ID_PREFIX + i + j);
            var celldivPos = parseInt(celldiv.style.top);
            if (celldivPos < topCellPos) {
                topCellPos = celldivPos;
             }
        }
        if (topCellPos != 0) {
            scrollColumn(topCellPos, j);
        }
    }
}

var getFocusValue = function (column) {
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + column);
        celldivPos = parseInt(celldiv.style.top);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            return parseInt(celldiv.innerHTML);
        }
    }
}

var getFocusMonth = function (column) {
    var result;
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + column);
        celldivPos = parseInt(celldiv.style.top);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            var monthNum = document.getElementById("monthNum" + i);
            result = parseInt(monthNum.innerHTML);
        }
    }
    return result;
}

var getFocusDay = function (column) {
    var result;
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + column);
        celldivPos = parseInt(celldiv.style.top);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            var dayNum = document.getElementById("dayNum" + i);
            result = parseInt(dayNum.innerHTML);
        }
    }
    return result;
}

var getFocusDateTime = function(column) {
    var result;
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + column);
        celldivPos = parseInt(celldiv.style.top);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            var dayNum = document.getElementById("dayNum" + i);
            var parts = dayNum.innerHTML.split(' ');
            result = new Date(_currentDate.getUTCFullYear(), getMonthFromString(parts[1]), parseInt(parts[0]));
        }
    }
    return result;
}

var getMonthFromString = function(str) {
    return new Date('1 ' + str + ' 2012').getUTCMonth();
}

var scrollColumn = function (offsetY, column) {
    var focusYear, focusMonth;
    var columnType = COLUMN_TYPES[_type][column];
    if (columnType === DAY) {
        focusYear = getFocusValue(2);
        focusMonth = getFocusMonth(1);
    }
    for (i = 0; i < _numRows; i ++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + column);
        var celldivPos = parseInt(celldiv.style.top);
        celldivPos = celldivPos - offsetY;
        if (celldivPos < 0) {
            celldiv.style.top = (_numRows * ROW_HEIGHT + celldivPos) + "px";
            switch (columnType) {
            case YEAR:
                celldiv.innerHTML = parseInt(celldiv.innerHTML) + _numRows;
                break;
            case HOUR:
            case MIN:
                celldiv.innerHTML = getValidNumber(columnType, parseInt(celldiv.innerHTML) + _numRows);
                break;
            case MONTH:
                var monthNum = document.getElementById("monthNum" + i);
                monthNum.innerHTML = getValidNumber(MONTH, parseInt(monthNum.innerHTML) + _numRows - 1) + 1;
                var monthLabel = document.getElementById("monthLabel" + i);
                monthLabel.innerHTML = MONTH_FULL_LABELS[parseInt(monthNum.innerHTML) - 1];
                break;
            case DAY:
                var dayNum = document.getElementById("dayNum" + i);
                dayNum.innerHTML = getValidDayNumber(parseInt(dayNum.innerHTML) + _numRows, focusMonth, focusYear);
                break;
            case DATETIME:
                var dayNum = document.getElementById("dayNum" + i);
                var parts = dayNum.innerHTML.split(' ');
                var newDay = new Date(_currentDate.getUTCFullYear(), getMonthFromString(parts[1]), parseInt(parts[0]));
                newDay.setTime(newDay.getTime() + DAY_IN_MILLIS * _numRows);
                dayNum.innerHTML = newDay.getUTCDate() + " " + MONTH_LABELS[newDay.getUTCMonth()];
                var dayLabel = document.getElementById("dayLabel" + i);
                dayLabel.innerHTML = DAYS_OF_THE_WEEK[newDay.getUTCDay()];
                _currentDate = newDay;
                break;
            }
        } else if (celldivPos > ROW_HEIGHT * _numRows) {
            celldiv.style.top = (celldivPos - _numRows * ROW_HEIGHT) + "px";
            switch (COLUMN_TYPES[_type][column]) {
            case YEAR:
                celldiv.innerHTML = parseInt(celldiv.innerHTML) - _numRows;
                break;
            case HOUR:
            case MIN:
                celldiv.innerHTML = getValidNumber(columnType, parseInt(celldiv.innerHTML) - _numRows);
                break;
            case MONTH:
                var monthNum = document.getElementById("monthNum" + i);
                monthNum.innerHTML = getValidNumber(MONTH, parseInt(monthNum.innerHTML) - _numRows - 1) + 1;
                var monthLabel = document.getElementById("monthLabel" + i);
                monthLabel.innerHTML = MONTH_FULL_LABELS[parseInt(monthNum.innerHTML) - 1];
                break;
            case DAY:
                var dayNum = document.getElementById("dayNum" + i);
                dayNum.innerHTML = getValidDayNumber(parseInt(dayNum.innerHTML) - _numRows, focusMonth, focusYear);
                break;
            case DATETIME:
                var dayNum = document.getElementById("dayNum" + i);
                var parts = dayNum.innerHTML.split(' ');
                var newDay = new Date(_currentDate.getUTCFullYear(), getMonthFromString(parts[1]), parseInt(parts[0]));
                newDay.setTime(newDay.getTime() - DAY_IN_MILLIS * _numRows);
                dayNum.innerHTML = newDay.getUTCDate() + " " + MONTH_LABELS[newDay.getUTCMonth()];
                var dayLabel = document.getElementById("dayLabel" + i);
                dayLabel.innerHTML = DAYS_OF_THE_WEEK[newDay.getUTCDay()];
                _currentDate = newDay;
                break;
            }
        } else {
            celldiv.style.top = celldivPos + "px";
        }
    }
    switch (columnType) {
        case MONTH:
            updateMonthColumn();
            break;
        case DAY:
            updateDayColumn();
            break;
        case DATETIME:
            updateDatetimeColumn();
            break;
        default:
            break;
    }
}

var updateMonthColumn = function() {
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + _currentColumn);
        celldivPos = parseInt(celldiv.style.top);
        var monthLabel = document.getElementById("monthLabel" + i);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            monthLabel.style.visibility = "visible";
        } else {
            monthLabel.style.visibility = "hidden";
        }
    }
}

var updateDayColumn = function() {
    var focusYear, focusMonth;
    focusYear = getFocusValue(2);
    focusMonth = getFocusMonth(1);
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + _currentColumn);
        celldivPos = parseInt(celldiv.style.top);
        var dayLabel = document.getElementById("dayLabel" + i);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            var dayNum = document.getElementById("dayNum" + i);
            var weekDay = getWeekDay(focusYear, focusMonth, parseInt(dayNum.innerHTML));
            dayLabel.innerHTML = DAYS_OF_THE_WEEK[weekDay];
            dayLabel.style.visibility = "visible";
        } else {
            dayLabel.style.visibility = "hidden";
        }
    }
}

var updateDatetimeColumn = function() {
    for (i = 0; i < _numRows; i ++) {
        celldiv = document.getElementById(CELL_ID_PREFIX + i + _currentColumn);
        celldivPos = parseInt(celldiv.style.top);
        var dayLabel = document.getElementById("dayLabel" + i);
        if ((celldivPos >= ROW_HEIGHT * Math.floor(_numRows / 2)) && (celldivPos < ROW_HEIGHT * (Math.floor(_numRows / 2) + 1))) {
            dayLabel.style.visibility = "visible";
        } else {
            dayLabel.style.visibility = "hidden";
        }
    }
}


var initialCoords = function (e) {
    _moveStart = true;
    _lastPosX = e.pageX;
    _lastPosY = e.pageY;
}

var stopCalculation = function (e) {
    getCoords(e);
    _moveStart = false;
    reAlignRows();
}

var layout = function () {
    var popup = document.createElement("div");
    popup.className = "popup-area";
    popup.id = "popup-area";
    select = document.createElement("div");
    select.className = "select-area";
    select.id = "select-area";
    select.style.height = _numRows * ROW_HEIGHT + "px";
    popup.appendChild(select);

    var columnWidth = _viewportWidth / _numColumns;

    for (i = 0; i < _numRows; i++) {
        for (j = 0; j < _numColumns; j++) {
            var option = document.createElement("div");
            option.className = "cell";
            option.style.top = 0 + ROW_HEIGHT * i + "px";
            option.style.left = 0 + columnWidth * j + "px";
            option.style.width = columnWidth + "px";
            option.setAttribute("id", CELL_ID_PREFIX + i + j);
            select.appendChild(option);
        }
    }
    var focusRow = document.createElement("div");
    focusRow.className = "focusRow";
    focusRow.style.top = Math.floor(_numRows / 2) * ROW_HEIGHT + "px";
    select.appendChild(focusRow);

    var okButton = document.createElement("button"),
        cancelButton = document.createElement('button'),
        buttons = document.createElement("div"),
        buttonDivider = document.createElement("div");
    buttons.className = "popup-buttons";
    buttons.style.top = _numRows * ROW_HEIGHT + BORDER_PADDING + "px";

    okButton.className = "popup-button";
    okButton.style.left = _viewportWidth / 2 + 2 + "px";
    okButton.addEventListener("click", ok);
    okButton.appendChild(document.createTextNode("OK"));
    cancelButton.className = "popup-button";
    cancelButton.appendChild(document.createTextNode("Cancel"));
    cancelButton.addEventListener("click", cancel);

    buttonDivider.className = "popup-button-divider";
    buttons.appendChild(cancelButton);
    buttons.appendChild(buttonDivider);
    buttons.appendChild(okButton);
    popup.appendChild(buttons);
    document.body.appendChild(popup);

    document.addEventListener("mousedown", initialCoords, true);
    document.addEventListener("mousemove", getCoords, true);
    document.addEventListener("mouseup", stopCalculation, true);
}

var getValidNumber = function (type, num) {
    var length;
    switch (type) {
        case MONTH:
            length = 12;
            break;
        case WEEK:
            length = 7;
            break;
        case HOUR:
            length = 24;
            break;
        case MIN:
            length = 60;
            break;
    }

    if (num < 0) {
        num += length;
    } else if (num > (length - 1)) {
        num -= length;
    }
    return num;
}

var getValidDayNumber = function (num, month, year) {
    var d = new Date(year, month, 0).getDate();
    if (num < 1) {
        num += d;
    } else if (num > d){
        num -= d;
    }
    return num;
}

var formatYearColumn = function (columnNum) {
    for (i = 0; i < _numRows; i++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + columnNum);
        celldiv.innerHTML = _initialDate.getUTCFullYear() - Math.floor(_numRows / 2) + i ;
    }
}

var formatHourColumn = function (columnNum) {
    for (i = 0; i < _numRows; i++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + columnNum);
        celldiv.innerHTML = getValidNumber(HOUR, _initialDate.getUTCHours() - Math.floor(_numRows / 2) + i) ;
    }
}

var formatMinuteColumn = function (columnNum) {
    for (i = 0; i < _numRows; i++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + columnNum);
        celldiv.innerHTML = getValidNumber(MIN, _initialDate.getUTCMinutes() - Math.floor(_numRows / 2) + i) ;
    }
}

var formatDatetimeColumn = function (columnNum) {
    for (i = 0; i < _numRows; i++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + columnNum);
        var dayNum = document.createElement("div");

        dayNum.setAttribute("id", "dayNum" + i);
        var newDay = new Date(_initialDate.getTime() + (i - Math.floor(_numRows / 2)) * DAY_IN_MILLIS);
        dayNum.appendChild(document.createTextNode( newDay.getUTCDate() + " " + MONTH_LABELS[newDay.getUTCMonth()]));
        celldiv.appendChild(dayNum);

        var dayLabel = document.createElement("div");
        dayLabel.className = "label";
        dayLabel.setAttribute("id", "dayLabel" + i);
        dayLabel.appendChild(document.createTextNode(DAYS_OF_THE_WEEK[newDay.getUTCDay()]));
        celldiv.appendChild(dayLabel);
        if (i == Math.floor(_numRows / 2)) {
            dayLabel.style.visibility = "visible";
        } else {
            dayLabel.style.visibility = "hidden";
        }
    }
}

var formatMonthColumn = function (columnNum) {
    for (i = 0; i < _numRows; i++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + columnNum);
        var monthNum = document.createElement("div");

        monthNum.setAttribute("id", "monthNum" + i);
        monthNum.appendChild(document.createTextNode(getValidNumber(MONTH, _initialDate.getUTCMonth() - Math.floor(_numRows / 2) + i) + 1));
        celldiv.appendChild(monthNum);

        var monthLabel = document.createElement("div");
        monthLabel.className = "label";
        monthLabel.setAttribute("id", "monthLabel" + i);
        monthLabel.appendChild(document.createTextNode(MONTH_FULL_LABELS[parseInt(monthNum.innerHTML) - 1]));
        celldiv.appendChild(monthLabel);
        if (i == Math.floor(_numRows / 2)) {
            monthLabel.style.visibility = "visible";
        } else {
            monthLabel.style.visibility = "hidden";
        }
    }
}

var formatDayColumn = function (columnNum) {
    for (i = 0; i < _numRows; i ++) {
        var celldiv = document.getElementById(CELL_ID_PREFIX + i + columnNum);
        var dayNum = document.createElement("div");

        dayNum.setAttribute("id", "dayNum" + i);
        dayNum.appendChild(document.createTextNode(getValidDayNumber(_initialDate.getUTCDate() - Math.floor(_numRows / 2) + i, _initialDate.getUTCMonth() + 1, _initialDate.getUTCFullYear())));
        celldiv.appendChild(dayNum);

        var dayLabel = document.createElement("div");
        dayLabel.className = "label";
        dayLabel.setAttribute("id", "dayLabel" + i);
        dayLabel.appendChild(document.createTextNode(DAYS_OF_THE_WEEK[getValidNumber(WEEK, _initialDate.getUTCDay() - Math.floor(_numRows / 2) + i)]));
        celldiv.appendChild(dayLabel);
        if (i == Math.floor(_numRows / 2)) {
            dayLabel.style.visibility = "visible";
        } else {
            dayLabel.style.visibility = "hidden";
        }
    }
}
var fillTables = function (type) {
    switch (type) {
    case "Date":
        formatDayColumn(0);
        formatMonthColumn(1);
        formatYearColumn(2);
        break;
    case "Month":
        formatMonthColumn(0);
        formatYearColumn(1);
        break;
    case "DateTime":
    case "DateTimeLocal":
       formatDatetimeColumn(0);
       formatHourColumn(1);
       formatMinuteColumn(2);
       break;
    case "Time":
       formatHourColumn(0);
       formatMinuteColumn(1);
    }
}

var removeChildren = function (child) {
    while (child.hasChildNodes()) {
        var lastChild = child.lastChild;
        removeChildren(lastChild);
        child.removeChild(lastChild);
    }
}

var checkOrientation = function () {
    if (window.orientation !== _previousOrientation) {
        _previousOrientation = window.orientation;
        _viewportWidth = document.body.clientWidth;
        _viewportHeight = document.body.clientHeight;
        if (_viewportHeight > 768) {
            _numRows = 7;
        } else {
            _numRows = 3;
        }
        var popup = document.getElementById("popup-area");
        removeChildren(popup);
        document.body.removeChild(popup);
        layout();
        formatLabels();
        fillTables(_type);
    }
}

var show = function (typeIn, current, minDate, maxDate, step) {
    // FIXME: step is not supported in this initial version.
    _type = typeIn;
    switch (_type) {
    case "Date":
    case "DateTime":
    case "DateTimeLocal":
        _numColumns = 3;
        break;
    case "Time":
    case "Month":
        _numColumns = 2;
        break;
    default:
        _numColumns = 3;
    }
    // FIXME: it fails to get correct window size at loading JS, so has to hard code window's padding.
    _viewportWidth = screen.width - WINDOW_PADDING;
    _viewportHeight = screen.height - WINDOW_PADDING;
    if (_viewportHeight > 768) {
        _numRows = 7;
    } else {
        _numRows = 3;
    }
    checkLimits();
    _initialDate = parseDateString(current);

    if (_initialDate < _minimumDate) {
        _initialDate = _minimumDate;
    } else if (_initialDate > _maximumDate) {
        _initialDate = _maximumDate;
    }
    _currentDate = _initialDate;
    layout();
    formatLabels();
    fillTables(_type);
}
window.popupcontrol = window.popupcontrol || {};
window.popupcontrol.show = show;

window.addEventListener("resize", checkOrientation, false);
}());
