#include "Numbers.hpp"

#include "LR2HackBox/LR2HackBox.hpp"
#include "ImGuiInjector/ImGuiInjector.hpp"

#include <format>

#include <safetyhook.hpp>
#include "imgui/imgui.h"

Numbers::Judgements Numbers::GetTotalJudgements() {
	Judgements total;
	for (int columnIdx = 0; columnIdx < mKeycount; columnIdx++) {
		Judgements& column = mJudgeCountColumns[mGuiMapping[columnIdx]];
		constexpr const unsigned int dataCount = sizeof(Judgements) / sizeof(unsigned int);
		for (int dataIdx = 0; dataIdx < dataCount; dataIdx++) {
			((unsigned int*)&total)[dataIdx] += ((unsigned int*)&column)[dataIdx];
		}
	}
	return total;
}

void Numbers::CalcJudgementSN(int lane, int keypress, int timing, int player, void* noteIn, int multibadIndent) {
	LR2::NoteStruct& note = *(LR2::NoteStruct*)noteIn;

	Numbers& numbers = *(Numbers*)(LR2HackBox::Get().mNumbers.get());
	LR2::game& game = *LR2HackBox::Get().GetGame();

	Judgements& counter = numbers.mJudgeCountColumns[lane];

	if (note.mine > 0) return;
	if (game.gameplay.player[player].note_current >= game.gameplay.player[player].totalnotes) return;

	if ((double)timing - note.realTiming > (double)game.gameplay.player[player].judgetime[2]) {
		counter.lpr++;
		counter.cb++;
		counter.noteCount++;
		return;
	}

	if (keypress != 1) return;

	int offset = timing - (int)note.realTiming;
	int gap = std::abs(offset);
	bool isFast = offset < 0;

	if (gap <= game.gameplay.player[player].judgetime[5]) {
		isFast ? counter.epg++ : counter.lpg++;
		counter.noteCount++;
	}
	else if (gap <= game.gameplay.player[player].judgetime[4]) {
		isFast ? counter.egr++ : counter.lgr++;
		isFast ? counter.fast++ : counter.slow++;
		counter.noteCount++;
	}
	else if (gap <= game.gameplay.player[player].judgetime[3]) {
		isFast ? counter.egd++ : counter.lgd++;
		isFast ? counter.fast++ : counter.slow++;
		counter.noteCount++;
	}
	else if (gap <= game.gameplay.player[player].judgetime[2]) {
		isFast ? counter.ebd++ : counter.lbd++;
		isFast ? counter.fast++ : counter.slow++;
		counter.cb++;
		counter.noteCount++;
		LR2::NoteStruct* nextNote = game.gameplay.bmsobj_note[lane].note_count + multibadIndent < game.gameplay.bmsobj_note[lane].size ? &game.gameplay.bmsobj_note[lane].notes[game.gameplay.bmsobj_note[lane].note_count + multibadIndent] : nullptr;
		if (nextNote != nullptr && std::abs(timing - (int)nextNote->realTiming) <= game.gameplay.player[player].judgetime[2]) {
			CalcJudgementSN(lane, keypress, timing, player, nextNote, multibadIndent + 1);
		}
	}
	else if ((int)note.realTiming - timing < game.gameplay.player[player].judgetime[1]) {
		counter.epr++;
	}
}

