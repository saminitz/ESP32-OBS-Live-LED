// Starts when the DOM Content is completely loaded
document.addEventListener('DOMContentLoaded', function () {
    loadSettings();
}, false);

document.addEventListener('mousedown', function () {
    saveSettings();
});

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

// Will load all colors and options
async function loadSettings() {
    var jsonString = await makeRequest('/getSettings', 'GET');
    var jsonSettings = JSON.parse(jsonString);
    var elementStreamingColor = document.getElementById('streamingColor');
    var elementRecordingColor = document.getElementById('recordingColor');

    elementStreamingColor.value = jsonSettings.streamingColor;
    elementRecordingColor.value = jsonSettings.recordingColor;
    elementStreamingColor.parentElement.style.color = jsonSettings.streamingColor;
    elementRecordingColor.parentElement.style.color = jsonSettings.recordingColor;
}

async function saveSettings() {
    var jsonSettings = {};
    jsonSettings.streamingColor = document.getElementById('streamingColor').value;
    jsonSettings.recordingColor = document.getElementById('recordingColor').value;
    await makeRequest('/postSettings', 'POST', jsonSettings);
}