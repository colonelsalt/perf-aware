char const *OpcodeMnemonics[] =
{
	"",
#define INST(Mnemonic, ...) #Mnemonic,
#define INSTALT(...)
#include "sim86_instruction_table.inl"
};

static char const *GetRegName(register_access Reg)
{
	char const *Names[][3] =
	{
		{"", "", ""},
		{"al", "ah", "ax"},
		{"bl", "bh", "bx"},
		{"cl", "ch", "cx"},
		{"dl", "dh", "dx"},
		{"sp", "sp", "sp"},
		{"bp", "bp", "bp"},
		{"si", "si", "si"},
		{"di", "di", "di"},
		{"es", "es", "es"},
		{"cs", "cs", "cs"},
		{"ss", "ss", "ss"},
		{"ds", "ds", "ds"},
		{"ip", "ip", "ip"},
		{"flags", "flags", "flags"},
		{"", "", ""}
	};
    
	char const *Result = Names[Reg.Index % ArrayCount(Names)][(Reg.Count == 2) ? 2 : Reg.Offset&1];
	return Result;
}

static void PrintEffectiveAddressExpression(effective_address_expression Address)
{
	char const *Separator = "";
	for(u32 Index = 0; Index < ArrayCount(Address.Terms); ++Index)
	{
		effective_address_term Term = Address.Terms[Index];
		register_access Reg = Term.Register;
        
		if(Reg.Index)
		{
			printf("%s", Separator);
			if(Term.Scale != 1)
			{
				printf("%d*", Term.Scale);
			}
			printf("%s", GetRegName(Reg));
			Separator = "+";
		}
	}
    
	if(Address.Displacement != 0)
	{
		printf("%+d", Address.Displacement);
	}
}

void PrintInstruction(instruction Instruction)
{
	if (Instruction.Op >= Op_Count)
	{
		printf("ERROR: Invalid Op\n");
		return;
	}


	printf("%s ", OpcodeMnemonics[Instruction.Op]);
	char* Separator = "";

	for (u32 i = 0; i < ArrayCount(Instruction.Operands); i++)
	{
		instruction_operand Op = Instruction.Operands[i];
		if (Op.Type == Operand_None)
		{
			return;
		}
		printf("%s", Separator);
		Separator = ", ";

		switch(Op.Type)
		{
			case Operand_Register:
			{
				printf("%s", GetRegName(Op.Register));
				break;
			}
			case Operand_Memory:
			{
				effective_address_expression EffAddr = Op.Address;

				if (Instruction.Operands[0].Type != Operand_Register)
				{
					char* ExplicitType;
					if (Instruction.Flags & Inst_Wide)
					{
						ExplicitType = "word";
					}
					else
					{
						ExplicitType = "byte";
					}

					printf("%s", ExplicitType);
				}

				printf("[");
				PrintEffectiveAddressExpression(Op.Address);
				printf("]");
				break;
			}
			case Operand_Immediate:
			{
				immediate Immediate = Op.Immediate;
				if (Immediate.Flags & Immediate_RelativeJumpDisplacement)
				{
					printf("$%+d", Immediate.Value + Instruction.Size);
				}
				else
				{
					printf("%d", Immediate.Value);
				}
				break;
			}
		}
	}
}