// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <vector>

std::vector<int> piecelist = { 0x0001, 0x0000, 0x0004, 0x000A };
int piecelist_idx = 0;

enum PieceTypes {
	Piece_1,
	Piece_2,
	Piece_3,
	Piece_enemy
};

#pragma pack(1)
struct EmeraldHintOffset{
	uint8_t maj_id;
	uint8_t min_id;
	uint16_t hint1_offset;
	uint16_t hint2_offset;
	uint16_t hint3_offset;
};

DataPointer(struct EmeraldHintOffset*, HintOffsetPtr, 0x1af016c);
DataPointer(char *, HintText, 0x1af0170);
void __cdecl GetHintText(uint8_t maj_id, uint8_t min_id)
{
	int hintoffsetidx = 0;
	struct EmeraldHintOffset hint_offset;
	do {
		hint_offset = HintOffsetPtr[hintoffsetidx];
		hintoffsetidx++;
	} while (hint_offset.maj_id != maj_id || hint_offset.min_id != min_id);

	char * hint1 = HintText + ((uint32_t*)HintText)[hint_offset.hint1_offset] + 3;
	if (hint1[0] == '\a')
		hint1 = hint1 + 1;
	char * hint2 = HintText + ((uint32_t*)HintText)[hint_offset.hint2_offset] + 3;
	if (hint2[0] == '\a')
		hint2 = hint2 + 1;
	char * hint3 = HintText + ((uint32_t*)HintText)[hint_offset.hint3_offset] + 3;
	if (hint3[0] == '\a')
		hint3 = hint3 + 1;
	PrintDebug("Hint 1: %s", hint1);
	PrintDebug("Hint 2: %s", hint2);
	PrintDebug("Hint 3: %s", hint3);
}

void __cdecl GenerateEmeralds_r(EmeManObj2 *emerald_manager)
{
	PrintDebug("At start of piecegen");
	uint8_t maj_id = piecelist[piecelist_idx] & 0xff;
	uint8_t min_id = (piecelist[piecelist_idx] >> 8) & 0xff;
	PieceTypes piece_type;

	switch (maj_id) {
	case 0x0:
	case 0x2:
	case 0x5:
		piece_type = Piece_2;
		break;
	case 0x1:
	case 0x3:
		piece_type = Piece_1;
		break;
	case 0x4:
	case 0x6:
	case 0x7: // Dig pieces in PH/EQ
		piece_type = Piece_3;
		break;
	case 0xa:
		piece_type = Piece_enemy;
	}

	// Emeralds, set to "collected"
	emerald_manager->byte2C[0].byte0 = 0xfe;
	emerald_manager->byte2C[1].byte0 = 0xfe;
	emerald_manager->byte2C[2].byte0 = 0xfe;

	// "Emeralds spawned" we always want this to be 1
	emerald_manager->byte5 = 1; 

	int offset = 0;
	EmeManThing emerald;
	switch (piece_type) {
	case Piece_1:
		do {
			if (offset > emerald_manager->byte6) {
				// panic
				PrintDebug("In infinite loop P1 oops");
			}
			emerald = emerald_manager->ptr_a[offset];
			offset = offset + 1;
		} while (emerald.byte0 != maj_id || emerald.byte1 != min_id);
		emerald_manager->byte2C[0] = emerald;
		emerald_manager->byte2C[1].byte0 = 0xff;
		emerald_manager->byte2C[2].byte0 = 0xff;
		break;
	case Piece_enemy: // Set enemy pieces as p1 because lazy
		do {
			if (offset > emerald_manager->byte9) {
				// panic
				PrintDebug("In infinite loop Enemy oops");
			}
			emerald = emerald_manager->ptr_d[offset];
			offset = offset + 1;
		} while (emerald.byte0 != maj_id || emerald.byte1 != min_id);
		emerald_manager->byte2C[0] = emerald;
		emerald_manager->byte2C[1].byte0 = 0xff;
		break;
	case Piece_2:
		do {
			if (offset > emerald_manager->byte7) {
				// panic
				PrintDebug("In infinite loop P2 oops");
			}
			emerald = emerald_manager->ptr_b[offset];
			offset = offset + 1;
		} while (emerald.byte0 != maj_id || emerald.byte1 != min_id);
		emerald_manager->byte2C[1] = emerald;
		emerald_manager->byte2C[2].byte0 = 0xff;
		break;
	case Piece_3:
		do {
			if (offset > emerald_manager->byte8) {
				// panic
				PrintDebug("In infinite loop P3 oops");
			}
			emerald = emerald_manager->ptr_c[offset];
			offset = offset + 1;
		} while (emerald.byte0 != maj_id || emerald.byte1 != min_id);
		emerald_manager->byte2C[2] = emerald;
		break;
	}
	GetHintText(maj_id, min_id);
	PrintDebug("At end of piece gen");
	return;
}

void __cdecl IncrementPiecelist() {
	piecelist_idx = (piecelist_idx + 1) % piecelist.size();
}

const void *const loc_739f8e = (void*)0x739f8e;
__declspec(naked) void Case5_Swap() {
	IncrementPiecelist();
	__asm
	{
		mov byte ptr[edi], 0x8
		jmp loc_739f8e
	}
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path, const HelperFunctions &helperFunctions)
	{
		// Executed at startup, contains helperFunctions and the path to your mod (useful for getting the config file.)
		// This is where we override functions, replace static data, etc.
		WriteJump(EmeraldLocations_1POr2PGroup3, GenerateEmeralds_r);
		WriteJump((void *)0x739e6c, Case5_Swap); // Case 5 of the Emeralds update thing
	}

	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer }; // This is needed for the Mod Loader to recognize the DLL.
}
