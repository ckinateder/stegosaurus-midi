import "./App.css";
import { WebMidi } from "webmidi";
import React, { useState } from "react";

const AppContext = {};

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

function addListeners(selectedInput) {
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
    });
    selectedInput.addListener("controlchange", (e) => {
      //console.log(e);
      let number = e.data[1];
      let value = e.data[2];
      const info = `CC @ chan ${e.target.number}: ${number} ${value}`;
      console.log(info);
    });
    selectedInput.addListener("sysex", (e) => {
      const info = `sysex: ${processArray(e.dataBytes)}`;
      console.log(info);
    });
  } catch (e) {
    console.log(e);
  }
}

function DeviceDropdown({ devices, onChange }) {
  return (
    <select onChange={onChange}>
      {devices.map((device, index) => (
        <option key={index}>{device.name}</option>
      ))}
    </select>
  );
}

function handleDeviceDropdownChange(e) {
  AppContext.input = WebMidi.getInputByName(e.target.value);
  AppContext.output = WebMidi.getOutputByName(e.target.value);
  addListeners(AppContext.input);
}

function sendMidiMessage() {
  AppContext.output.sendProgramChange(28);
}

export default function App() {
  const [midiDevices, setMidiDevices] = useState([]);

  WebMidi.enable({ sysex: true })
    .then(() => {
      if (WebMidi.inputs.length < 1) {
        console.log("No device detected.");
      } else {
        setMidiDevices(WebMidi.inputs);
        AppContext.input = WebMidi.getInputByName(WebMidi.inputs[0].name);
        AppContext.output = WebMidi.getOutputByName(WebMidi.inputs[0].name);
        addListeners(AppContext.input);
      }
    })
    .catch((err) => console.log(err));

  return (
    <div id="main-page">
      <h1>Stegosaurus Editor</h1>
      <label htmlFor="select-device">Select a device: </label>
      <DeviceDropdown
        id="select-device"
        devices={midiDevices}
        handleDeviceDropdownChange={handleDeviceDropdownChange}
      />
      <h2>Monitor</h2>
      <div id="monitor-container"></div>
      <button id="send-button" onClick={sendMidiMessage}>
        Send
      </button>
    </div>
  );
}
