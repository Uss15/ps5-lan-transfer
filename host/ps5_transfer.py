#!/usr/bin/env python3
import argparse
import socket
from pathlib import Path


def connect(args):
    return socket.create_connection((args.host, args.port), timeout=args.timeout)


def receive_line(sock):
    data = bytearray()
    while not data.endswith(b"\n"):
        chunk = sock.recv(1)
        if not chunk:
            raise RuntimeError("connection closed before response header")
        data.extend(chunk)
    return data.decode("utf-8").rstrip("\n")


def main():
    parser = argparse.ArgumentParser(description="PS5 LAN Transfer host client")
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=9022)
    parser.add_argument("--timeout", type=float, default=10)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("list")
    get = sub.add_parser("get")
    get.add_argument("name")
    get.add_argument("--output", type=Path, required=True)
    put = sub.add_parser("put")
    put.add_argument("source", type=Path)
    put.add_argument("--name")
    args = parser.parse_args()
    with connect(args) as sock:
        if args.command == "list":
            sock.sendall(b"LIST\n")
            while True:
                line = receive_line(sock)
                if line == ".": break
                print(line)
            return
        if args.command == "get":
            sock.sendall(f"GET {args.name}\n".encode())
            header = receive_line(sock).split()
            if header[0] != "OK": raise RuntimeError(" ".join(header))
            remaining = int(header[1])
            with args.output.open("wb") as output:
                while remaining:
                    chunk = sock.recv(min(65536, remaining))
                    if not chunk: raise RuntimeError("connection closed during download")
                    output.write(chunk)
                    remaining -= len(chunk)
            print(f"Downloaded {args.name} -> {args.output}")
            return
        name = args.name or args.source.name
        data = args.source.read_bytes()
        sock.sendall(f"PUT {name} {len(data)}\n".encode() + data)
        print(receive_line(sock))


if __name__ == "__main__":
    main()