void Numbers::CalcJudgementLN(int lane, int keypress, int timing, int player, void* noteIn) {
	LR2::NoteStruct& note = *(LR2::NoteStruct*)noteIn;

	Numbers& numbers = *(Numbers*)(LR2HackBox::Get().mNumbers.get());
	LR2::game& game = *LR2HackBox::Get().GetGame();

	Judgements& counter = numbers.mJudgeCountColumns[lane];

	if (game.gameplay.player[player].note_current >= game.gameplay.player[player].totalnotes) return;

	if ((double)timing - note.realTiming > (double)game.gameplay.player[player].judgetime[3] && note.active <= 0) {
		counter.lpr++;
		counter.cb++;
		counter.noteCount++;
		return;
	}

	if (keypress == 1) {
		int gap = abs(timing - (int)note.realTiming);
		int offset = timing - (int)note.realTiming;
		bool isFast = offset < 0;

		if (gap <= game.gameplay.player[player].judgetime[5]) {
			numbers.mLastLNFast[lane] = isFast;
		}
		else if (gap <= game.gameplay.player[player].judgetime[4]) {
			numbers.mLastLNFast[lane] = isFast;
			isFast ? counter.fast++ : counter.slow++;
		}
		else if (gap <= game.gameplay.player[player].judgetime[3]) {
			numbers.mLastLNFast[lane] = isFast;
			isFast ? counter.fast++ : counter.slow++;
		}
		else if (gap <= game.gameplay.player[player].judgetime[2]) {
			numbers.mLastLNFast[lane] = isFast;
			isFast ? counter.fast++ : counter.slow++;
		}
		else if ((int)note.realTiming - timing < game.gameplay.player[player].judgetime[1]) {
			counter.epr++;
		}
		return;
	}
	else if (keypress == 2) {
		bool isFast = numbers.mLastLNFast[lane];
		if ((int)note.realTiming_ln < timing && note.active > 0) {
			switch (note.active) {
			case 5: isFast ? counter.epg++ : counter.lpg++; break;
			case 4: isFast ? counter.egr++ : counter.lgr++; break;
			case 3: isFast ? counter.egd++ : counter.lgd++; break;
			case 2: isFast ? counter.ebd++ : counter.lbd++; counter.cb++; break;
			}
			counter.noteCount++;
		}
		return;
	}
	else if (keypress == 3 && note.active > 0) {
		bool isFast = numbers.mLastLNFast[lane];
		if (game.gameplay.player[player].judgetime[3] + timing < (int)note.realTiming_ln) {
			isFast ? counter.ebd++ : counter.lbd++;
			counter.cb++;
		}
		else {
			switch (note.active) {
			case 5: isFast ? counter.epg++ : counter.lpg++; break;
			case 4: isFast ? counter.egr++ : counter.lgr++; break;
			case 3: isFast ? counter.egd++ : counter.lgd++; break;
			case 2: isFast ? counter.ebd++ : counter.lbd++; counter.cb++; break;
			}
		}
		counter.noteCount++;
		return;
	}
}

void Numbers::OnProcSNOrLNFork(SafetyHookContext& regs) {
	Numbers& numbers = *(Numbers*)(LR2HackBox::Get().mNumbers.get());
	LR2::game& game = *LR2HackBox::Get().GetGame();

	int lane = regs.esi;
	int keypress = *(int*)(regs.esp + 0x18);
	int timing = *(int*)(regs.esp + 0x1C);
	int player = *(int*)(regs.esp + 0x20);

	if (game.gameplay.replay.status == 2) return;
	if (!numbers.mIsBattle && player != 0) return;


	LR2::NoteStruct* note = &game.gameplay.bmsobj_note[lane].notes[game.gameplay.bmsobj_note[lane].note_count];
	
	bool isLN = note->realTiming < note->realTiming_ln;
	if (!isLN) {
		CalcJudgementSN(lane, keypress, timing, player, note);
	}
	else {
		CalcJudgementLN(lane, keypress, timing, player, note);
	}
}

