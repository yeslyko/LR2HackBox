#include "Funny.hpp"

#include <iostream>
#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"

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

bool Funny::Init(uintptr_t moduleBase) {
	Funny::mModuleBase = moduleBase;

	HMODULE fmodModule = GetModuleHandle("fmodex.dll");
	FMOD_Channel_GetPan = (decltype(FMOD_Channel_GetPan))GetProcAddress(fmodModule, "FMOD_Channel_GetPan");
	FMOD_Channel_SetPan = (decltype(FMOD_Channel_SetPan))GetProcAddress(fmodModule, "FMOD_Channel_SetPan");

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x7098), OnDrawNote));
	oProcSinglenote = safetyhook::create_inline(moduleBase + 0x018850, OnProcSinglenote);

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
	LR2::game* game = LR2HackBox::Get().GetGame();

	ImGui::Indent();

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


	ImGui::Unindent();
}