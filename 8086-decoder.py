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

IMM_TO_REG_MASK = 0b1011 << 4
MEMREG_TO_REG_MASK = 0b100010 << 2
IMM_TO_REGMEM_MASK = 0b1100011 << 1

def Decode(FileName : str):
    InFile = open(FileName, mode="rb")

    Bytes = list(InFile.read())

    OutLines = []
    OutLines.append("bits 16\n\n")

    ByteIndex = 0
    while ByteIndex < len(Bytes):
        Byte0 = Bytes[ByteIndex]
        ByteIndex += 1

        if Byte0 & IMM_TO_REG_MASK == IMM_TO_REG_MASK:
            # Immediate to register
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

            WideBit = (Byte0 & (1 << 3)) >> 3
            RegField = Byte0 & 0b111
            DestReg = REG_NAME_MAP[RegField][WideBit]

            Data = Byte1
            if WideBit:
                Byte2 = Bytes[ByteIndex]
                ByteIndex += 1

                Data |= (Byte2 << 8)
            OutLines.append(f"mov {DestReg}, {Data}\n")
        elif Byte0 & MEMREG_TO_REG_MASK == MEMREG_TO_REG_MASK:
            # Memory/register to register
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

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
        elif Byte0 & IMM_TO_REGMEM_MASK == IMM_TO_REGMEM_MASK:
            # Immediate to register/memory
            pass
        

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