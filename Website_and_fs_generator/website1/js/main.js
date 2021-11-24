
let Setting_CH1;
let Setting_CH2;
let Setting_CH3;
let Relay1;
let Relay2;

// Data format for communication with server (settings only)
// First 3 characters - parameter
// Forth character - space
// Fifth, sixth, seventh character - value

let test1 = document.getElementById('CH1_setting');
// console.log(test1.value);
test1.value = -5;



// Get server IP block - begining ************************************************************************
let getHost_IP_LoopCount = 0;
let getHost_IP_LoopInterval = setInterval(getHost_IP_Loop, 500);
let getHost_IP = new XMLHttpRequest();
let receivedHost_IP_string = ""; // string
let host_IP_stringKnown = false;

function getHost_IP_Loop() {
    console.log("getHostIP attempt: " +getHost_IP_LoopCount);

    if(document.location.hostname != "monitor1" && document.location.hostname != "127.0.0.1") {  // If webside has been accessed not by "monitor1" name (presumably by IP from mobile)
                                                                                                 // then get IP from hostname (which will equal to IP) and return
        receivedHost_IP_string = document.location.hostname;
        clearInterval(getHost_IP_LoopInterval);
        host_IP_stringKnown = true;
        console.log("Test");
        return; 
    }

    getHost_IP.open('GET', 'http://monitor1/get_host_IP');
    getHost_IP.send();
    getHost_IP_LoopCount++;

    if(receivedHost_IP_string != "") {
        clearInterval(getHost_IP_LoopInterval);
        host_IP_stringKnown = true;
    }

    if(getHost_IP_LoopCount > 6 && receivedHost_IP_string == "") {
        clearInterval(getHost_IP_LoopInterval);
        alert("Communication error, refresh the page");
    };
}

getHost_IP.onload = function() {
    let host_IP_stringDOM = document.getElementById('host_IP_string');

    receivedHost_IP_string = getHost_IP.responseText;
    host_IP_stringDOM.innerText = 'To access this website from mobile use: http://' + receivedHost_IP_string;


        //else if(strncmp((char const *)buf,"GET /get_all_settings", 21) == 0)

        let getAllChannelSetttings = new XMLHttpRequest();

        getAllChannelSetttings.open('GET', 'http://' + receivedHost_IP_string + '/get_all_settings');
        getAllChannelSetttings.send();

        getAllChannelSetttings.onload = function() {
        let serverDataParsed = JSON.parse(getAllChannelSetttings.responseText);
        console.log(serverDataParsed);
        }


    // console.log("Attempts: " + getHost_IP_LoopCount);
    // console.log("IP string received inside: " + receivedHost_IP_string);
}
// Get server IP block - end ************************************************************************


// let text = "Hello world!";
// let result = text.substring(1, 4);













// Read server values and update document - begining ********************************************************************
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
// Read server values and update document - end ********************************************************************


//actual version
function SettingsChangeCH1(value)
{    //should I block here other asynchronous GET or POST requests to make sure this one is handled?

    console.log("test event");

    let postInstruction = new XMLHttpRequest();
    postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    postInstruction.send("CH1 " + value);
    console.log(value);

    postInstruction.onload = function()
    {
        console.log("Server responded: " + postInstruction.responseText);
        //alert(postInstruction.responseText);

        if("CH1 " + value == postInstruction.responseText) alert("Message delivered!");
    }


    // function makeFunc() {
    //     var name = 'Mozilla';

    //     function displayName() {
    //       alert(name);
    //     }
    //     return displayName;
    //   }
      


    //   var myFunc = makeFunc();
    //   myFunc();






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

function SettingsChangeRelay1(value)
{
    let postInstruction = new XMLHttpRequest();
    postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    postInstruction.send("Relay1 " + value);
    console.log(value);
}

function SettingsChangeRelay2(value)
{
    let postInstruction = new XMLHttpRequest();
    postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    postInstruction.send("Relay2 " + value);
    console.log(value);
}



// if (typeof browser === "undefined") {
//     var browser = chrome;
// }
// browser.downloads.download({url: "https://http://127.0.0.1:5500/index.html"});