


let Setting_CH1_DOM = document.getElementById('CH1_setting');
let Setting_CH2_DOM = document.getElementById('CH2_setting');
let Setting_CH3_DOM = document.getElementById('CH3_setting');
let Setting_Relay1_DOM = document.getElementById('Relay1_setting');
let Setting_Relay2_DOM = document.getElementById('Relay2_setting');
let Pulse_measurement_delay_DOM = document.getElementById('delay_setting');
let Watchdog_Status_DOM = document.getElementById('watchdog_on_off_button');
let Watchdog_Channel_DOM = document.getElementById('watchdog_channel');
let Watchdog_above_below_DEM = document.getElementById('watchdog_above_below');
let Watchdog_Threshold_DOM = document.getElementById('watchdog_threshold');
let Watchdog_Units_DOM = document.getElementById('watchdog_units');
let Watchdog_Action1_DOM = document.getElementById('watchdog_action1');
let Watchdog_Action2_DOM = document.getElementById('watchdog_action2');
let Watchdog_Email_DOM = document.getElementById('watchdog_email');




// Get server IP block - begining ************************************************************************
let getHost_IP_LoopCount = 0;
let getHost_IP_LoopInterval = setInterval(getHost_IP_Loop, 500);
let request_getHost_IP = new XMLHttpRequest();
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
        getAllChannelSettings();
        return; 
    }

    request_getHost_IP.open('GET', 'http://monitor1/get_host_IP');
    request_getHost_IP.send();
    getHost_IP_LoopCount++;

    if(receivedHost_IP_string != "") {
        clearInterval(getHost_IP_LoopInterval);
        host_IP_stringKnown = true;
        getAllChannelSettings();                        // should I try few times to make sure it is updated?
    }

    if(getHost_IP_LoopCount > 6 && receivedHost_IP_string == "") {
        clearInterval(getHost_IP_LoopInterval);
        alert("Communication error, refresh the page");
    };
}

request_getHost_IP.onload = function() {
    let host_IP_stringDOM = document.getElementById('host_IP_string');

    receivedHost_IP_string = request_getHost_IP.responseText;
    host_IP_stringDOM.innerText = 'To access this website from mobile use: http://' + receivedHost_IP_string;
}



function getAllChannelSettings() {
    let request_getAllChannelSetttings = new XMLHttpRequest();

    request_getAllChannelSetttings.open('GET', 'http://' + receivedHost_IP_string + '/get_all_settings');
    request_getAllChannelSetttings.send();

    request_getAllChannelSetttings.onload = function() {

    let serverDataParsed = JSON.parse(request_getAllChannelSetttings.responseText);

    console.log(serverDataParsed);
    Setting_CH1_DOM.value = serverDataParsed.Ch1_setting;
    Setting_CH2_DOM.value = serverDataParsed.Ch2_setting;
    Setting_CH3_DOM.value = serverDataParsed.Ch3_setting;
    Setting_Relay1_DOM.value = serverDataParsed.Relay1_setting;
    Setting_Relay2_DOM.value = serverDataParsed.Relay2_setting; 

    Pulse_measurement_delay_DOM.value = serverDataParsed.pulse_measurement_delay;
    Watchdog_Status_DOM.value = serverDataParsed.watchdogStatus;
    Watchdog_Channel_DOM.value = serverDataParsed.watchdogChannel;
    Watchdog_above_below_DEM.value = serverDataParsed.watchdogAboveBelow;
    Watchdog_Threshold_DOM.value = serverDataParsed.watchdogThreshold;
    Watchdog_Units_DOM.value = serverDataParsed.watchdogUnits;
    Watchdog_Action1_DOM.value = serverDataParsed.watchdogAction1;
    Watchdog_Action2_DOM.value = serverDataParsed.watchdogAction2;
    Watchdog_Email_DOM.value = serverDataParsed.Email_recepient;
    }
}

// let text = "Hello world!";
// let result = text.substring(1, 4);


// Read server values and update document - begining ********************************************************************
let request_getMonitorReadings = new XMLHttpRequest(); // to receive data
let getVoltagesLoopCount = 1;
let DataReadingInterval = setInterval(getMonitorReadingsLoop, 500);

