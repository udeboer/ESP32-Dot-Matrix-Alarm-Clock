var networkSettings = {
    name: "Esp32"
};
var espTime = {};
var espFiles = {};
var returnTxt;

//var tempJSON = 

async function jsonCall(objectToSend) {
    console.log('Json request is:', JSON.stringify(objectToSend));
    const response = await fetch('api/json/request', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(objectToSend),
    });
    returnTxt = await response.text();
    console.log('Responce with JSON:', response);
}

function jsonGetSettings() {
    console.log("This is the network setup receive function");
    networkSettings.RequestType = "SetupRead";
    jsonCall(networkSettings)
        .then(response => {
            console.log('Success', response);
            networkSettings = JSON.parse(returnTxt);
            console.log('VAR returned:', networkSettings);
            updateScreen();
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function jsonSetSettings() {
    console.log("This is the network settings send function");
    networkSettings.RequestType = "SetupSet";
    jsonCall(networkSettings)
        .then((response) => {
            console.log('Success:');
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function jsonGetTime() {
    console.log("This is the time receive function");
    espTime.RequestType = "TimeRead";
    jsonCall(espTime)
        .then(response => {
            console.log('Success', response);
            espTime = JSON.parse(returnTxt);
            console.log('Time returned:', espTime);
            updateTime();
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function jsonSetTime() {
    console.log("This is the time send function");
    espTime.RequestType = "TimeSet";
    jsonCall(espTime)
        .then((response) => {
            console.log('Success:');
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function jsonGetFiles() {
    console.log("This is the file listing receive function");
    espFiles = {};
    espFiles.RequestType = "FileList";
    espFiles.filesystem = "/www"
    jsonCall(espFiles)
        .then(response => {
            console.log('Success', response);
            espFiles = JSON.parse(returnTxt);
            console.log('File list returned:', espFiles);
            updateFiles();
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}


function jsonDeleteFile(fileToDelete) {
    console.log("This is the file delete function");
    espFiles = {};
    espFiles.RequestType = "DeleteFile";
    espFiles.filename = fileToDelete;
    jsonCall(espFiles)
        .then(response => {
            console.log('Success', response);
            espFiles = JSON.parse(returnTxt);
            console.log('Answer from delete file:', espFiles);
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function updateScreen() {
    document.getElementById("name").value = networkSettings.name;
    document.getElementById("formapssid").value = networkSettings.apssid;
    document.getElementById("formappw").value = networkSettings.appw;
    document.getElementById("formstassid").value = networkSettings.stssid;
    document.getElementById("formstapw").value = networkSettings.stpw;
    document.getElementById("timezone").value = networkSettings.timezone;
    document.getElementById("ntpserver").value = networkSettings.ntpserver;
    switchWifiMode(networkSettings.wifimode);
    addEvents();
}

function submitSetup() {
    var form = new FormData(document.getElementById("wifiform"));
    networkSettings.name = form.get("espname");
    networkSettings.apssid = form.get("apssid");
    networkSettings.appw = form.get("appw");
    networkSettings.apchannel = form.get("apch");
    networkSettings.stssid = form.get("stssid");
    networkSettings.stpw = form.get("stpw");
    networkSettings.timezone = form.get("timezone");
    networkSettings.ntpserver = form.get("ntpserver");
    console.log("This is the submitsetup function");
    jsonSetSettings();
}

function showPassword(inputObject) {
    if (inputObject.type === "password") {
        inputObject.type = "text";
    } else {
        inputObject.type = "password";
    }
}

function addEvents() {
    staPassword = document.getElementById("formstapw");
    staPassword.addEventListener("click", function () { showPassword(this) });
}

function switchWifiMode(wfmode) {
    console.log("nu in switch wifi");
    switch (wfmode) {
        case 1:
            document.getElementById('idsta').style.display = 'none';
            document.getElementById('idap').style.display = 'initial';
            document.getElementById('idapch').style.display = 'initial';
            document.getElementById('radio_ap').checked = true;
            break;
        case 2:
            document.getElementById('idap').style.display = 'initial';
            document.getElementById('idapch').style.display = 'none';
            document.getElementById('idsta').style.display = 'initial';
            document.getElementById('radio_apsta').checked = true;
            break;
        case 3:
            document.getElementById('idap').style.display = 'none';
            document.getElementById('idapch').style.display = 'none';
            document.getElementById('idsta').style.display = 'initial';
            document.getElementById('radio_sta').checked = true;
    }
    networkSettings.wifimode = wfmode;
}

function updateTime() {
    let espDate = new Date(espTime.utctimestamp * 1000);
    let browserDate = new Date();
    let timeLoc = document.getElementById('espUtcStamp');
    timeLoc.innerText = espTime.utctimestamp;
    timeLoc = document.getElementById('espUtcTime');
    timeLoc.innerText = espDate.toUTCString();
    timeLoc = document.getElementById('espLocalTime');
    timeLoc.innerText = espDate.toString();
    timeLoc = document.getElementById('systemTime');
    timeLoc.innerText = browserDate.toString();
}

function submitTime() {
    let browserDate = new Date();
    espTime.utctimestamp = Math.trunc(Date.now() / 1000);
    jsonSetTime();
    jsonGetTime();
}

function updateFiles() {
    //read espFiles an display in page
    let fileLoc = document.getElementById("fileList");
    fileLoc.innerHTML = "";
    row = document.createElement("div");
    row.className = "formRow fileTableHead";
    // add filename
    tempLoc = document.createElement("div");
    tempLoc.className = "filename";
    tempLoc.innerText = "File name";
    row.appendChild(tempLoc);
    // add filesize
    tempLoc = document.createElement("div");
    tempLoc.className = "filesize";
    tempLoc.innerText = "File size"
    row.appendChild(tempLoc);
    //add filedelete icon
    tempLoc = document.createElement("div");
    tempLoc.className = "filedelete";
    row.appendChild(tempLoc);
    fileLoc.appendChild(row)
    //sort directory listing
    let filesArray = espFiles.directory.slice(0);
    filesArray.sort(function (a, b) {
        var x = a.filename.toLowerCase();
        var y = b.filename.toLowerCase();
        return x < y ? -1 : x > y ? 1 : 0;
    });
    console.log("filessorted", filesArray);

    for (let rowCount = 0; rowCount < filesArray.length; rowCount++) {
        row = document.createElement("div");
        row.className = "formRow";
        // add filename
        tempLoc = document.createElement("div");
        tempLoc.className = "filename";
        tempLoc.innerText = filesArray[rowCount].filename;
        row.appendChild(tempLoc);
        // add filesize
        tempLoc = document.createElement("div");
        tempLoc.className = "filesize";
        tempLoc.innerText = filesArray[rowCount].filesize;
        row.appendChild(tempLoc);
        //add filedelete icon
        tempLoc = document.createElement("div");
        tempLoc.className = "filedelete";
        spanLoc = document.createElement("span");
        spanLoc.addEventListener("click", function () { deleteFileChoosen(this) })
        spanLoc.innerHTML = '&#10006';
        tempLoc.appendChild(spanLoc);
        row.appendChild(tempLoc);
        fileLoc.appendChild(row)
    }
}

function deleteFileChoosen(trashcanChoosen) {
    console.log("trashcan pressed");
    rowChoosen = trashcanChoosen.closest(".formRow");
    fileChoosen = rowChoosen.firstElementChild.innerText;
    console.log(fileChoosen);
    jsonDeleteFile(fileChoosen);
    jsonGetFiles();
}

function getFileChoice() {
    let slash = fileToUpload.value.lastIndexOf("\\");
    let filename;
    if (slash > 0) {
        filename = fileToUpload.value.slice(slash + 1);
    } else {
        filename = fileToUpload.value;
    }
    return filename;
}


function updateFileChoice() {
    console.log("File choosen");
    document.getElementById("fileUploadLabel").innerText = getFileChoice();
}

async function fileSend(fileNameToSend, FileToSend) {
    console.log('Filesend filename received is:', fileNameToSend);
    const response = await fetch('api/file_upload/www/' + fileNameToSend, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/octet-stream',
        },
        body: FileToSend,
    });
    returnTxt = await response.text();
    console.log('File send responce:', response);
}

function uploadFile() {
    console.log("Upload file is called");
    let filename = getFileChoice();
    console.log("File to send " + filename);
    fileSend(filename, fileToUpload.files[0])
        .then((response) => {
            console.log('Success in sending file: ' + returnTxt);
            jsonGetFiles();
        })
        .catch((error) => {
            console.error('Error:', error);
        });
    document.getElementById("fileUploadLabel").innerText = "Browse for files";
    fileToUpload.value = '';
}

function startSetup() {
    jsonGetSettings();
    jsonGetTime();
}

function startFiles() {
    jsonGetFiles();
}