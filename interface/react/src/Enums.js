export const ALL_CHANNELS = 16;
export const TRIGGERS = [
  { name: "EnterPreset", value: 1, display: "Enter Preset" },
  { name: "ExitPreset", value: 2, display: "Exit Preset" },
  { name: "ShortPress", value: 3, display: "Short Press" },
  { name: "LongPress", value: 4, display: "Long Press" },
  { name: "DoublePress", value: 5, display: "Double Press" },
];

export const ACTIONS = [
  { name: "ControlChange", value: 1, display: "Control Change" },
  { name: "ProgramChange", value: 2, display: "Program Change" },
  { name: "PresetUp", value: 3, display: "Preset Up" },
  { name: "PresetDown", value: 4, display: "Preset Down" },
];

export const SWITCHES = [
  { name: "SWA", value: 0, display: "Switch A" },
  { name: "SWB", value: 1, display: "Switch B" },
  { name: "SWC", value: 2, display: "Switch C" },
  { name: "SWD", value: 3, display: "Switch D" },
];

export const MSG_TYPES = [
  { name: "SetSlot", value: 0, display: "Set Slot" },
  { name: "GetSlot", value: 1, display: "Get Slot" },
  { name: "GetSlotReturn", value: 2, display: "Get Slot Return" },
  { name: "SetSystemParam", value: 3, display: "Set System Param" },
  { name: "GetSystemParam", value: 4, display: "Get System Param" },
  {
    name: "GetSystemParamReturn",
    value: 5,
    display: "Get System Param Return",
  },
  { name: "GetPreset", value: 6, display: "Get Preset" },
  { name: "GetPresetReturn", value: 7, display: "Get Preset Return" },
];

export const SYSTEM_PARAMS = [
  { name: "CurrentPreset", value: 0, display: "Current Preset" },
];

export const EnumValue = (list, name) =>
  list.find((item) => item.name === name).value;