function getMonitorReadingsLoop() {
    if(host_IP_stringKnown == false) return; // don't attempt to read data from server while IP is unknown

    request_getMonitorReadings.open('GET', 'http://' + receivedHost_IP_string + '/data1');
    request_getMonitorReadings.send();
    getVoltagesLoopCount++;

    //  console.log(getVoltagesLoopCount);

    // if (getVoltagesLoopCount > 1150) {
    //     clearInterval(DataReadingInterval); 
    // }
}

request_getMonitorReadings.onload = function () {
    let serverDataParsed = JSON.parse(request_getMonitorReadings.responseText);
    console.log(serverDataParsed);

    let voltage1_ReadValue = serverDataParsed.voltage1;
    let voltage2_ReadValue = serverDataParsed.voltage2;
    let voltage3_ReadValue = serverDataParsed.voltage3;
    let temperature1_ReadValue = serverDataParsed.temperature1;
    let temperature2_ReadValue = serverDataParsed.temperature2;
    let relay1_ReadValue = serverDataParsed.relay1;
    let relay2_ReadValue = serverDataParsed.relay2;

    document.getElementById('CH1_voltage').innerText = voltage1_ReadValue + " V";
    document.getElementById('CH2_voltage').innerText = voltage2_ReadValue + " V";
    document.getElementById('CH3_voltage').innerText = voltage3_ReadValue + " V";
    document.getElementById('TC1_temp').innerText = temperature1_ReadValue + " °C";
    document.getElementById('TC2_temp').innerText = temperature2_ReadValue + " °C";

    if(relay1_ReadValue == 0) {
        document.getElementById('Relay1').innerText = "Closed";
    }
    else {
        document.getElementById('Relay1').innerText = "Opened";
    }

    if(relay2_ReadValue == 1) {
        document.getElementById('Relay2').innerText = "Opened";
    }
    else {
        document.getElementById('Relay2').innerText = "Closed";
    }
}

			// Data format for communication with server (settings only)
			// First 3 characters - parameter
			// Forth character - space
			// Fifth, sixth, seventh character - value
function SettingsChangeCH1(value)
{   
    let valueTemp = document.getElementById('CH1_setting').value;
    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('CH1_setting').value = -9;          // blank select field by changing to non existent element 
    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    request_postInstruction.send("CH1 " + value);

    request_postInstruction.onload = function()
    {
        if("CH1 " + value == request_postInstruction.responseText) {
            document.getElementById('CH1_setting').value = valueTemp; // restore select field
            console.log("Setting confirmed!");
        } 
    }
}


function SettingsChangeCH2(value)
{   
    let valueTemp = document.getElementById('CH2_setting').value;
    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('CH2_setting').value = -9; // blank select field by changing to non existent element 
    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    request_postInstruction.send("CH2 " + value);

    request_postInstruction.onload = function()
    {
        if("CH2 " + value == request_postInstruction.responseText) {
            document.getElementById('CH2_setting').value = valueTemp; // restore select field
            console.log("Setting confirmed!");
        } 
    }
}


function SettingsChangeCH3(value)
{   
    let valueTemp = document.getElementById('CH3_setting').value;
    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('CH3_setting').value = -9; // blank select field by changing to non existent element 
    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    request_postInstruction.send("CH3 " + value);

    request_postInstruction.onload = function()
    {
        if("CH3 " + value == request_postInstruction.responseText) {
            document.getElementById('CH3_setting').value = valueTemp; // restore select field
            console.log("Setting confirmed!");
        } 
    }
}


function SettingsChangeRelay1(value)
{   
    let valueTemp = document.getElementById('Relay1_setting').value;
    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('Relay1_setting').value = -9; // blank select field by changing to non existent element 
    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    request_postInstruction.send("Re1 " + value);

    request_postInstruction.onload = function()
    {
        if("Re1 " + value == request_postInstruction.responseText) {
            document.getElementById('Relay1_setting').value = valueTemp; // restore select field
            console.log("Setting confirmed!");
        } 
    }
}


function SettingsChangeRelay2(value)
{   
    let valueTemp = document.getElementById('Relay2_setting').value;  
    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('Relay2_setting').value = -9; // blank select field by changing to non existent element 
    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    request_postInstruction.send("Re2 " + value);

    request_postInstruction.onload = function()
    {
        if("Re2 " + value == request_postInstruction.responseText) {
            document.getElementById('Relay2_setting').value = valueTemp; // restore select field
            console.log("Setting confirmed!");
        } 
    }
}