static int* SetGuiMapping(int* arr, int keymode, bool P2, bool battle) {
	constexpr int mapping5KP1[] = { 0, 1, 2, 3, 4, 5 };
	constexpr int mapping5KP2[] = { 1, 2, 3, 4, 5, 0 };
	constexpr int mapping7KP1[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	constexpr int mapping7KP2[] = { 1, 2, 3, 4, 5, 6, 7, 0 };
	constexpr int mapping9K[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	constexpr int mapping10K[] = { 0, 1, 2, 3, 4, 5, 11, 12, 13, 14, 15, 10 };
	constexpr int mapping14K[] = { 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 15, 16, 17, 10 };
	switch (keymode) {
	case 5:
		if (battle) memcpy(arr, mapping10K, sizeof(mapping10K));
		else if (!P2) memcpy(arr, mapping5KP1, sizeof(mapping5KP1));
		else memcpy(arr, mapping5KP2, sizeof(mapping5KP2));
		return arr;
	case 7:
		if (battle) memcpy(arr, mapping14K, sizeof(mapping14K));
		else if (!P2) memcpy(arr, mapping7KP1, sizeof(mapping7KP1));
		else memcpy(arr, mapping7KP2, sizeof(mapping7KP2));
		return arr;
	case 9:
		memcpy(arr, mapping9K, sizeof(mapping9K));
		return arr;
	case 10:
		memcpy(arr, mapping10K, sizeof(mapping10K));
		return arr;
	case 14:
		memcpy(arr, mapping14K, sizeof(mapping14K));
		return arr;
	default:
		return arr;
	}
}

void Numbers::SceneInit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();

	mKeymode = game.sSelect.metaSelected.keymode;
	mIsBattle = (mKeymode < 10 && game.config.play.battle == 1);
	mKeycount = mKeymode < 10 ? (mKeymode == 9 ? 9 : mKeymode + 1) : mKeymode + 2;
	if (mIsBattle && mKeycount < 10) mKeycount *= 2;

	mJudgeCountColumns.fill(Judgements());
	mLastLNFast.fill(false);

	mGuiMapping.clear();
	mGuiMapping.resize(mKeycount);
	SetGuiMapping(mGuiMapping.data(), mKeymode, mIsP2Flip, mIsBattle);
}

bool Numbers::Init(uintptr_t moduleBase) {
	Numbers::mModuleBase = moduleBase;

	ConfigManager& config = *LR2HackBox::Get().mConfig;
	mIsP2Flip = config.ReadValue("bColumnStatsP2", mIsP2Flip);
	SetGuiMapping(mGuiMapping.data(), mKeymode, mIsP2Flip, mIsBattle);

	ImGuiInjector::Get().AddMenu(&mColumnStatsMenu);

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0193EB), OnProcSNOrLNFork));

	return true;
}

bool Numbers::Deinit() {
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

void Numbers::ColumnStatsMenu() {
	const float scale = ImGui::GetStyle().FontScaleMain;
	float paddingHeight = 4.f * scale;
	if ((static_cast<int>(scale * 10) % 5) != 0) {
		paddingHeight = 3.f * scale;
	}
	ImVec2 oldCursorPos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(-10000,-10000));
	ImGui::TextUnformatted("PGREAT:");
	const float rowHeight = ImGui::GetItemRectSize().y + paddingHeight;
	ImGui::SetCursorPos(oldCursorPos);
	const float scrollHeight = 14.f * scale;
	constexpr const int judgeRowCount = 10;
	constexpr const char* notations[] = { "PGREAT:", " GREAT:", "  GOOD:", "   BAD:", "  POOR:", "E.POOR:", "  FAST:", "  SLOW:", "    CB:", "   EX%:"};
	static_assert(std::size(notations) == judgeRowCount);
	ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;
	if (ImGui::BeginTable("JudgementCounterTable", mKeycount + 2, tableFlags, ImVec2(0.f, static_cast<float>(judgeRowCount) * rowHeight + scrollHeight))) {
		ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("PGREAT:").x);
		for (int columnIdx = 0; columnIdx < mKeycount + 1; columnIdx++) {
			std::string columnName = "Column##" + (columnIdx + 1);
			ImGui::TableSetupColumn(columnName.c_str(), ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("100.00").x);
		}
		Judgements total = GetTotalJudgements();
		for (int rowIdx = 0; rowIdx < judgeRowCount; rowIdx++) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(notations[rowIdx]);
			for (int columnIdx = 0; columnIdx < mKeycount + 1; columnIdx++) {
				Judgements& judgements = columnIdx == mKeycount ? total : mJudgeCountColumns[mGuiMapping[columnIdx]];
				ImGui::TableSetColumnIndex(columnIdx + 1);
				if (columnIdx < mKeycount) {
					if (mGuiMapping[columnIdx] == 0 || mGuiMapping[columnIdx] == 10) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(189, 0, 0, 100));
					}
					else {
						if (mGuiMapping[columnIdx] % 2 == 0) {
							ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 0, 139, 100));
						}
						else {
							ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(230, 230, 230, 100));
						}
					}
				}
				switch (rowIdx) {
				case 0:
					ImGui::Text("%d", judgements.epg + judgements.lpg);
					break;
				case 1:
					ImGui::Text("%d", judgements.egr + judgements.lgr);
					break;
				case 2:
					ImGui::Text("%d", judgements.egd + judgements.lgd);
					break;
				case 3:
					ImGui::Text("%d", judgements.ebd + judgements.lbd);
					break;
				case 4:
					ImGui::Text("%d", judgements.lpr);
					break;
				case 5:
					ImGui::Text("%d", judgements.epr);
					break;
				case 6:
					ImGui::Text("%d", judgements.fast);
					break;
				case 7:
					ImGui::Text("%d", judgements.slow);
					break;
				case 8:
					ImGui::Text("%d", judgements.cb);
					break;
				case 9:
					if (judgements.noteCount) ImGui::Text("%.2f", static_cast<float>((judgements.epg + judgements.lpg) * 2 + judgements.egr + judgements.lgr) / (judgements.noteCount * 2) * 100.f);
					else ImGui::Text("%.2f", 100.f);
					break;
				}
			}
		}
		ImGui::EndTable();
	}
}

