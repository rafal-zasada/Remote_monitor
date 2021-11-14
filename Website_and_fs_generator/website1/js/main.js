
let getVoltages = new XMLHttpRequest(); // to receive data

getVoltages.onload = function () {
    let serverDataParsed = JSON.parse(getVoltages.responseText);
    let serverData2 = getVoltages.responseText;

    console.log(serverDataParsed);

    let voltage1_ReadValue = serverDataParsed.voltage1;
    let voltage2_ReadValue = serverDataParsed.voltage2;
    let voltage3_ReadValue = serverDataParsed.voltage3;

    let voltage1DOM = document.getElementById('voltage1');
    voltage1DOM.innerText = "Voltage read from STM32 = " + voltage1_ReadValue;

    let voltage2DOM = document.getElementById('voltage2');
    voltage2DOM.innerText = "Voltage read from STM32 = " + voltage2_ReadValue;

    let voltage3DOM = document.getElementById('voltage3');
    voltage3DOM.innerText = "Voltage read from STM32 = " + voltage3_ReadValue;
}

let count = 1;
let intervalID = setInterval(readVoltages, 100);

function readVoltages() {
    getVoltages.open('GET', 'http://192.168.0.29/data1');
    getVoltages.send();

    console.log(count++);

    if (count > 20) {
        clearInterval(intervalID);
    }
}

let postInstruction = new XMLHttpRequest(); // to send data

postInstruction.onload = function () {
    console.log("onload called after AJAX POST request");
}

postInstruction.open('POST', 'http://192.168.0.29');
//postInstruction.setRequestHeader("Access-Control-Allow-Origin:*"); // not needed
postInstruction.send("Rafal message Bloody hell sgdfgdfgsdgdfsgdsfgdsgfgdfg");

console.log(window.location.host); // log address of server (HTTP)

