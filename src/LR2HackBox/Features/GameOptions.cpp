#include "GameOptions.hpp"

#include "LR2HackBox/LR2HackBox.hpp"

#include <safetyhook.hpp>
#include "imgui/imgui.h"

bool GameOptions::Init(uintptr_t moduleBase) {
	GameOptions::mModuleBase = moduleBase;
	LR2::game& game = *LR2HackBox::Get().GetGame();
	ConfigManager& config = *LR2HackBox::Get().mConfig;

	int waitTimeMs = config.ReadValue("iGameOptionsIRWaitTime", 10000);
	game.net.waitTime = waitTimeMs;
	mIRWaitTime = static_cast<float>(waitTimeMs) / 1000.f;

	return true;
}

bool GameOptions::Deinit() {
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

void GameOptions::Menu() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	ConfigManager& config = *LR2HackBox::Get().mConfig;

	ImGui::Indent();
	const float SCALE = ImGui::GetStyle().FontScaleMain;
	const float START_X = ImGui::GetCursorPosX();
	const float COLUMN_WIDTH = 40.f * SCALE;
	const float COLUMN_SPACING = 120.f * SCALE;
	const float COLUMN1_X = 170.f * SCALE;
	const float COLUMN2_X = COLUMN1_X + COLUMN_SPACING;
	const float COLUMN3_X = COLUMN2_X + COLUMN_SPACING;
	const float MIN_WIDTH = ImGui::CalcTextSize("MIN").x;
	const float MAX_WIDTH = ImGui::CalcTextSize("MAX").x;
	const float MARGIN_WIDTH = ImGui::CalcTextSize("MARGIN").x;
	const float FIRST_WIDTH = ImGui::CalcTextSize("First").x;
	const float NEXT_WIDTH = ImGui::CalcTextSize("Next").x;
	const float LEGEND_SPACING = 5.f * SCALE;

	ImGui::PushItemWidth(COLUMN_WIDTH);

	// have to do InputInt first on the line for correct cursor Y for text.
	ImGui::SetCursorPosX(COLUMN1_X);
	ImGui::InputInt("##hsmin", &game.config.play.hsmin, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN1_X - MIN_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("MIN");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN2_X - MAX_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("MAX");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN2_X);
	ImGui::InputInt("##hsmax", &game.config.play.hsmax, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN3_X - MARGIN_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("MARGIN");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN3_X);
	ImGui::InputInt("##hsmargin", &game.config.play.hsmargin, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(START_X);
	ImGui::TextUnformatted("HI SPEED");

	ImGui::SetCursorPosX(COLUMN1_X);
	ImGui::InputInt("##basespeed", &game.config.play.basespeed, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN2_X - MAX_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("Lane cover");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN3_X - MARGIN_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("MARGIN");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN3_X);
	ImGui::InputInt("##shuttermargin", &game.config.play.shuttermargin, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(START_X);
	ImGui::TextUnformatted("Base speed");

	ImGui::SetCursorPosX(COLUMN1_X);
	ImGui::InputInt("ms##poorbga", &game.config.play.poorbga, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(START_X);
	ImGui::TextUnformatted("Miss BGA");
	
	ImGui::SetCursorPosX(COLUMN2_X);
	ImGui::InputInt("ms##inputinterval", &game.config.input.sys_inputinterval, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(START_X);
	ImGui::TextUnformatted("Minimum input interval");

	ImGui::SetCursorPosX(COLUMN2_X);
	ImGui::InputInt("ms##speedfirst", &game.config.select.speedfirst, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN2_X - FIRST_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("First");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN3_X - NEXT_WIDTH - LEGEND_SPACING);
	ImGui::TextUnformatted("Next");
	ImGui::SameLine();
	ImGui::SetCursorPosX(COLUMN3_X);
	ImGui::InputInt("ms##speednext", &game.config.select.speednext, 0);
	ImGui::SameLine();
	ImGui::SetCursorPosX(START_X);
	ImGui::TextUnformatted("Music list");

	ImGui::SetCursorPosX(COLUMN1_X);
	if (ImGui::InputFloat("s##irwaittime", &mIRWaitTime, 0, 0, "%.1f")) {
		int timeMs = static_cast<int>(mIRWaitTime * 1000.f);
		game.net.waitTime = timeMs;
		config.WriteValueAndSave("iGameOptionsIRWaitTime", timeMs);
	}
	ImGui::SameLine();
	ImGui::SetCursorPosX(START_X);
	ImGui::TextUnformatted("IR wait time");

	ImGui::PopItemWidth();
	

	ImGui::Checkbox("PM controller", (bool*)&game.config.select.control);
	ImGui::Checkbox("Assign 1/3 key to scroll", (bool*)&game.config.select.buttonselect);
	ImGui::SameLine();
	ImGui::Checkbox("Disable system keys", (bool*)&game.config.system.disablesystemkey);
	ImGui::Checkbox("Disable skin preview", (bool*)&game.config.system.disableskinpreview);
	ImGui::Checkbox("Disable right click exit", (bool*)&game.config.play.disableleftclickexit);
	ImGui::SameLine();
	ImGui::Checkbox("Assign up/down key to hs change", (bool*)&game.config.play.disablecurspeedchange);
	ImGui::Unindent();
}