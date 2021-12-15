import { TimepickerUI } from './timepicker-ui.js';
const DOMElement = document.querySelector('.timepicker-ui');
const options = {};
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
    document.addEventListener('mousedown', function () {
        saveSettings();
    });

    document.getElementById('brightnessRange').addEventListener('input', function () {
        this.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + this.value.toString();
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

    elementStreamingColor.value = jsonSettings.streamingColor;
    elementRecordingColor.value = jsonSettings.recordingColor;
    elementStreamingColor.parentElement.style.color = jsonSettings.streamingColor;
    elementRecordingColor.parentElement.style.color = jsonSettings.recordingColor;
    elementBrightnessRange.value = jsonSettings.brightnessRange;
    elementBrightnessRange.previousElementSibling.textContent = 'Aktuelle Helligkeit: ' + jsonSettings.brightnessRange;
}

async function saveSettings() {
    var jsonSettings = {};
    jsonSettings.streamingColor = document.getElementById('streamingColor').value;
    jsonSettings.recordingColor = document.getElementById('recordingColor').value;
    jsonSettings.brightnessRange = document.getElementById('brightnessRange').value;
    await makeRequest('/postSettings', 'POST', jsonSettings);
}