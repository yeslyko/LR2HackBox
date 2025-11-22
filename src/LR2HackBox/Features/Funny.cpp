#include "Funny.hpp"

#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"
#include <random>

#include <safetyhook.hpp>
#include "imgui/imgui.h"

void Funny::OnDrawNote(SafetyHookContext& regs) {
	Funny& funny = *(Funny*)(LR2HackBox::Get().mFunny.get());
	if (!funny.mIsInvisibleScratch) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.gameplay.keymode != 7) return;

	if (regs.esi == 0) regs.esi++;
}

int Funny::OnProcSinglenote(void* g, int lane, int keypress, int timing, int player) {
	Funny& funny = *(Funny*)(LR2HackBox::Get().mFunny.get());
	if (!funny.mIsSpatialKeysounds) return funny.oProcSinglenote.ccall<int>(g, lane, keypress, timing, player);;
	LR2::game& game = *(LR2::game*)g;
	LR2::NoteStruct& note = game.gameplay.bmsobj_note[lane].notes[game.gameplay.bmsobj_note[lane].note_count];

	int laneCount = game.gameplay.keymode < 10 ? game.gameplay.keymode == 9 ? game.gameplay.keymode : game.gameplay.keymode + 1 : game.gameplay.keymode + 2;
	float panIncrement = 2.f / laneCount;
	float pan = -1.f + lane * panIncrement + (panIncrement / 2);
	int result = funny.oProcSinglenote.ccall<int>(g, lane, keypress, timing, player);
	funny.FMOD_Channel_SetPan(game.gameplay.keysound[static_cast<int>(note.val)].fmod_channel, pan);
	return result;
}

void Funny::OnEndParseBmsFile(SafetyHookContext& regs) {
	Funny& funny = *(Funny*)(LR2HackBox::Get().mFunny.get());
	if (!funny.mIsShuffleKeysounds) return;

	LR2::gameplay& gameplay = LR2HackBox::Get().GetGame()->gameplay;
	std::unordered_map<double, double> randomKeysoundsMapping;
	std::vector<double> shuffledKeysounds;
	for (int laneIdx = 0; laneIdx < 20; laneIdx++) {
		LR2::LaneStruct& lane = gameplay.bmsobj_note[laneIdx];
		if (lane.count == 0) continue;
		for (int noteIdx = 0; noteIdx < lane.count; noteIdx++) {
			if (randomKeysoundsMapping.contains(lane.notes[noteIdx].val)) continue;
			randomKeysoundsMapping[lane.notes[noteIdx].val] = 0;
			shuffledKeysounds.push_back(lane.notes[noteIdx].val);
		}
	}

	std::mt19937 g(gameplay.randomseed);
	std::shuffle(shuffledKeysounds.begin(), shuffledKeysounds.end(), g);
	int i = 0;
	for (auto& it : randomKeysoundsMapping) {
		it.second = shuffledKeysounds[i];
		i++;
	}

	if (gameplay.isAutoplay) {
		for (int noteIdx = 0; noteIdx < gameplay.bmsobj.count; noteIdx++) {
			LR2::NoteStruct& note = gameplay.bmsobj.notes[noteIdx];
			if (randomKeysoundsMapping.contains(note.val)) {
				note.val = randomKeysoundsMapping[note.val];
			}
		}
	}
	else {
		for (int laneIdx = 0; laneIdx < 20; laneIdx++) {
			LR2::LaneStruct& lane = gameplay.bmsobj_note[laneIdx];
			if (lane.count == 0) continue;
			for (int noteIdx = 0; noteIdx < lane.count; noteIdx++) {
				LR2::NoteStruct& note = lane.notes[noteIdx];
				note.val = randomKeysoundsMapping[note.val];
			}
		}
	}
}

bool Funny::Init(uintptr_t moduleBase) {
	Funny::mModuleBase = moduleBase;

	HMODULE fmodModule = GetModuleHandle("fmodex.dll");
	FMOD_Channel_GetPan = (decltype(FMOD_Channel_GetPan))GetProcAddress(fmodModule, "FMOD_Channel_GetPan");
	FMOD_Channel_SetPan = (decltype(FMOD_Channel_SetPan))GetProcAddress(fmodModule, "FMOD_Channel_SetPan");

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x7098), OnDrawNote));
#ifdef NDEBUG
	oProcSinglenote = safetyhook::create_inline(moduleBase + 0x018850, OnProcSinglenote);
#endif
	mMidHooks.push_back(safetyhook::create_mid(moduleBase + 0x0B64D9, OnEndParseBmsFile));

	return true;
}

bool Funny::Deinit() {
	return true;
}

static void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void Funny::Menu() {
	struct IdPopper { IdPopper(const char* id) { ImGui::PushID(id); };  ~IdPopper() { ImGui::PopID(); } } _id_popper{ "Funny" };

	LR2::game* game = LR2HackBox::Get().GetGame();

	ImGui::Checkbox("Invisible Scratch", &mIsInvisibleScratch);
	ImGui::SameLine();
	HelpMarker("When enabled, will stop scratch notes from rendering.");

	if (ImGui::Checkbox("Metronome", &mIsMetronome)) {
		((Misc*)LR2HackBox::Get().mMisc.get())->SetMetronome(mIsMetronome);
	}
	ImGui::SameLine();
	HelpMarker("Enables the metronome sound in playing.\n  Expects 'metronome-measure.wav' and 'metronome-beat.wav'\n  in LR2files\\Sound\\LR2HackBox\\ directory\n  or defaults to using f-open and f-close");

	ImGui::Checkbox("Spatial Keysounds", &mIsSpatialKeysounds);
	ImGui::SameLine();
	HelpMarker("Left ear, right ear, left ear, middle, left ear, right ear");

	ImGui::Checkbox("Shuffle Keysounds", &mIsShuffleKeysounds);
	ImGui::SameLine();
	HelpMarker("Randomly shuffles keysounds of a chart. Makes predictable result combined with unrandomizer");
}