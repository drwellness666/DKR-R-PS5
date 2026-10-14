#!/usr/bin/env python3
"""Send one explicitly chosen ELF to an already running PS5 ELF loader."""
import argparse
from pathlib import Path
import socket

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", help="Your console's LAN IP; no discovery")
    parser.add_argument("elf", type=Path)
    parser.add_argument("--port", type=int, default=9021)
    parser.add_argument("--send-timeout", type=float, default=180,
                        help="Seconds allowed for sending the ELF; default 180 for large installers")
    parser.add_argument("--no-wait", action="store_true", help="Send and disconnect; useful for persistent server payloads")
    args = parser.parse_args()
    if args.send_timeout <= 0:
        parser.error("--send-timeout must be positive")
    data = args.elf.read_bytes()
    if data[:4] != b"\x7fELF":
        parser.error("Input is not an ELF")
    with socket.create_connection((args.host, args.port), timeout=10) as sock:
        sock.settimeout(args.send_timeout)
        sock.sendall(data)
        sock.shutdown(socket.SHUT_WR)
        if args.no_wait:
            print("ELF sent. No console output was collected; execution is not confirmed.")
            return
        sock.settimeout(70)
        while True:
            try:
                chunk = sock.recv(4096)
            except socket.timeout:
                print("Loader returned no more output; inspect the console.")
                break
            except KeyboardInterrupt:
                print("\nStopped receiving output. This does not send a stop command to the PS5; the payload may still be running.")
                return
            if not chunk:
                break
            print(chunk.decode("utf-8", errors="replace"), end="", flush=True)
    print("\nTransfer complete. Transfer success is not a console gameplay test.")

if __name__ == "__main__":
    main()
