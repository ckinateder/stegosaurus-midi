import { WebMidi } from "webmidi";
import { processArray, AppContext } from "./util.js";
import { EnumValue, MSG_TYPES } from "./Enums.js";

const MFID = [0x00, 0x53, 0x4d];

export function addListeners(selectedInput) {
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

// make a function to send a sysex byte array
export function sendSysexMessage(byteArray) {
  AppContext.output.sendSysex(MFID, byteArray);
}

/**
 * Send a "Save Slot" message to the device
 * @param {*} presetNumber
 * @param {*} slotNumber
 * @param {*} trigger
 * @param {*} action
 * @param {*} channel
 * @param {*} switchNum
 * @param {*} data1
 * @param {*} data2
 * @param {*} data3
 */
export function sendSaveSlotMessage(
  presetNumber,
  slotNumber,
  trigger,
  action,
  channel,
  switchNum,
  data1,
  data2,
  data3
) {
  const byteArray = [
    EnumValue(MSG_TYPES, "SetSlot"),
    presetNumber,
    slotNumber,
    trigger,
    action,
    channel === "all" ? 16 : channel,
    switchNum,
    data1,
    data2,
    data3,
  ];
  sendSysexMessage(byteArray);
}

/**
 * Set a system parameter on the device
 * @param {*} param
 * @param {*} value
 */
export function sendSetSystemParamMessage(param, value) {
  const byteArray = [EnumValue(MSG_TYPES, "SetSystemParam"), param, value];
  sendSysexMessage(byteArray);
}

export function sendMidiMessage() {
  AppContext.output.sendProgramChange(28);
}
