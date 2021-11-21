
let getHost_IP = new XMLHttpRequest();
let receivedHost_IP_string ; // string
let host_IP_stringKnown = false;
let getHost_IP_LoopCount = 0;
let getHost_IP_LoopInterval = setInterval(getHost_IP_Loop, 500);


function getHost_IP_Loop() {

    getHost_IP.open('GET', 'http://monitor1/get_host_IP');
    getHost_IP.send();
    getHost_IP_LoopCount++;

    console.log("Attempts: " + getHost_IP_LoopCount);
    console.log("IP string received inside: " + receivedHost_IP_string);

    if(receivedHost_IP_string != undefined) {
        clearInterval(getHost_IP_LoopInterval);
        host_IP_stringKnown = true;
    }

    if(getHost_IP_LoopCount > 10 && receivedHost_IP_string == undefined) {
        clearInterval(getHost_IP_LoopInterval);
        alert("Communication error, refresh the page");
    };
}

getHost_IP.onload = function() {
    let host_IP_stringDOM = document.getElementById('host_IP_string');

    receivedHost_IP_string = getHost_IP.responseText;
    host_IP_stringDOM.innerText = 'To access this website from mobile use: http://' + receivedHost_IP_string;
}

let getVoltages = new XMLHttpRequest(); // to receive data
let getVoltagesLoopCount = 1;
let DataReadingInterval = setInterval(getVoltagesLoop, 300);

function getVoltagesLoop() {

    if(host_IP_stringKnown == false) return; // don't attempt to read data from server while IP is unknown

    getVoltages.open('GET', 'http://' + receivedHost_IP_string + '/data1');
    getVoltages.send();

    console.log(getVoltagesLoopCount++);

    if (getVoltagesLoopCount > 7) {
        clearInterval(DataReadingInterval); 
    }
}

getVoltages.onload = function () {
    let serverDataParsed = JSON.parse(getVoltages.responseText);
    console.log(serverDataParsed);

    let voltage1_ReadValue = serverDataParsed.voltage1;
    let voltage2_ReadValue = serverDataParsed.voltage2;
    let voltage3_ReadValue = serverDataParsed.voltage3;
    let temperature1_ReadValue = serverDataParsed.temperature1;
    let temperature2_ReadValue = serverDataParsed.temperature2;
    let relay1_ReadValue = serverDataParsed.relay1;
    let relay2_ReadValue = serverDataParsed.relay2;

    let voltage1DOM = document.getElementById('CH1_voltage');
    voltage1DOM.innerText = voltage1_ReadValue + " V";

    let voltage2DOM = document.getElementById('CH2_voltage');
    voltage2DOM.innerText = voltage2_ReadValue + " V";

    let voltage3DOM = document.getElementById('CH3_voltage');
    voltage3DOM.innerText = voltage3_ReadValue + " V";

    let temperature1DOM = document.getElementById('TC1_temp');
    temperature1DOM.innerText = temperature1_ReadValue + " °C";

    let temperature2DOM = document.getElementById('TC2_temp');
    temperature2DOM.innerText = temperature2_ReadValue + " °C";

    let relay1DOM = document.getElementById('Relay1');
    relay1DOM.innerText = relay1_ReadValue;

    let relay2DOM = document.getElementById('Relay2');
    relay2DOM.innerText = relay2_ReadValue;
}



function SettingsChangeCH1(value)
{    //should I block here other asynchronous GET or POST requests to make sure this one is handled?

    let postInstruction = new XMLHttpRequest();
    postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    postInstruction.send("CH1 " + value);
    console.log(value);
}

function SettingsChangeCH2(value)
{
    let postInstruction = new XMLHttpRequest();
    postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    postInstruction.send("CH2 " + value);
    console.log(value);
}

function SettingsChangeCH3(value)
{
    let postInstruction = new XMLHttpRequest();
    postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    postInstruction.send("CH3 " + value);
    console.log(value);
}