#!/usr/bin/env python3

import sys

# Index by 3-bit register field first, then by wide bit
REG_NAME_MAP = [
    ["al", "ax"],
    ["cl", "cx"],
    ["dl", "dx"],
    ["bl", "bx"],
    ["ah", "sp"],
    ["ch", "bp"],
    ["dh", "si"],
    ["bh", "di"]
]

def Decode(FileName : str):
    InFile = open(FileName, mode="rb")

    Bytes = list(InFile.read())

    OutLines = []
    OutLines.append("bits 16\n\n")

    assert len(Bytes) % 2 == 0

    for i in range(0, len(Bytes) - 1, 2):
        Byte0 = Bytes[i]
        Byte1 = Bytes[i + 1]

        assert Byte0 & (0b0100010 << 2) # Check this is definitely a mov instruction
    
        DBit = (Byte0 & (1 << 1)) >> 1
        WideBit = Byte0 & 1
        RegField = (Byte1 & ((1 << 5) | (1 << 4) | (1 << 3))) >> 3
        RMField = Byte1 & ((1 << 2) | (1 << 1) | 1)
        
        if DBit:
            SrcReg = REG_NAME_MAP[RMField][WideBit]
            DestReg = REG_NAME_MAP[RegField][WideBit]
        else:
            SrcReg = REG_NAME_MAP[RegField][WideBit]
            DestReg = REG_NAME_MAP[RMField][WideBit]

        OutLines.append(f"mov {DestReg}, {SrcReg}\n")

    IndexOfLastSlash = FileName.rfind('/')
    OutFileName = FileName[0:IndexOfLastSlash + 1]
    OutFileName += "out.asm"

    OutFile = open(OutFileName, 'w')
    OutFile.writelines(OutLines)

    InFile.close()
    OutFile.close()


if __name__ == "__main__":
    if (len(sys.argv) != 2):
        print("Usage, homie")
        sys.exit(1)
    Decode(sys.argv[1])