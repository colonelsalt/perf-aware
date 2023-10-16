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

JUMP_OP_TABLE = [
    "jo", "jno", "jb", "jnb", "je", "jne", "jbe", "jnbe", "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle"
]

LOOP_OP_TABLE = [
    "loopnz", "loopz", "loop", "jcxz"
]

def GetDisplacementFormula(Mod : int, RMField : int, Bytes : [int], ByteIndex : int):
    AddressReg = OffsetReg = Disp = DispSign = None
    if Mod != 0b11:
        if Mod == 0 and RMField == 0b110:
            # 16-bit direct address
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

MOV_IMM_TO_REG_MASK = 0b1011

MOV_MEMREG_TO_MEMREG_MASK = 0b100010
ADD_MEMREG_TO_MEMREG_MASK = 0b000000
SUB_MEMREG_TO_MEMREG_MASK = 0b001010
CMP_MEMREG_TO_MEMREG_MASK = 0b001110

MOV_IMM_TO_REGMEM_MASK = 0b1100011
DEF_IMM_TO_REGMEM_MASK = 0b100000

MOV_ACC_OP_MASK = 0b101000

ADD_IMM_TO_ACC_MASK = 0b0000010
SUB_IMM_TO_ACC_MASK = 0b0010110
CMP_IMM_TO_ACC_MASK = 0b0011110

JMP_PREFIX = 0b111
LOOP_PREFIX = 0b111000

ADD_PATTERN = 0b000
SUB_PATTERN = 0b101
CMP_PATTERN = 0b111

