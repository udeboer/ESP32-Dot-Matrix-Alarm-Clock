//Define global
var clockSettings = {};
var wavList = {};
var returnTxt;

//var tempJSON = '{"alarm_on":false,"default_on":true,"sleep_minutes":3,"brightness":4,"alarms":[{"hour":14,"minute":25,"day":0,"month":0,"weekday":0,"is_alarm":true,"sound":"bird1"},{"hour":20,"minute":58,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":21,"minute":2,"day":0,"month":0,"weekday":6,"is_alarm":false,"sound":"bird1"},{"hour":13,"minute":27,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":14,"minute":50,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":14,"minute":51,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":14,"minute":52,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":14,"minute":53,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":14,"minute":54,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":14,"minute":55,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":"bird1"},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""},{"hour":0,"minute":0,"day":0,"month":0,"weekday":0,"is_alarm":false,"sound":""}]}';
//var tempDataList = '{"wavs":[{"wavsound":"CardinalBird"},{"wavsound":"JollyLaugh"},{"wavsound":"SleighBells"},{"wavsound":"HappyBirthday"},{"wavsound":"ChurchBell"},{"wavsound":"bird1"}]}';

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
    console.log("This is the clock setup receive function");
    clockSettings.RequestType = "ClockRead";
    jsonCall(clockSettings)
        .then(response => {
            console.log('Success', response);
            clockSettings = JSON.parse(returnTxt);
            console.log('VAR returned:', clockSettings);
            clockSettings.RequestType = "";
            updateScreen();
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function jsonSetSettings() {
    console.log("This is the clock settings send function");
    clockSettings.RequestType = "ClockSet";
    jsonCall(clockSettings)
        .then((response) => {
            console.log('Success:');
            clockSettings.RequestType = "";
        })
        .catch((error) => {
            console.error('Error:', error);
        });
};

function fillWavDatalist(datalistID) {
    console.log("This is wav datalist fill function");
    wavList.RequestType = "WavList";
    jsonCall(wavList)
        .then(response => {
            console.log('Success Wav Sound List', response);
            wavList = JSON.parse(returnTxt);
            console.log('WavList returned:', wavList);
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function generateForm() {
    let settingLoc = document.getElementById("settingID");
    let alarmLoc = document.getElementById("alarmID");
    //alarm on
    let row = document.createElement("div");
    row.className = "formRow";
    let labelContainer = document.createElement("div");
    labelContainer.className = "labelContainer";
    let label = document.createElement("label");
    label.htmlFor = "alarm_on";
    label.innerHTML = "Alarm on"
    label.className = "settingLabel";
    labelContainer.appendChild(label);
    row.appendChild(labelContainer);
    settingLoc.appendChild(row);
    let inputContainer = document.createElement("div");
    inputContainer.className = "inputContainer";
    let input = document.createElement("input");
    input.type = "checkbox";
    input.name = "alarm_on";
    inputContainer.appendChild(input);
    let span = document.createElement("span");
    span.className = "checkTick";
    inputContainer.appendChild(span);
    row.appendChild(inputContainer);
    settingLoc.appendChild(row);
    //Default on
    row = document.createElement("div");
    row.className = "formRow";
    labelContainer = document.createElement("div");
    labelContainer.className = "labelContainer";
    label = document.createElement("label");
    label.htmlFor = "default_on";
    label.innerHTML = "Default on"
    label.className = "settingLabel";
    labelContainer.appendChild(label);
    row.appendChild(labelContainer);
    settingLoc.appendChild(row);
    inputContainer = document.createElement("div");
    inputContainer.className = "inputContainer";
    input = document.createElement("input");
    input.type = "checkbox";
    input.name = "default_on";
    inputContainer.appendChild(input);
    span = document.createElement("span");
    span.className = "checkTick";
    inputContainer.appendChild(span);
    row.appendChild(inputContainer);
    settingLoc.appendChild(row);
    //Sleep Minutes
    row = document.createElement("div");
    row.className = "formRow";
    labelContainer = document.createElement("div");
    labelContainer.className = "labelContainer";
    label = document.createElement("label");
    label.htmlFor = "sleep_minutes";
    label.innerHTML = "Sleep minutes"
    label.className = "settingLabel";
    labelContainer.appendChild(label);
    row.appendChild(labelContainer);
    settingLoc.appendChild(row);
    inputContainer = document.createElement("div");
    inputContainer.className = "inputContainer";
    input = document.createElement("input");
    input.type = "number";
    input.max = 60;
    input.min = 1;
    input.step = 1;
    input.pattern = "[0-9]{2}";
    input.name = "sleep_minutes";
    inputContainer.appendChild(input);
    row.appendChild(inputContainer);
    settingLoc.appendChild(row);
    //Brightness Minutes
    row = document.createElement("div");
    row.className = "formRow";
    labelContainer = document.createElement("div");
    labelContainer.className = "labelContainer";
    label = document.createElement("label");
    label.htmlFor = "brightness";
    label.innerHTML = "Brightness"
    label.className = "settingLabel";
    labelContainer.appendChild(label);
    row.appendChild(labelContainer);
    settingLoc.appendChild(row);
    inputContainer = document.createElement("div");
    inputContainer.className = "inputContainer";
    input = document.createElement("input");
    input.type = "number";
    input.max = 15;
    input.min = 0;
    input.step = 1;
    input.pattern = "[0-9]{2}";
    input.name = "brightness";
    inputContainer.appendChild(input);
    row.appendChild(inputContainer);
    settingLoc.appendChild(row);
    //add array
    for (let rowCount = 0; rowCount < clockSettings.alarms.length; rowCount++) {
        let alarmRow = document.createElement("div");
        alarmRow.className = "formRow";
        alarmRow.id = rowCount + "_alarmRow"
        //time
        let tabelContainer = document.createElement("div");
        tabelContainer.className = "time";
        input = document.createElement("input");
        input.type = "number";
        input.max = 24;
        input.min = 0;
        input.step = 1;
        input.pattern = "[0-9]{2}";
        input.name = "hour";
        tabelContainer.appendChild(input);
        tabelContainer.appendChild(document.createTextNode(":"));
        input = document.createElement("input");
        input.type = "number";
        input.max = 59;
        input.min = 0;
        input.step = 1;
        input.pattern = "[0-9]{2}";
        input.name = "minute";
        tabelContainer.appendChild(input);
        alarmRow.appendChild(tabelContainer);
        //date
        tabelContainer = document.createElement("div");
        tabelContainer.className = "date";
        input = document.createElement("input");
        input.type = "number";
        input.max = 31;
        input.min = 0;
        input.step = 1;
        input.pattern = "[0-9]{2}";
        input.name = "day";
        tabelContainer.appendChild(input);
        tabelContainer.appendChild(document.createTextNode("/"));
        input = document.createElement("input");
        input.type = "number";
        input.max = 12;
        input.min = 0;
        input.step = 1;
        input.pattern = "[0-9]{2}";
        input.name = "month";
        tabelContainer.appendChild(input);
        alarmRow.appendChild(tabelContainer);
        //weekday
        tabelContainer = document.createElement("div");
        tabelContainer.className = "weekday";
        input = document.createElement("input");
        input.type = "number";
        input.max = 7;
        input.min = 0;
        input.step = 1;
        input.pattern = "[0-7]{1}";
        input.name = "weekday";
        tabelContainer.appendChild(input);
        alarmRow.appendChild(tabelContainer);
        //isAlarm
        tabelContainer = document.createElement("div");
        tabelContainer.className = "isAlarm";
        input = document.createElement("input");
        input.type = "checkbox";
        input.name = "is_alarm";
        tabelContainer.appendChild(input);
        alarmRow.appendChild(tabelContainer);
        span = document.createElement("span");
        span.className = "checkTick";
        tabelContainer.appendChild(span);
        alarmRow.appendChild(tabelContainer);
        //Sound
        tabelContainer = document.createElement("div");
        tabelContainer.className = "sound";
        input = document.createElement("input");
        input.type = "text";
        input.name = "sound";
        input.autocomplete = "off";
        tabelContainer.appendChild(input);
        alarmRow.appendChild(tabelContainer);
        //Add row to form
        alarmLoc.appendChild(alarmRow);
        //end of array loop
    }
}

function addEvents() {
    // First row set disabled. Is standard alarm
    let rowInput = document.getElementById(("0_alarmRow")).getElementsByTagName("input");
    rowInput["day"].disabled = true;
    rowInput["month"].disabled = true;
    rowInput["weekday"].disabled = true;
    rowInput["is_alarm"].disabled = true;
    // Start of setting time readonly on alarm items already set
    // And add event listener
    for (let cnt = 1 ; cnt < clockSettings.alarms.length ; cnt ++){
        rowInput = document.getElementById((cnt.toString() + "_alarmRow")).getElementsByTagName("input");
        if (rowInput["is_alarm"].checked == true) {
            rowInput["hour"].readOnly = true;
            rowInput["minute"].readOnly = true;
        }
        rowInput["is_alarm"].addEventListener("click", function () { changeIsAlarm(this) });
    }
    //Event for dropdown menu
    for (let cnt = 0 ; cnt < clockSettings.alarms.length ; cnt ++){
        rowInput = document.getElementById((cnt.toString() + "_alarmRow")).getElementsByTagName("input");
        rowInput["sound"].parentElement.addEventListener("click", function () { customlist(this, wavList.wavs, "wavsound") });
    }
}

function customlist(inputContainer, arrayToList, itemName) {
    // Extra class dropDown is added to parent of input when clicked.
    // This function should be click event on text input
    inputContainer.classList.toggle("dropDown");
    // The next click closes the dropDown. Needs capture
    window.addEventListener("click", function (clickListEvent) {
        console.log("Globalevent triggered");
        let node = document.getElementById("listBox");
        if (node != null) {
            node.parentNode.removeChild(node);
        }
        // When clicked outside of input box remove .dropDown
        if (!clickListEvent.target.matches(".dropDown>input") && !clickListEvent.target.classList.contains("dropDown")) {
            console.log("Globalevent outside inputbox");
            node = document.querySelector(".dropDown");
            if (node != null) {
                console.log("Removing .dropDown class")
                node.classList.remove("dropDown");
            }
        }
    }, { capture: true, once: true });

    if (inputContainer.classList.contains("dropDown")) {
        input = inputContainer.querySelector("input");
        console.log("Started Custom list event handler");
        inputLocation = input.getBoundingClientRect();
        // create container for dropdown
        let listContainer = document.createElement("div");
        listContainer.style.position = "absolute";
        //here we add the amount we scrolled on the page and postion the dropdown list
        listContainer.style.top = (window.scrollY + inputLocation.bottom) + "px";
        listContainer.style.left = (window.scrollX + inputLocation.left) + "px";
        listContainer.style.width = inputLocation.width + "px";
        listContainer.id = "listBox";
        // Here we add the rows for the list. Tested with vertical flex layout
        for (let x = 0; x < arrayToList.length; x++) {
            let listRow = document.createElement("div");
            listRow.className = "listItem";
            listRow.textContent = arrayToList[x][itemName];
            // Add click function to get value from list
            listRow.addEventListener("click", function () {
                input.value = arrayToList[x][itemName];
            });
            listContainer.appendChild(listRow);
        }
        //add to body to have absolute position working
        document.body.appendChild(listContainer);
    }
}

function changeIsAlarm(checkBox) {
    // Toggle readonly on time when alarm
    const row = checkBox.parentElement.parentElement;
    const rowInput = row.getElementsByTagName("input");
    if (checkBox.checked == true) {
        rowInput["hour"].readOnly = true;
        rowInput["minute"].readOnly = true;
    } else {
        rowInput["hour"].readOnly = false;
        rowInput["minute"].readOnly = false;
    }

}

function setFormInput() {
    //not using formData. Does not work on table
    formInput = document.getElementById("clockForm");
    //document.getElementsByName("brightness").value = clockSettings.brightness; 
    formInput.elements["brightness"].value = clockSettings.brightness;
    formInput.elements["sleep_minutes"].value = clockSettings.sleep_minutes;
    if (clockSettings.default_on == true) {
        formInput.elements["default_on"].checked = true;
    }
    if (clockSettings.alarm_on == true) {
        formInput.elements["alarm_on"].checked = true;
    }
    // Lets set values to array
    for (let arrayRow = 0; arrayRow < clockSettings.alarms.length; arrayRow++) {
        //console.log("ArrayRow is " + arrayRow);
        rowInput = document.getElementById((arrayRow.toString() + "_alarmRow")).getElementsByTagName("input");
        rowInput["hour"].value = clockSettings.alarms[arrayRow].hour;
        rowInput["minute"].value = clockSettings.alarms[arrayRow].minute;
        rowInput["month"].value = clockSettings.alarms[arrayRow].month;
        rowInput["day"].value = clockSettings.alarms[arrayRow].day;
        rowInput["weekday"].value = clockSettings.alarms[arrayRow].weekday;
        rowInput["sound"].value = clockSettings.alarms[arrayRow].sound;
        if (clockSettings.alarms[arrayRow].is_alarm == true) {
            rowInput["is_alarm"].checked = true;
        }
        //end of looping through array
    }
}

function getFormInput() {
    //not using formData. Does not work on table
    formInput = document.getElementById("clockForm");
    clockSettings.brightness = Number(formInput.elements["brightness"].value);
    clockSettings.sleep_minutes = Number(formInput.elements["sleep_minutes"].value);
    if (formInput.elements["default_on"].checked == true) {
        clockSettings.default_on = true;
    } else {
        clockSettings.default_on = false;
    }
    if (formInput.elements["alarm_on"].checked == true) {
        clockSettings.alarm_on = true;
    } else {
        clockSettings.alarm_on = false;
    }
    // Lets add values to array
    for (let arrayRow = 0; arrayRow < clockSettings.alarms.length; arrayRow++) {
        //console.log("ArrayRow is " + arrayRow);
        rowInput = document.getElementById((arrayRow.toString() + "_alarmRow")).getElementsByTagName("input");
        clockSettings.alarms[arrayRow].hour = Number(rowInput["hour"].value);
        clockSettings.alarms[arrayRow].minute = Number(rowInput["minute"].value);
        clockSettings.alarms[arrayRow].sound = rowInput["sound"].value;
        if (arrayRow > 0) {
            //Row 0 has some disabled items
            clockSettings.alarms[arrayRow].month = Number(rowInput["month"].value);
            clockSettings.alarms[arrayRow].day = Number(rowInput["day"].value);
            clockSettings.alarms[arrayRow].weekday = Number(rowInput["weekday"].value);
            if (rowInput["is_alarm"].checked == true) {
                clockSettings.alarms[arrayRow].is_alarm = true;
            } else {
                clockSettings.alarms[arrayRow].is_alarm = false;
            }
        }
    }
}


function updateScreen() {
    //clockSettings = JSON.parse(tempJSON);
    //wavList = JSON.parse(tempDataList);
    fillWavDatalist();
    generateForm();
    setFormInput();
    addEvents();
}

function submitSetup() {
    console.log("This is the submitsetup function");
    getFormInput();
    console.log('VAR after submit:', clockSettings);
    jsonSetSettings();
}


window.onload = jsonGetSettings();