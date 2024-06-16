WebMidi.enable({ sysex: true })
  .then(onEnabled)
  .catch((err) => alert(err));

let selectedInput, selectedOutput, deviceDropdown;

function processArray(array) {
  let result = "[";
  for (let i = 0; i < array.length; i++) {
    result += array[i];
    if (i < array.length - 1) {
      result += ", ";
    }
  }
  result += "]";
  return result;
}

async function addListeners(selectedInput) {
  try {
    selectedInput.removeListener();
  } catch (e) {
    console.log("No listeners to remove");
  }
  try {
    console.log(`Adding listeners for ${selectedInput.name}`);
    selectedInput.addListener("programchange", (e) => {
      //console.log(e);
      const info = `PC @ chan ${e.target.number}: ${e.value}`;
      console.log(info);

      // add info to body of 'monitor' div
      document.getElementById("monitor").innerHTML += info + "<br>";
    });
    selectedInput.addListener("controlchange", (e) => {
      //console.log(e);
      let number = e.data[1];
      let value = e.data[2];
      const info = `CC @ chan ${e.target.number}: ${number} ${value}`;
      console.log(info);
      document.getElementById("monitor").innerHTML += info + "<br>";
    });
    selectedInput.addListener("sysex", (e) => {
      const info = `sysex: ${processArray(e.dataBytes)}`;
      console.log(info);
      document.getElementById("monitor").innerHTML += info + "<br>";
    });
  } catch (e) {
    console.log(e);
  }
}

function onEnabled() {
  // select dropdown with id select-device
  deviceDropdown = document.querySelector("#select-device");

  if (WebMidi.inputs.length < 1) {
    document.getElementById("monitor").innerHTML += "No device detected.";
  } else {
    // add options to dropdown
    WebMidi.inputs.forEach((device, index) => {
      let option = document.createElement("option");
      option.text = device.name;
      option.value = device.name;
      deviceDropdown.add(option);
    });

    deviceDropdown.addEventListener("change", (event) => {
      console.log("change event");
      selectedInput = WebMidi.getInputByName(deviceDropdown.value);
      selectedOutput = WebMidi.getOutputByName(deviceDropdown.value);
      addListeners(selectedInput);
    });

    // call change event to set up listeners
    deviceDropdown.dispatchEvent(new Event("change"));

    document.body.innerHTML += "<br>";
    // get the selected device from the dropdown
  }
}
