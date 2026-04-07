<h2 align="center">AVRDUDE WebAssembly</h2>
<p align="center">Flash AVR microcontrollers directly from the browser via WebSerial and WebUSB</p>
<p align="center">
    <a href="#about">About</a> •
    <a href="#features">Features</a> •
    <a href="#use-cases">Use Cases</a> •
    <a href="#installation">Installation</a> •
    <a href="#usage">Usage</a> •
    <a href="#license">License</a>
</p>

## About

AVRDUDE WebAssembly is a port of [AVRDUDE](https://github.com/avrdudes/avrdude) (v8.1) compiled to WebAssembly, letting you program AVR microcontrollers straight from a web page — no drivers, no native binaries, no backend. Serial I/O is bridged to the browser's WebSerial and WebUSB APIs so any Chromium-based browser can flash a board.

## Features

- **Runs in the browser**: Full AVRDUDE functionality compiled to WebAssembly, no server or native helper needed.
- **WebSerial & WebUSB**: Native support for both transport layers, including FTDI devices via `@leaphy-robotics/webusb-ftdi`.
- **SharedArrayBuffer I/O**: Ring-buffered serial communication between the WASM module and a Web Worker for low-overhead data transfer.
- **Upstream v8.1**: Tracks the latest AVRDUDE release with all programmers and chip definitions.
- **ES6 module**: Ships as a modularized async factory (`MODULARIZE=1`, `EXPORT_ES6=1`) for easy bundler integration.
- **Broad board support**: Tested with Arduino Uno, Nano Every, ATmega boards, USBasp and more.
- **Docker build**: Reproducible build via Docker Compose — no local Emscripten toolchain required.

## Use Cases

- **Web-based IDEs**: Upload firmware from online editors like the Leaphy or Arduino web editors without a plugin.
- **Classroom & workshops**: Let students flash boards from a Chromebook or lab machine with zero install.
- **Firmware distribution**: Ship a one-click "flash this board" button on product or documentation pages.
- **Cross-platform tooling**: Avoid maintaining separate native AVRDUDE builds for Linux, macOS and Windows.
- **Kiosk & field updates**: Update devices in the field from a tablet or phone browser over USB-C.
- **Rapid prototyping**: Iterate on firmware without context-switching out of the browser.

## Installation

Prebuilt artifacts (`avrdude.js`, `avrdude-worker.js`, `avrdude.wasm`, `avrdude.conf`) are available as build artifacts from the [latest CI run](https://github.com/gaetanlhf/avrdude-webassembly/actions). Download the `avrdude-wasm` artifact, extract the four files next to your app, and import `avrdude.js` as an ES module.

### Build from source (Docker)

No local toolchain needed — just Docker.

```bash
git clone https://github.com/gaetanlhf/avrdude-webassembly.git
cd avrdude-webassembly
docker compose up build
```

Artifacts land in `./output/`.

### Build from source (local Emscripten)

Needs [emsdk](https://emscripten.org/docs/getting_started/downloads.html) (with `$EMSDK` set), `flex`, `bison`, and `yarn`.

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build --target avrdude
# or, to also copy the artifacts to the repo root:
yarn build
```

## Usage

`avrdude.js` is an async ES6 factory. The module takes the exact same CLI string you would pass to native `avrdude`, so any existing build pipeline translates directly.

### Minimal example — flash an ATtiny84 via USBasp

```js
import avrdudeJsUrl   from './avrdude.js';
import avrdudeWasmUrl from './avrdude.wasm';
import avrdudeConfUrl from './avrdude.conf';

// 1. Let the user pick the programmer (WebUSB for USBasp / USBtiny / AVR ISP mkII…)
const device = await navigator.usb.requestDevice({
  filters: [{ vendorId: 0x16c0, productId: 0x05dc }], // USBasp
});

// 2. Load the WASM factory and wire up logging
const logLines = [];
const { default: factory } = await import(/* webpackIgnore: true */ avrdudeJsUrl);
const Module = await factory({
  locateFile: (p) => p.endsWith('.wasm') ? avrdudeWasmUrl : p,
  print:    (t) => { logLines.push(t); console.log(t); },
  printErr: (t) => { logLines.push(t); console.warn(t); },
});

// 3. Drop avrdude.conf and the firmware into the virtual FS
const confText = await (await fetch(avrdudeConfUrl)).text();
const hexText  = await (await fetch('/firmware/main.hex')).text();
Module.FS.writeFile('/tmp/avrdude.conf', confText);
Module.FS.writeFile('/tmp/firmware.hex', hexText);

// 4. Call startAvrdude with the same arguments you'd use on the CLI
const startAvrdude = Module.cwrap('startAvrdude', 'number', ['string'], { async: true });
const exitCode = await startAvrdude(
  'avrdude -C /tmp/avrdude.conf -c usbasp -p t84 -v ' +
  '-U flash:w:/tmp/firmware.hex:i ' +
  '-U lfuse:w:0xe2:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m'
);
console.log(exitCode === 0 ? 'Flashed!' : `avrdude exited with ${exitCode}`);
```

### Serial programmer example — flash an ATmega328P via Arduino as ISP

For serial programmers (Arduino as ISP, Bus Pirate, AVR109/910 bootloaders…), request a port with WebSerial instead. The WASM module doesn't care about the real port name — `avrdude-worker.js` owns the connection — so just pass a placeholder (`/dev/null`) and the actual baud rate via `-P` / `-b`.

```js
import avrdudeJsUrl   from './avrdude.js';
import avrdudeWasmUrl from './avrdude.wasm';
import avrdudeConfUrl from './avrdude.conf';

// 1. Let the user pick a serial port
const port = await navigator.serial.requestPort();

// 2. Load the WASM factory
const { default: factory } = await import(/* webpackIgnore: true */ avrdudeJsUrl);
const Module = await factory({
  locateFile: (p) => p.endsWith('.wasm') ? avrdudeWasmUrl : p,
  print:    (t) => console.log(t),
  printErr: (t) => console.warn(t),
});

// 3. Write config + firmware to the virtual FS
Module.FS.writeFile('/tmp/avrdude.conf', await (await fetch(avrdudeConfUrl)).text());
Module.FS.writeFile('/tmp/firmware.hex',  await (await fetch('/firmware/blink.hex')).text());

// 4. Invoke avrdude — -P /dev/null is a placeholder, the worker uses the real port
const startAvrdude = Module.cwrap('startAvrdude', 'number', ['string'], { async: true });
const exitCode = await startAvrdude(
  'avrdude -C /tmp/avrdude.conf -c stk500v1 -P /dev/null -b 19200 ' +
  '-p atmega328p -v -U flash:w:/tmp/firmware.hex:i'
);
```

Swap `-c stk500v1 -b 19200` for `-c stk500v2 -b 115200` (Arduino Mega as ISP), `-c buspirate` (Bus Pirate), `-c avr109` / `-c avr910` (bootloaders), etc.

### Supported programmers

The full AVRDUDE programmer database is bundled, so any programmer is accepted by the `-c` flag. In a browser context, the two families that make sense are:

- **WebUSB** — USBasp (`16c0:05dc`), USBtinyISP (`1781:0c9f`), AVR ISP mkII (`03eb:2104`), AVR Dragon (`03eb:2107`), Atmel-ICE (`03eb:2141`), MPLAB Snap (`03eb:2180`), PICkit 4 / 5 (`03eb:2177` / `03eb:2184`), Xplained Mini / Pro, Power Debugger, CH341A, …
- **WebSerial** — Arduino as ISP (`stk500v1`, `stk500v2`), Bus Pirate, AVR910 / AVR109 bootloaders, and any serial-attached programmer.

Pick the device with `navigator.usb.requestDevice(...)` or `navigator.serial.requestPort()` before launching the WASM module; the bundled `avrdude-worker.js` then owns the connection and exchanges bytes with AVRDUDE through a SharedArrayBuffer ring — zero per-byte messaging overhead.

### Parsing progress in real time

AVRDUDE prints progress bars using `\r`, e.g. `Writing | ###...  | 45% 0.30 s`. Note that `print` / `printErr` callbacks may fire with partial lines or even single characters — accumulate output into a buffer and split on `/[\r\n]+/` only when parsing. Match the phase names (`Writing`, `Reading`, `Verifying`, `Caching`) to drive a progress UI. Look for `AVR device initialized`, `Processing -U <mem>:`, and `N bytes of <mem> verified` for step transitions.

## Documentation

Upstream AVRDUDE documentation is available [on GitHub Pages](https://avrdudes.github.io/avrdude/).

## License

AVRDUDE is distributed under the GNU General Public License version 2. This port preserves the original license — see the `COPYING` file for the full text.
