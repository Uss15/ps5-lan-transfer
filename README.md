# PS5 LAN Transfer

A small open-source homebrew payload and host client for transferring files over a local network between an exploited PS5 and a development computer.

## Features

- `LIST` files below `/data/ps5-transfer`
- `GET <name>` download a file
- `PUT <name> <size>` upload a file
- path traversal protection
- simple Python host client with progress output

This project is intended for lawful homebrew development and personal files. It does not access PSN, bypass DRM, or handle commercial game content.

## Requirements

- [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk)
- an ELF loader compatible with your PS5 homebrew environment
- Python 3.10+ for the host client

The SDK and loader determine firmware compatibility. This repository does not claim support for a specific firmware version without device testing.

## Build

```sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
make
```

The result is `ps5-lan-transfer.elf`.

## Run

Start the payload with your ELF loader. It listens on TCP port `9022` by default.

```sh
python3 host/ps5_transfer.py --host 192.168.1.50 list
python3 host/ps5_transfer.py --host 192.168.1.50 get notes.txt --output notes.txt
python3 host/ps5_transfer.py --host 192.168.1.50 put local.bin --name local.bin
```

Set `PS5_TRANSFER_PORT` or pass `--port` to change the port. Files are stored under `/data/ps5-transfer` on the console.

## Status

The protocol and payload are intentionally small and easy to extend. Add authentication, checksums, resume support, and a UI only after the basic flow is tested on real hardware.

## License

GPL-3.0-or-later. The PS5 SDK is a separate project with its own license.