void Numbers::Menu() {
	struct IdPopper { IdPopper(const char* id) { ImGui::PushID(id); };  ~IdPopper() { ImGui::PopID(); } } _id_popper{ "Numbers" };

	ConfigManager& config = *LR2HackBox::Get().mConfig;
	constexpr const char* columnStatsHelp = "This is a table with all the judgement statistics for each column individually. Coloured columns represent their respective columns, while right-most column is their total. Autoplay and Replay modes are currently unimplemented.";
	if (ImGui::TreeNode("Per-Column Stats")) {
		ImGui::SameLine();
		HelpMarker(columnStatsHelp);
		if (ImGui::Checkbox("P2 Scratch Side", &mIsP2Flip)) {
			SetGuiMapping(mGuiMapping.data(), mKeymode, mIsP2Flip, mIsBattle);
			config.WriteValueAndSave("bColumnStatsP2", mIsP2Flip);
		}
		ImGui::SameLine();
		HelpMarker("Change the side of scratch column between P1 and P2 when playing 5K or 7K.");

		if (ImGui::Checkbox("Separate Window", &mIsWindow)) {
			mColumnStatsMenu.SetOpen(mIsWindow);
		}
		ImGui::SameLine();
		HelpMarker("Bring the table to a separate independent window, which remains open until this is toggled again, or an 'X' button is pressed in the top-right corner of that window. Useful to observe these stats during gameplay. This feature can be bound to a key in 'Binds' menu.");
		if (!mIsWindow) {
			ColumnStatsMenu();
		}
		ImGui::TreePop();
	}
	else {
		ImGui::SameLine();
		HelpMarker(columnStatsHelp);
	}
}

void Numbers::SetOpenColumnStatsMenu(bool value) {
	mIsWindow = value;
}

void Numbers::ToggleColumnStatsMenu() {
	mColumnStatsMenu.ToggleOpen();
	mIsWindow = mColumnStatsMenu.IsOpen();
}

void ColumnStatsMenu::Loop() {
	ImGui::Begin("Per-Column Stats", &mIsOpen);
	Numbers& numbers = *(Numbers*)(LR2HackBox::Get().mNumbers.get());
	numbers.ColumnStatsMenu();
	if (!mIsOpen) {
		numbers.SetOpenColumnStatsMenu(false);
	}
	ImGui::End();
}