function SettingsChangeDelay(value)
{   
    let valueTemp = document.getElementById('watchdog_on_off_button').value;
    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('delay_setting').value = -9; // blank select field by changing to non existent element 
    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);
    request_postInstruction.send("DEL " + value);

    request_postInstruction.onload = function()
    {
        if("DEL " + value == request_postInstruction.responseText) {
            document.getElementById('delay_setting').value = valueTemp; // restore select field
            console.log("Setting confirmed!");
        } 
    }
}


function WatchdogEnableDisable()
{   
console.log (document.getElementById('watchdog_on_off_button').innerHTML);

    let request_postInstruction = new XMLHttpRequest();

    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);

    if(document.getElementById('watchdog_on_off_button').innerHTML === "&nbsp; disable &nbsp;")
    {
        console.log ("Test2");

        request_postInstruction.send("WAT SET 0");

        request_postInstruction.onload = function()
        {
            if("WAT SET 0" == request_postInstruction.responseText) {
                document.getElementById('watchdog_on_off_button').innerHTML = "&nbsp; enable &nbsp;";
                document.getElementById('watchdog_status_text').innerHTML = "&nbsp; DISABLED &nbsp;";
                console.log("Setting confirmed!");
            } 
        }
    }
    else if(document.getElementById('watchdog_on_off_button').innerHTML === "&nbsp; enable &nbsp;" || document.getElementById('watchdog_on_off_button').innerText === "&nbsp; re-enable &nbsp;")
    {
        console.log ("Test3");

        request_postInstruction.send("WAT SET 1");

        request_postInstruction.onload = function()
        {
            if("WAT SET 1" == request_postInstruction.responseText) {
                document.getElementById('watchdog_on_off_button').innerHTML = "&nbsp; disable &nbsp;"; 
                document.getElementById('watchdog_status_text').innerHTML = "&nbsp; ENABLED &nbsp;";
                console.log("Setting confirmed!");
            } 
        }
    }
}


function WatchdogSettingsModified()
{
    document.getElementById('watchdog_new_settings_status').innerHTML = "&nbsp; new settings not saved";
}


function WatchdogSaveSettings()
{
	// (0)WAT   (4)OPT   (8)0      (10)0    (12)123.44  (19)0   (21)0   (23)0     (25)zasi@poczta.onet.pl  (position in the string in brackets)
	// watchdog options channel above/below   value     units  action1  action2         email

    let WatchdogSettingsString = "WAT OPT" + " " +
                                 document.getElementById('watchdog_channel').value + " " +
                                 document.getElementById('watchdog_above_below').value + " " +
                                 document.getElementById('watchdog_threshold').value + " ";

    console.log(WatchdogSettingsString);
    console.log(WatchdogSettingsString.length);

    while(WatchdogSettingsString.length < 19)  
    {
        WatchdogSettingsString = WatchdogSettingsString + " ";  // fill with spaces to get correct string length
    }

    console.log(WatchdogSettingsString.length);

    WatchdogSettingsString = WatchdogSettingsString + document.getElementById('watchdog_units').value + " " +
                                                      document.getElementById('watchdog_action1').value + " " +
                                                      document.getElementById('watchdog_action2').value + " " +
                                                      document.getElementById('watchdog_email').value;

    console.log("units value = " + document.getElementById('watchdog_units').value);                                               
    console.log(WatchdogSettingsString);
    console.log(WatchdogSettingsString.length);



    let request_postInstruction = new XMLHttpRequest();

    document.getElementById('watchdog_new_settings_status').innerHTML = "&nbsp; no confirmation from server, try to save again";

    request_postInstruction.open('POST', 'http://' + receivedHost_IP_string);    
    request_postInstruction.send(WatchdogSettingsString);

    request_postInstruction.onload = function()
    {


        if(WatchdogSettingsString === request_postInstruction.responseText)
        {
            document.getElementById('watchdog_new_settings_status').innerHTML = "&nbsp; settings saved";

        }
    }

}


let inactivityWatchdog = function() {
    window.addEventListener('blur', setTimer);
    window.addEventListener('focus', resetTimer);
    
    let time;
    
    function setTimer() {
        time = setTimeout(logout, 600000);  // 10 minutes interval
    }
    
    function resetTimer() {
        clearTimeout(time);  
    }
    
    function logout() {
    alert("Updates stopped due to inactivity. Press OK to continue");
    }
}

inactivityWatchdog();









