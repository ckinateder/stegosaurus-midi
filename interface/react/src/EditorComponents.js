import { WebMidi } from "webmidi";
import React, { useState } from "react";
import { AppContext } from "./util.js";
import {
  sendSaveSlotMessage,
  sendSetSystemParamMessage,
  sendSysexMessage,
} from "./MidiControl.js";
import App from "./App.js";
import {
  TRIGGERS,
  ACTIONS,
  SWITCHES,
  MSG_TYPES,
  EnumValue,
  SYSTEM_PARAMS,
} from "./Enums.js";
/**
 * ADD
 * Dropdown for preset
 */

const channels = [...[...Array(16)].map((_, index) => index + 1), "all"];
const presets = [...Array(128)].map((_, index) => index);

const midiValues = [...Array(128)].map((_, index) => index);

export function Row({ slotNumber, presetNumber }) {
  const [trigger, setTrigger] = useState(TRIGGERS[0].value);
  const [action, setAction] = useState(ACTIONS[0].value);
  const [switchNum, setSwitchNum] = useState(SWITCHES[0].value);
  const [channel, setChannel] = useState(channels[0]);

  const [data1, setData1] = useState(midiValues[0]);
  const [data2, setData2] = useState(midiValues[0]);
  const [data3, setData3] = useState(midiValues[0]);

  return (
    <div
      style={{
        display: "flex",
        flexDirection: "row",
        gap: "12px",
      }}
    >
      <label htmlFor="select-trigger">Trigger: </label>
      <select
        id="select-trigger"
        value={trigger}
        onChange={(e) => {
          setTrigger(e.target.value);
        }}
      >
        {TRIGGERS.map((trigger, index) => (
          <option key={index} value={trigger.value}>
            {trigger.display}
          </option>
        ))}
      </select>

      <label htmlFor="select-action">Action: </label>
      <select
        id="select-action"
        value={action}
        onChange={(e) => {
          setAction(e.target.value);
        }}
      >
        {ACTIONS.map((action, index) => (
          <option key={index} value={action.value}>
            {action.display}
          </option>
        ))}
      </select>

      <label htmlFor="select-switch">Switch: </label>
      <select
        id="select-switch"
        value={switchNum}
        onChange={(e) => {
          setSwitchNum(e.target.value);
        }}
      >
        {SWITCHES.map((switchNum, index) => (
          <option key={index} value={switchNum.value}>
            {switchNum.display}
          </option>
        ))}
      </select>

      <label htmlFor="select-channel">Channel: </label>
      <select
        id="select-channel"
        value={channel}
        onChange={(e) => {
          setChannel(e.target.value);
        }}
      >
        {channels.map((channel, index) => (
          <option key={index}>{channel}</option>
        ))}
      </select>

      <label htmlFor="select-data-1">Data 1:</label>
      <select
        id="select-data-1"
        value={data1}
        onChange={(e) => {
          setData1(e.target.value);
        }}
      >
        {midiValues.map((midiValue, index) => (
          <option key={index}>{midiValue}</option>
        ))}
      </select>

      <label htmlFor="select-data-2">Data 2:</label>
      <select
        id="select-data-2"
        value={data2}
        onChange={(e) => {
          setData2(e.target.value);
        }}
      >
        {midiValues.map((midiValue, index) => (
          <option key={index}>{midiValue}</option>
        ))}
      </select>

      <label htmlFor="select-data-3">Data 3:</label>
      <select
        id="select-data-3"
        value={data3}
        onChange={(e) => {
          setData3(e.target.value);
        }}
      >
        {midiValues.map((midiValue, index) => (
          <option key={index}>{midiValue}</option>
        ))}
      </select>

      <button
        id="save-slot-button"
        type="button"
        onClick={(e) => {
          // send a sysex message to save the slot

          console.log(`Saving slot ${slotNumber} for preset ${presetNumber}`);
          sendSaveSlotMessage(
            presetNumber,
            slotNumber,
            trigger,
            action,
            channel,
            switchNum,
            data1,
            data2,
            data3
          );
        }}
      >
        Save
      </button>
      <label>Slot {slotNumber}</label>
    </div>
  );
}

export function DropdownGroup() {
  const [preset, setPreset] = useState(presets[0]);

  return (
    // add a dropdown for preset
    <div>
      <div
        id="preset-dropdown"
        style={{
          display: "flex",
          flexDirection: "row",
          gap: "12px",
          padding: "5px 0px",
        }}
      >
        <label htmlFor="select-preset">Preset: </label>
        <select
          id="select-preset"
          onChange={(e) => {
            // send the new preset value to the device
            const p = e.target.value;
            setPreset(p);
            sendSetSystemParamMessage(
              EnumValue(SYSTEM_PARAMS, "CurrentPreset"),
              p
            );
          }}
        >
          {presets.map((preset, index) => (
            <option key={index}>{preset}</option>
          ))}
        </select>
      </div>

      <div
        id="dropdown-group"
        style={{
          display: "flex",
          flexDirection: "column",
          gap: "12px",
          border: "1px solid black",
          width: "fit-content",
          padding: "12px",
        }}
      >
        {[...Array(16)].map((_, index) => (
          // remember, slot 0 is metadata
          <Row key={index} slotNumber={index + 1} presetNumber={preset} />
        ))}
      </div>
    </div>
  );
}
