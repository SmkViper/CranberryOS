# Cranberry OS

Nothing terribly special here - just a toy OS (and really, not even that yet). Mostly using it to explore OS development and bare metal programming on the Raspberry Pi.

## Requirements
* Clang (with C++17 support)
* CMake (3.18 or newer)
* Raspberry Pi 3B (if you want to run on hardware)
* QEMU (for emulating/debugging)

There isn't currently any support for GCC or different Raspberry Pi boards.

## Building
The repository is set up for editing with VSCode and the CMake Tools extension from MS (which is what the cmake-kits.json file is for). Just select the "Clang RPi3 Baremetal" kit using the extension so it builds properly.

## Running
### QEMU
First, you'll have to grab the `bcm2710-rpi-3-b.dtb` file from the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware).

Then run QEMU with the following command:

`qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -dtb <path>/bcm2710-rpi-3-b.dtb`

(Assuming this is run from the `build\kernel` folder which contains the kernel image file)

### Real Hardware
* Create a microSD card formatted as the Raspberry Pi expects, with the `bootcode.bin`, `fixup.dat`, `start.elf`, `*.dtb` files, and `Overlays` folder - all from the `boot` folder in the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware).
* Copy the built `kernel8.img` from `build/kernel` onto the card.
* Insert the card into the Raspberry Pi
* Connect a USB-to-TTL cable to the Raspberry Pi and your PC
* Launch a terminal emulator and have it connect to the cable's COM port at `115200` speed.
  * Windows: PuTTY has been tested
  * Linux: `minicom -b 115200 -D /dev/ttyUSB0` (assumes the USB-to-TTL cable shows up as ttyUSB0)
* Power on the Raspberry Pi

## Debugging
Run the kernel in QEMU, but give it the `-s` option to enable a remote gdb server. You can also pass the `-S` option (in addition to the other one) to have QEMU halt on the very first instruction, waiting for you to connect.

Debugging has been tested in VSCode with the CodeLLDB extension. The following launch config will work:

```json
{
    "type": "lldb",
    "request": "custom",
    "name": "Connect to QEMU",
    "targetCreateCommands": [
        "target create ${workspaceFolder}/build/kernel/kernel8.elf"
    ],
    "processCreateCommands": [
        "gdb-remote <ip or localhost>:1234"
    ]
}
```

Note that trying to debug with LLDB on a Windows machine (using Win32 QEMU) will cause LLDB to be unable to find source files if built on a Linux box or WSL, due to a known issue with LLDB where it "sanitizes" all paths it receives for code mapping. This results in LLDB transforming all forward slashes to back slashes, which won't match the slashes in the DWARF data. To work around this, launch VSCode from WSL so that the Linux version of LLDB is used instead.

If you need to debug the boot process before the MMU is enabled, you'll need to tell the debugger to load the symbols at a different location. For example, to tell lldb to load symbols just for the specific boot assembly, use:

`target modules load --file <path>/kernel8.elf .text.boot 0x80000`

Where `<path>` is the path to the elf file (you can find it easily using the `image list` command).

## Contributing
For now, this is just a personal exploration project, so outside contributions are not being accepted.

## License
This project is licensed under the MIT License.

## Acknowledgements
Thank you to the following projects and websites for the huge amount of assistance and reference material (in no particular order):
* [OSDev Wiki](https://wiki.osdev.org/Expanded_Main_Page)
* [bztsrc's Raspberry Pi 3 bare metal github](https://github.com/bztsrc/raspi3-tutorial)
* [s-matyukevich's Raspberry Pi OS development github](https://github.com/s-matyukevich/raspberry-pi-os)
* [SerenityOS's rapidly progressing ARM port](https://github.com/SerenityOS/serenity)
* [lefticus' CMake template](https://github.com/cpp-best-practices/cmake_template)