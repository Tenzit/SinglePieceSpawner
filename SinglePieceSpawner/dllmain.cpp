// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "IniFile.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <unordered_map>
#include <iostream>
#include <fstream>

std::vector<std::string> hint_text_split;
bool clear_on_collect;

std::map<LevelIDs, std::vector<int>> piecelist_map;
std::map<LevelIDs, int> listidx_map;

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
	hint_text_split.clear();
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


	std::stringstream ss(hint1);
	std::string to;
	while (std::getline(ss, to, '\n')) {
		hint_text_split.push_back(to);
	}
	PrintDebug("[Single Piece Spawner] Hint 1: %s", hint1);
	PrintDebug("[Single Piece Spawner] Hint 2: %s", hint2);
	PrintDebug("[Single Piece Spawner] Hint 3: %s", hint3);
}

void __cdecl GenerateEmeralds_r(EmeManObj2 *emerald_manager)
{
	PrintDebug("[Single Piece Spawner] Spawning Piece 0x%.4x (Number %d) in level %d", piecelist_map.at((LevelIDs)CurrentLevel).at(listidx_map.at((LevelIDs)CurrentLevel)), listidx_map.at((LevelIDs)CurrentLevel), CurrentLevel);

	int piece_id = piecelist_map.at((LevelIDs)CurrentLevel).at(listidx_map.at((LevelIDs)CurrentLevel));

	uint8_t maj_id = piece_id & 0xff;
	uint8_t min_id = (piece_id >> 8) & 0xff;
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
	case 0x8:
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
				PrintDebug("[Single Piece Spawner] In infinite loop P1 oops");
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
				PrintDebug("[Single Piece Spawner] In infinite loop Enemy oops");
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
				PrintDebug("[Single Piece Spawner] In infinite loop P2 oops");
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
				PrintDebug("[Single Piece Spawner] In infinite loop P3 oops");
			}
			emerald = emerald_manager->ptr_c[offset];
			offset = offset + 1;
		} while (emerald.byte0 != maj_id || emerald.byte1 != min_id);
		emerald_manager->byte2C[2] = emerald;
		break;
	}
	GetHintText(maj_id, min_id);
	return;
}

void __cdecl IncrementPiecelist() {
	listidx_map.at((LevelIDs)CurrentLevel) = listidx_map.at((LevelIDs)CurrentLevel) + 1;

	if (listidx_map.at((LevelIDs)CurrentLevel) > piecelist_map.at((LevelIDs)CurrentLevel).size()) {
		PrintDebug("[Single Piece Spawner] You've gone through all pieces in list for level %d!", CurrentLevel);
		PrintDebug("[Single Piece Spawner] If you continue in this level, you will start from the beginning of the list!");
		listidx_map.at((LevelIDs)CurrentLevel) = 0;
	}
}

void __cdecl ClearHintText() {
	if (clear_on_collect) {
		hint_text_split.clear();
	}
}

const void *const loc_739f8e = (void*)0x739f8e;
__declspec(naked) void Case5_Swap() {
	IncrementPiecelist();
	ClearHintText();
	__asm
	{
		mov byte ptr[edi], 0x8
		jmp loc_739f8e
	}
}

signed char GetCharacterLevel() {
	for (int i = 0; i < 33; i++)
	{
		if (CurrentLevel == StageSelectLevels[i].Level)
		{
			return StageSelectLevels[i].Character;
		}
	}

	return -1;
}

void __cdecl ReadCsvFile(std::string path, LevelIDs level) {
	std::ifstream file(path);
	std::string line;
	std::vector<int> pieces;

	if (!file.is_open()) {
		PrintDebug("[Single Piece Spawner] Error opening file %s", path.c_str());
		return;
	}
	while (std::getline(file, line)) {
		int i = stoi(line, 0, 16);
		pieces.push_back(i);
	}
	piecelist_map[level] = pieces;
	listidx_map[level] = 0;
}

HelperFunctions helpers;
int y_offset;
bool display_hint;
extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path, const HelperFunctions &helperFunctions)
	{
		// Executed at startup, contains helperFunctions and the path to your mod (useful for getting the config file.)
		// This is where we override functions, replace static data, etc.
		WriteJump(EmeraldLocations_1POr2PGroup3, GenerateEmeralds_r);
		WriteJump((void *)0x739e6c, Case5_Swap); // Case 5 of the Emeralds update thing
		helpers = helperFunctions;

		const IniFile* config = new IniFile(std::string(path) + "\\config.ini");

		display_hint = config->getBool("HintDisplay", "DisplayHint", true);
		y_offset = config->getInt("HintDisplay", "YOffset", 0);
		int text_size = config->getInt("HintDisplay", "TextSize", 12);
		clear_on_collect = config->getBool("HintDisplay", "ClearHint", false);

		helperFunctions.SetDebugFontSize(text_size);

		delete config;

		ReadCsvFile(std::string(path) + "\\pieces\\wild_canyon.csv", LevelIDs_WildCanyon);
		ReadCsvFile(std::string(path) + "\\pieces\\pumpkin_hill.csv", LevelIDs_PumpkinHill);
		ReadCsvFile(std::string(path) + "\\pieces\\aquatic_mine.csv", LevelIDs_AquaticMine);
		ReadCsvFile(std::string(path) + "\\pieces\\death_chamber.csv", LevelIDs_DeathChamber);
		ReadCsvFile(std::string(path) + "\\pieces\\meteor_herd.csv", LevelIDs_MeteorHerd);
		ReadCsvFile(std::string(path) + "\\pieces\\dry_lagoon.csv", LevelIDs_DryLagoon);
		ReadCsvFile(std::string(path) + "\\pieces\\egg_quarters.csv", LevelIDs_EggQuarters);
		ReadCsvFile(std::string(path) + "\\pieces\\security_hall.csv", LevelIDs_SecurityHall);
		ReadCsvFile(std::string(path) + "\\pieces\\mad_space.csv", LevelIDs_MadSpace);

	}

	__declspec(dllexport) void __cdecl OnFrame() {
		if ((GetCharacterLevel() == Characters_Rouge || GetCharacterLevel() == Characters_Knuckles) && EmeraldManagerObj2 && GameState != GameStates_Loading && display_hint) {
			int y = 0;
			for (std::string str : hint_text_split) {
				helpers.DisplayDebugStringFormatted(NJM_LOCATION(0, y+y_offset), "%s", str.c_str());
				y++;
			}
		}
	}

	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer }; // This is needed for the Mod Loader to recognize the DLL.
}