def Decode(FileName : str):
    InFile = open(FileName, mode="rb")

    Bytes = list(InFile.read())

    OutLines = []
    OutLines.append("bits 16\n\n")

    ByteIndex = 0
    while ByteIndex < len(Bytes):
        Byte0 = Bytes[ByteIndex]
        ByteIndex += 1

        if Byte0 >> 4 == MOV_IMM_TO_REG_MASK:
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
        elif Byte0 >> 2 == MOV_MEMREG_TO_MEMREG_MASK or\
             Byte0 >> 2 == 0 or\
             Byte0 >> 2 == SUB_MEMREG_TO_MEMREG_MASK or\
             Byte0 >> 2 == CMP_MEMREG_TO_MEMREG_MASK:
            # Memory/register to register
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

            Mod = Byte1 >> 6
            DBit = (Byte0 & (1 << 1)) >> 1
            WideBit = Byte0 & 1
            RegField = (Byte1 & ((1 << 5) | (1 << 4) | (1 << 3))) >> 3
            RMField = Byte1 & ((1 << 2) | (1 << 1) | 1)
            
            AddressReg, OffsetReg, Disp, DispSign, ByteIndex = GetDisplacementFormula(Mod, RMField, Bytes, ByteIndex)
            
            OpPattern = (Byte0 >> 3) & 0b111

            if OpPattern == CMP_PATTERN:
                Op = "cmp"
            elif OpPattern == SUB_PATTERN:
                Op = "sub"
            elif OpPattern == ADD_PATTERN:
                Op = "add"
            else:
                Op = "mov"

            if Mod != 0b11:
                if Mod == 0 and RMField == 0b110:
                    # 16-bit direct address
                    DestReg = REG_NAME_MAP[RegField][WideBit]
                    OutLines.append(f"{Op} {DestReg}, [{Disp}]\n")
                    continue
                if Mod == 0:
                    # No displacement
                    if DBit:
                        DestReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"{Op} {DestReg}, [{AddressReg} + {OffsetReg}]\n")
                        else:
                            OutLines.append(f"{Op} {DestReg}, [{AddressReg}]\n")
                    else:
                        SrcReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"{Op} [{AddressReg} + {OffsetReg}], {SrcReg}\n")
                        else:
                            OutLines.append(f"{Op} [{AddressReg}], {SrcReg}\n")
                elif Mod == 0b01 or Mod == 0b10:
                    # 8/16-bit displacement
                    if DBit:
                        DestReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"{Op} {DestReg}, [{AddressReg} + {OffsetReg} {DispSign} {Disp}]\n")
                        else:
                            OutLines.append(f"{Op} {DestReg}, [{AddressReg} {DispSign} {Disp}]\n")
                    else:
                        SrcReg = REG_NAME_MAP[RegField][WideBit]
                        if OffsetReg:
                            OutLines.append(f"{Op} [{AddressReg} + {OffsetReg} {DispSign} {Disp}], {SrcReg}\n")
                        else:
                            OutLines.append(f"{Op} [{AddressReg} {DispSign} {Disp}], {SrcReg}\n")
            else:
                # No displacement (reg-to-reg)
                if DBit:
                    SrcReg = REG_NAME_MAP[RMField][WideBit]
                    DestReg = REG_NAME_MAP[RegField][WideBit]
                else:
                    SrcReg = REG_NAME_MAP[RegField][WideBit]
                    DestReg = REG_NAME_MAP[RMField][WideBit]
                OutLines.append(f"{Op} {DestReg}, {SrcReg}\n")


        elif Byte0 >> 1 == MOV_IMM_TO_REGMEM_MASK or\
             Byte0 >> 2 == DEF_IMM_TO_REGMEM_MASK:
            # Immediate to register/memory
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1
            
            OpPattern = (Byte1 >> 3) & 0b111
            Op = "ERROR"
            if Byte0 >> 1 == MOV_IMM_TO_REGMEM_MASK:
                Op = "mov"
            elif OpPattern == ADD_PATTERN:
                Op = "add"
            elif OpPattern == SUB_PATTERN:
                Op = "sub"
            elif OpPattern == CMP_PATTERN:
                Op = "cmp"

            WideBit = Byte0 & 1
            Mod = Byte1 >> 6
            RMField = Byte1 & 0b111

            if Op == "mov":
                SignBit = 0
            else:
                SignBit = (Byte0 >> 1) & 1
            SW = (SignBit << 1) | WideBit

            if WideBit:
                DataType = "word"
            else:
                DataType = "byte"

            AddressReg, OffsetReg, Disp, DispSign, ByteIndex = GetDisplacementFormula(Mod, RMField, Bytes, ByteIndex)
            
            Immediate = Bytes[ByteIndex]
            ByteIndex += 1

            if SW == 0b01:
                ImmHigh = Bytes[ByteIndex]
                ByteIndex += 1

                Immediate = (ImmHigh << 8) | Immediate
            if AddressReg is None:
                # Moving immediate into register
                DestReg = REG_NAME_MAP[RMField][WideBit]
                OutLines.append(f"{Op} {DestReg}, {Immediate}\n")
            else:
                if OffsetReg is None:
                    if Disp is None:
                        OutLines.append(f"{Op} [{AddressReg}], {DataType} {Immediate}\n")
                    else:
                        OutLines.append(f"{Op} [{AddressReg} {DispSign} {Disp}], {DataType} {Immediate}\n")
                else:
                    if Disp is None:
                        OutLines.append(f"{Op} [{AddressReg} + {OffsetReg}], {DataType} {Immediate}\n")
                    else:
                        OutLines.append(f"{Op} [{AddressReg} + {OffsetReg} {DispSign} {Disp}], {DataType} {Immediate}\n")
        elif Byte0 >> 2 == MOV_ACC_OP_MASK or\
             Byte0 >> 1 == ADD_IMM_TO_ACC_MASK or\
             Byte0 >> 1 == SUB_IMM_TO_ACC_MASK or\
             Byte0 >> 1 == CMP_IMM_TO_ACC_MASK:
            WideBit = Byte0 & 1
            if WideBit:
                Reg = "ax"
            else:
                Reg = "al"
            
            OpPattern = Byte0 >> 3
            if OpPattern == ADD_PATTERN:
                Op = "add"
            elif OpPattern == SUB_PATTERN:
                Op = "sub"
            elif OpPattern == CMP_PATTERN:
                Op = "cmp"
            else:
                Op = "mov"

            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

            if Op == "mov" or WideBit:
                Byte2 = Bytes[ByteIndex]
                ByteIndex += 1

                Addr = (Byte2 << 8) | Byte1
            else:
                Addr = Byte1

            if Op == "mov":
                if Byte0 & (1 << 1):
                    # Accumulator to memory
                    OutLines.append(f"mov [{Addr}], {Reg}\n")
                else:
                    # Memory to accumulator
                    OutLines.append(f"mov {Reg}, [{Addr}]\n")
            else:
                OutLines.append(f"{Op} {Reg}, {Addr}\n")
        elif Byte0 >> 4 == JMP_PREFIX:
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

            if Byte1 > 127:
                Offset = Byte1 - 256
            else:
                Offset = Byte1

            Op = JUMP_OP_TABLE[Byte0 & 0b1111]
            OutLines.append(f"{Op} label; {Offset}\n")
        elif Byte0 >> 2 == LOOP_PREFIX:
            Byte1 = Bytes[ByteIndex]
            ByteIndex += 1

            if Byte1 > 127:
                Offset = Byte1 - 256
            else:
                Offset = Byte1

            Op = LOOP_OP_TABLE[Byte0 & 0b11]
            OutLines.append(f"{Op} label; {Offset}\n")
            

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