#include "Cafe/OS/common/OSCommon.h"
#include "nn_ec.h"
#include "Cafe/OS/libs/nn_common.h"

typedef struct  
{
	/* +0x00 */ uint8 ukn00[0x10];
	/* +0x10 */ uint8 ukn10[0x0C];
	/* +0x2C */ uint8 ukn2C[0x10]; // currency string?
	/* +0x3C */ uint32 ukn3C; // money amount?
	// size of struct is 0x40
}nnEcUknMoneyStruct_t;

void nnEcExport___ct__Q3_2nn2ec5MoneyFPCcN21(PPCInterpreter_t* hCPU)
{
	debug_printf("nn_ec.__ct__Q3_2nn2ec5MoneyFPCcN21(0x%08x, ...)\n", hCPU->gpr[3]);
	nnEcUknMoneyStruct_t* moneyStruct = (nnEcUknMoneyStruct_t*)memory_getPointerFromVirtualOffset(hCPU->gpr[3]);
	// todo -> Setup struct
	osLib_returnFromFunction(hCPU, memory_getVirtualOffsetFromPointer(moneyStruct));
}

namespace nn::ec
{
	// Object constructors: the game passes the constructor's result on, so a
	// constructor must return its this pointer (an unimplemented export returns 0
	// and the game then dereferences null). The object layouts are unknown, so the
	// storage is left as the game allocated it: writing a guessed size could
	// overrun a smaller object.
	static MEMPTR<void> ShoppingCatalog_ct(MEMPTR<void> thisPtr) { return thisPtr; }
	static MEMPTR<void> Query_ct(MEMPTR<void> thisPtr) { return thisPtr; }
	static MEMPTR<void> DateTime_ct(MEMPTR<void> thisPtr) { return thisPtr; }
	static MEMPTR<void> ShoppingCatalog_dt(MEMPTR<void> thisPtr, uint32) { return thisPtr; }
	static MEMPTR<void> Query_dt(MEMPTR<void> thisPtr, uint32) { return thisPtr; }

	class : public COSModule
	{
		public:
		std::string_view GetName() override
		{
			return "nn_ec";
		}

		void RPLMapped() override
		{
			osLib_addFunction("nn_ec", "__ct__Q3_2nn2ec5MoneyFPCcN21", nnEcExport___ct__Q3_2nn2ec5MoneyFPCcN21);
			cafeExportRegisterFunc(ShoppingCatalog_ct, "nn_ec", "__ct__Q3_2nn2ec15ShoppingCatalogFv", LogType::Placeholder);
			cafeExportRegisterFunc(ShoppingCatalog_dt, "nn_ec", "__dt__Q3_2nn2ec15ShoppingCatalogFv", LogType::Placeholder);
			cafeExportRegisterFunc(Query_ct, "nn_ec", "__ct__Q3_2nn2ec5QueryFv", LogType::Placeholder);
			cafeExportRegisterFunc(Query_dt, "nn_ec", "__dt__Q3_2nn2ec5QueryFv", LogType::Placeholder);
			cafeExportRegisterFunc(DateTime_ct, "nn_ec", "__ct__Q3_2nn2ec8DateTimeFv", LogType::Placeholder);
		};

	}s_COSnnEcModule;

	COSModule* GetModule()
	{
		return &s_COSnnEcModule;
	}
}