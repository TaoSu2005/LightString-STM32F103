#!/usr/bin/env python3
"""serial_midi_bridge: 把流光琴串口日志桥接为电脑上的虚拟 MIDI 设备。

开发板每次触发音符会经 USART1 打印:
    [LS] on  n=<note> v=<vel> L=<norm>
    [LS] off
本脚本解析这些行并通过 mido 转发到虚拟 MIDI 端口, 这样 GarageBand /
LMMS 等宿主软件就能把流光琴当成一个 MIDI 键盘使用。

用法:
    pip install pyserial mido python-rtmidi
    python3 serial_midi_bridge.py /dev/cu.usbserial-11130
"""
import re
import sys

import mido
import serial

PAT_ON = re.compile(r"\[LS\] on\s+n=(\d+)\s+v=(\d+)")
PAT_OFF = re.compile(r"\[LS\] off")


def main() -> None:
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbserial-11130"
    ser = serial.Serial(port, 115200, timeout=1)
    out = mido.open_output("LightString", virtual=True)
    print(f"bridging {port} -> virtual MIDI port 'LightString'")
    last_note = None
    while True:
        line = ser.readline().decode(errors="ignore")
        if m := PAT_ON.search(line):
            note, vel = int(m.group(1)), int(m.group(2))
            if last_note is not None:
                out.send(mido.Message("note_off", note=last_note))
            out.send(mido.Message("note_on", note=note, velocity=vel))
            last_note = note
            print(f"on  {note} vel={vel}")
        elif PAT_OFF.search(line):
            if last_note is not None:
                out.send(mido.Message("note_off", note=last_note))
                last_note = None
            print("off")


if __name__ == "__main__":
    main()
