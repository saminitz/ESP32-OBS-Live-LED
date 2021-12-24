import { TimepickerUI } from './timepicker-ui.js';
const DOMElement = document.querySelector('.timepicker-ui');
const options = { clockType: '24h', enableScrollbar: true };
const timepicker = new TimepickerUI(DOMElement, options);

// Starts when the DOM Content is completely loaded
document.addEventListener('DOMContentLoaded', function () {
    loadSettings();
    initializeObjectsAndEvents();
}, false);

function makeRequest(url, method, data) {
    return new Promise(function (resolve, reject) {
        let xhr = new XMLHttpRequest();
        xhr.open(method, url);
        xhr.onload = function () {
            if (this.status >= 200 && this.status < 300) {
                resolve(xhr.response);
            } else {
                reject({
                    status: this.status,
                    statusText: xhr.statusText
                });
            }
        };
        xhr.onerror = function () {
            reject({
                status: this.status,
                statusText: xhr.statusText
            });
        };
        if (data == undefined) {
            xhr.send();
        }
        else {
            xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
            xhr.send(JSON.stringify(data));
        }
    });
}

function initializeObjectsAndEvents() {
    document.addEventListener('mouseup', function () {
        if (document.getElementById('clr-picker').className.includes('clr-open')) {
            saveSettings();
        }
    });

    document.addEventListener('touchend', function () {
        if (document.getElementById('clr-picker').className.includes('clr-open')) {
            saveSettings();
        }
    });

    document.getElementById('brightnessRange').addEventListener('input', function () {
        this.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + this.value.toString();
        var secondSlider = document.getElementById('brightnessRange2');
        secondSlider.value = this.value;
        secondSlider.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + this.value.toString();
    });

    document.getElementById('brightnessRange2').addEventListener('input', function () {
        this.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + this.value.toString();
        var firstSlider = document.getElementById('brightnessRange');
        firstSlider.value = this.value;
        firstSlider.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + this.value.toString();
    });

    document.getElementById('brightnessRange').addEventListener('mouseup', function () {
        saveSettings();
    });

    document.getElementById('brightnessRange').addEventListener('touchend', function () {
        saveSettings();
    });

    document.getElementById('brightnessRange2').addEventListener('mouseup', function () {
        saveSettings();
    });

    document.getElementById('brightnessRange2').addEventListener('touchend', function () {
        saveSettings();
    });

    document.getElementById('useShutOffTime').addEventListener('change', function () {
        $('#clockDiv').toggle("fast");
        saveSettings();
    });

    document.getElementById('manualControl').addEventListener('change', function () {
        $('#manualColorDiv').toggle("fast");
        saveSettings();
    });

    document.getElementById('shutOffTime').addEventListener('change', function () {
        saveSettings();
    });

    timepicker.create();
}

// Will load all colors and options
async function loadSettings() {
    var jsonString = await makeRequest('/getSettings', 'GET');
    var jsonSettings = JSON.parse(jsonString);
    var elementStreamingColor = document.getElementById('streamingColor');
    var elementRecordingColor = document.getElementById('recordingColor');
    var elementBrightnessRange = document.getElementById('brightnessRange');
    var elementBrightnessRange2 = document.getElementById('brightnessRange2');
    var elementManualControl = document.getElementById('manualControl');
    var elementManualColorDiv = $('#manualColorDiv');
    var elementManualColor = document.getElementById('manualColor');
    var elementUseShutOffTime = document.getElementById('useShutOffTime');
    var elementClockDiv = $('#clockDiv');
    var elementShutOffTime = document.getElementById('shutOffTime');

    elementStreamingColor.value = jsonSettings.streamingColor;
    elementRecordingColor.value = jsonSettings.recordingColor;
    elementStreamingColor.parentElement.style.color = jsonSettings.streamingColor;
    elementRecordingColor.parentElement.style.color = jsonSettings.recordingColor;
    elementBrightnessRange.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + jsonSettings.brightnessIndex;
    elementBrightnessRange.value = jsonSettings.brightnessIndex;

    elementManualControl.checked = jsonSettings.manualControl;
    elementManualColorDiv.toggle(jsonSettings.manualControl);
    elementManualColor.value = jsonSettings.manualColor;
    elementManualColor.parentElement.style.color = jsonSettings.manualColor;
    elementBrightnessRange2.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + jsonSettings.brightnessIndex;
    elementBrightnessRange2.value = jsonSettings.brightnessIndex;

    elementUseShutOffTime.checked = jsonSettings.useShutOffTime;
    elementClockDiv.toggle(jsonSettings.useShutOffTime);
    elementShutOffTime.value = jsonSettings.shutOffTime.hour + ':' + jsonSettings.shutOffTime.minute;
}

async function saveSettings() {
    var jsonSettings = {};
    jsonSettings.streamingColor = document.getElementById('streamingColor').value;
    jsonSettings.recordingColor = document.getElementById('recordingColor').value;
    jsonSettings.brightnessIndex = parseInt(document.getElementById('brightnessRange').value);

    jsonSettings.manualControl = document.getElementById('manualControl').checked;
    jsonSettings.manualColor = document.getElementById('manualColor').value;
    jsonSettings.useShutOffTime = document.getElementById('useShutOffTime').checked;

    jsonSettings.shutOffTime = {
        hour: parseInt(document.getElementById('shutOffTime').value.split(':')[0]),
        minute: parseInt(document.getElementById('shutOffTime').value.split(':')[1])
    };
    await makeRequest('/postSettings', 'POST', jsonSettings);
}