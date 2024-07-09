import "./App.css";
import { WebMidi } from "webmidi";
import React, { useState } from "react";
import { AppContext } from "./util.js";
import { addListeners, sendMidiMessage } from "./MidiControl.js";
import { DropdownGroup } from "./KevinDidThisOnVacation.js";

function handleDeviceDropdownChange(e) {
  AppContext.input = WebMidi.getInputByName(e.target.value);
  AppContext.output = WebMidi.getOutputByName(e.target.value);
  addListeners(AppContext.input);
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
        onChange={handleDeviceDropdownChange}
      />
      <DropdownGroup />
      <h2>Monitor</h2>
      <div id="monitor-container"></div>
      <button id="send-button" onClick={sendMidiMessage}>
        Send
      </button>
    </div>
  );
}
