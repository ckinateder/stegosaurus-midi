const { SerialPort } = require("serialport");
const readline = require("node:readline");

// List all available ports
const getPorts = async () => {
  const ports = await SerialPort.list();
  console.log("Available ports:");
  ports.forEach((port) => {
    console.log(port.path);
  });
};

const port = new SerialPort({
  path: "/dev/cu.usbmodem150734001",
  baudRate: 9600,
});

// Open errors will be emitted as an error event
port.on("error", function (err) {
  console.error("Error: ", err.message);
});

const send = async (message) => {
  await port.write(message + "\n", function (err) {
    if (err) return console.error("Error on write: ", err.message);
  });
};

// main loop

getPorts();
send("Hello, world!");
