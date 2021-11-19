
let getHost_IP = new XMLHttpRequest();

getHost_IP.onload = function() {
    let host_IP_string_received = getHost_IP.responseText;
    let host_IP_stringDOM = document.getElementById('host_IP_string');
    host_IP_stringDOM.innerText = host_IP_string_received;

 //   console.log("Received text:" + host_IP_string_received);
 //   console.log(this.getResponseHeader("Content-Type"));
 //   console.log(this.getAllResponseHeaders());
}

getHost_IP.open('GET', 'http://monitor1/get_host_IP');
getHost_IP.send();


let getVoltages = new XMLHttpRequest(); // to receive data

getVoltages.onload = function () {
    let serverDataParsed = JSON.parse(getVoltages.responseText);

    // let serverData2 = getVoltages.responseText;

    console.log(serverDataParsed);
    console.log(this.getAllResponseHeaders());
    console.log(this.getResponseHeader("Content-Type"));

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

let count = 1;
let intervalID = setInterval(readVoltages, 100);

function readVoltages() {
 //   getVoltages.open('GET', 'http://192.168.0.29/data1');
    getVoltages.open('GET', 'http://monitor1/data1');
    getVoltages.send();

    console.log(count++);

    if (count > 3) {
        clearInterval(intervalID); 
    }
}

let postInstruction = new XMLHttpRequest(); // to send data

postInstruction.onload = function () {
 //   console.log("onload called after AJAX POST request");
}

// postInstruction.open('POST', 'http://monitor1/data1');
// postInstruction.setRequestHeader("Access-Control-Allow-Origin:*"); // not needed
// postInstruction.send("Rafal message Bloody hell 123456");

//console.log(window.location.host); // log address of server (HTTP)

function SettingsChangeCH1(value)
{
    let postInstruction222 = new XMLHttpRequest(); // to send data
    postInstruction222.open('POST', 'http://monitor1/data1');
    postInstruction222.send("Sending POST from SignalTypeChangeCH1 function");
}

function SettingsChangeCH2(value)
{
    alert(value + 99);
}

function SettingsChangeCH3(value)
{
    alert(value + 999);
}