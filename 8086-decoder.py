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

def GetDisplacementFormula(Mod : int, RMField : int, Bytes : [int], ByteIndex : int):
    AddressReg = OffsetReg = Disp = DispSign = None
    if Mod != 0b11:
        if Mod == 0 and RMField == 0b110:
            # TODO: 16-bit direct address
            Byte2 = Bytes[ByteIndex]
            ByteIndex += 1
            Byte3 = Bytes[ByteIndex]
            ByteIndex += 1

            DispLow = Byte2
            DispHigh = Byte3

            Disp = (DispHigh << 8) | DispLow
            return None, None, Disp, None, ByteIndex

        if RMField >> 2:
            # No offset register
            if RMField == 0b100:
                AddressReg = "si"
            elif RMField == 0b101:
                AddressReg = "di"
            elif RMField == 0b110:
                AddressReg = "bp"
            elif RMField == 0b111:
                AddressReg = "bx"
        else:
            if RMField & 1:
                OffsetReg = "di"
            else:
                OffsetReg = "si"
            if (RMField >> 1) & 1:
                AddressReg = "bp"
            else:
                AddressReg = "bx"

        if Mod == 0:
            # No displacement
            return AddressReg, OffsetReg, Disp, DispSign, ByteIndex
        elif Mod == 0b01:
            # 8-bit displacement
            Byte2 = Bytes[ByteIndex]
            ByteIndex += 1

            Disp = Byte2
            if Disp > 127:
                Disp -= 256
                Disp *= -1
                DispSign = '-'
            else:
                DispSign = '+'

            return AddressReg, OffsetReg, Disp, DispSign, ByteIndex
        elif Mod == 0b10:
            # 16-bit displacement
            Byte2 = Bytes[ByteIndex]
            ByteIndex += 1
            Byte3 = Bytes[ByteIndex]
            ByteIndex += 1

            DispLow = Byte2
            DispHigh = Byte3

            Disp = (DispHigh << 8) | DispLow
            if Disp > (1 << 15) - 1:
                Disp -= 1 << 16
                Disp *= -1
                DispSign = '-'
            else:
                DispSign = '+'

            return AddressReg, OffsetReg, Disp, DispSign, ByteIndex
            
    else:
        # No displacement (reg-to-reg)
        return None, None, None, None, ByteIndex

IMM_TO_REG_MASK = 0b1011 << 4
MEMREG_TO_MEMREG_MASK = 0b100010 << 2
IMM_TO_REGMEM_MASK = 0b1100011 << 1
ACC_OP_MASK = 0b101000 << 2

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
        elif Byte0 & MEMREG_TO_MEMREG_MASK == MEMREG_TO_MEMREG_MASK:
            # Memory/register to register
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

            Mod = Byte1 >> 6
            DBit = (Byte0 & (1 << 1)) >> 1
            WideBit = Byte0 & 1
            RegField = (Byte1 & ((1 << 5) | (1 << 4) | (1 << 3))) >> 3
            RMField = Byte1 & ((1 << 2) | (1 << 1) | 1)
            
            AddressReg, OffsetReg, Disp, DispSign, ByteIndex = GetDisplacementFormula(Mod, RMField, Bytes, ByteIndex)

            if Mod != 0b11:
                if Mod == 0 and RMField == 0b110:
                    # 16-bit direct address
                    DestReg = REG_NAME_MAP[RegField][WideBit]
                    OutLines.append(f"mov {DestReg}, [{Disp}]\n")
                    continue
                if Mod == 0:
                    # No displacement
                    if DBit:
                        DestReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"mov {DestReg}, [{AddressReg} + {OffsetReg}]\n")
                        else:
                            OutLines.append(f"mov {DestReg}, [{AddressReg}]\n")
                    else:
                        SrcReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"mov [{AddressReg} + {OffsetReg}], {SrcReg}\n")
                        else:
                            OutLines.append(f"mov [{AddressReg}], {SrcReg}\n")
                elif Mod == 0b01 or Mod == 0b10:
                    # 8/16-bit displacement
                    if DBit:
                        DestReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"mov {DestReg}, [{AddressReg} + {OffsetReg} {DispSign} {Disp}]\n")
                        else:
                            OutLines.append(f"mov {DestReg}, [{AddressReg} {DispSign} {Disp}]\n")
                    else:
                        SrcReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"mov [{AddressReg} + {OffsetReg} {DispSign} {Disp}], {SrcReg}\n")
                        else:
                            OutLines.append(f"mov [{AddressReg} {DispSign} {Disp}], {SrcReg}\n")
            else:
                # No displacement (reg-to-reg)
                if DBit:
                    SrcReg = REG_NAME_MAP[RMField][WideBit]
                    DestReg = REG_NAME_MAP[RegField][WideBit]
                else:
                    SrcReg = REG_NAME_MAP[RegField][WideBit]
                    DestReg = REG_NAME_MAP[RMField][WideBit]
                OutLines.append(f"mov {DestReg}, {SrcReg}\n")


        elif Byte0 & IMM_TO_REGMEM_MASK == IMM_TO_REGMEM_MASK:
            # Immediate to register/memory
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1
            
            WideBit = Byte0 & 1
            Mod = Byte1 >> 6
            RMField = Byte1 & 0b111

            if WideBit:
                DataType = "word"
            else:
                DataType = "byte"

            AddressReg, OffsetReg, Disp, DispSign, ByteIndex = GetDisplacementFormula(Mod, RMField, Bytes, ByteIndex)
            
            Immediate = Bytes[ByteIndex]
            ByteIndex += 1

            if WideBit:
                ImmHigh = Bytes[ByteIndex]
                ByteIndex += 1

                Immediate = (ImmHigh << 8) | Immediate
            if AddressReg is None:
                # Moving immediate into register
                DestReg = REG_NAME_MAP[RMField][WideBit]
                OutLines.append(f"mov {DestReg}, {Immediate}\n")
            else:
                if OffsetReg is None:
                    if Disp is None:
                        OutLines.append(f"mov [{AddressReg}], {DataType} {Immediate}\n")
                    else:
                        OutLines.append(f"mov [{AddressReg} {DispSign} {Disp}], {DataType} {Immediate}\n")
                else:
                    if Disp is None:
                        OutLines.append(f"mov [{AddressReg} + {OffsetReg}], {DataType} {Immediate}\n")
                    else:
                        OutLines.append(f"mov [{AddressReg} + {OffsetReg} {DispSign} {Disp}], {DataType} {Immediate}\n")
        elif Byte0 & ACC_OP_MASK == ACC_OP_MASK:
            WideBit = Byte0 & 1
            if WideBit:
                Reg = "ax"
            else:
                Reg = "al"

            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1
            Byte2 = Bytes[ByteIndex]
            ByteIndex += 1

            Addr = (Byte2 << 8) | Byte1

            if Byte0 & (1 << 1):
                # Accumulator to memory
                OutLines.append(f"mov [{Addr}], {Reg}\n")
            else:
                # Memory to accumulator
                OutLines.append(f"mov {Reg}, [{Addr}]\n")

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