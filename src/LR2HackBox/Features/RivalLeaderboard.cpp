#include "RivalLeaderboard.hpp"

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <fstream>
#include <codecvt>
#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"

#include <safetyhook.hpp>

void RivalLeaderboard::RANKINGPLAYER_COPY(RANKINGPLAYER& copyTo, LR2::RANKINGPLAYER& copyFrom) {
	copyTo.name = copyFrom.name.body;
	copyTo.id = copyFrom.id;
	copyTo.sp = copyFrom.sp;
	copyTo.dp = copyFrom.dp;
	copyTo.clear = copyFrom.clear;
	copyTo.notes = copyFrom.notes;
	copyTo.combo = copyFrom.combo;
	copyTo.pg = copyFrom.pg;
	copyTo.gr = copyFrom.gr;
	copyTo.gd = copyFrom.gd;
	copyTo.bd = copyFrom.bd;
	copyTo.pr = copyFrom.pr;
	copyTo.minbp = copyFrom.minbp;
	copyTo.option = copyFrom.option;
	copyTo.sussussuspected = copyFrom.sussussuspected;
	copyTo.playcount = copyFrom.playcount;
	copyTo.ranking = copyFrom.ranking;
}

void RivalLeaderboard::LR2RANKINGPLAYER_COPY(LR2::RANKINGPLAYER& copyTo, RANKINGPLAYER& copyFrom) {
	copyTo.name.assign(copyFrom.name.c_str());
	copyTo.id = copyFrom.id;
	copyTo.sp = copyFrom.sp;
	copyTo.dp = copyFrom.dp;
	copyTo.clear = copyFrom.clear;
	copyTo.notes = copyFrom.notes;
	copyTo.combo = copyFrom.combo;
	copyTo.pg = copyFrom.pg;
	copyTo.gr = copyFrom.gr;
	copyTo.gd = copyFrom.gd;
	copyTo.bd = copyFrom.bd;
	copyTo.pr = copyFrom.pr;
	copyTo.minbp = copyFrom.minbp;
	copyTo.option = copyFrom.option;
	copyTo.sussussuspected = copyFrom.sussussuspected;
	copyTo.playcount = copyFrom.playcount;
	copyTo.ranking = copyFrom.ranking;
}

void RivalLeaderboard::GetRivalList() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	mRivals[game.net.IR_ID] = true;
	for (auto& file : std::filesystem::directory_iterator("LR2files/Rival")) {
		auto path = file.path();
		if (path.extension().string() != ".lr2folder") continue;
		std::ifstream file(path);
		std::string line;
		while (std::getline(file, line)) {
			if (!line.starts_with("#MAXTRACKS")) continue;
			std::string sLr2id = line.substr(11);
			int lr2id{};
			auto [ptr, ec] = std::from_chars(sLr2id.data(), sLr2id.data() + sLr2id.size(), lr2id);
			if (ec == std::errc()) {
				mRivals[lr2id] = true;
			}
		}
	}
}

void RivalLeaderboard::UpdateRivalData() {
	RivalLeaderboard& rivalLb = *(RivalLeaderboard*)LR2HackBox::Get().mRivalLeaderboard.get();
	if (!rivalLb.mIsEnabled) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	rivalLb.mCurSongRivalData.clear();

	bool useLocalBest = true;
	for (int i = 0; i < game.net.rankingData.rankingCount; i++) {
		if (rivalLb.mCurSongRivalData.size() >= mRivals.size()) break;
		auto& ranking = game.net.rankingData.ranking[i];
		if (!mRivals.contains(ranking.id)) continue;
		if (ranking.id == game.net.IR_ID) {
			LR2::STATUS& mybest = game.sSelect.bmsList[game.sSelect.cur_song].mybest;
			if (ranking.pg * 2 + ranking.gr < mybest.stat_pgreat * 2 + mybest.stat_great) {
				useLocalBest = true;
				continue;
			}
			else {
				useLocalBest = false;
			}
		}
		RANKINGPLAYER rival;
		RANKINGPLAYER_COPY(rival, ranking);
		rivalLb.mCurSongRivalData.push_back(rival);
	}
	if (useLocalBest) {
		RANKINGPLAYER self;
		self.id = game.net.IR_ID;
		self.name = game.config.player.id.body;
		LR2::STATUS& mybest = game.sSelect.bmsList[game.sSelect.cur_song].mybest;
		self.clear = mybest.clear;
		self.notes = mybest.total_notes;
		self.combo = mybest.stat_maxcombo;
		self.pg = mybest.stat_pgreat;
		self.gr = mybest.stat_great;
		self.gd = mybest.stat_good;
		self.bd = mybest.stat_bad;
		self.pr = mybest.stat_poor;
		self.minbp = mybest.minbp;
		self.ranking = mybest.IRranking;
		self.option = mybest.op_best;
		self.playcount = mybest.playcount;
		rivalLb.mCurSongRivalData.push_back(self);
	}
	std::sort(rivalLb.mCurSongRivalData.begin(), rivalLb.mCurSongRivalData.end(), [](RANKINGPLAYER& first, RANKINGPLAYER& second) {
		int exFirst = first.pg * 2 + first.gr;
		int exSecond = second.pg * 2 + second.gr;
		return exFirst >= exSecond;
	});
	return;
}

void RivalLeaderboard::OnCheckLeaderboardInput(SafetyHookContext& regs) {
	RivalLeaderboard& rivalLb = *(RivalLeaderboard*)LR2HackBox::Get().mRivalLeaderboard.get();
	if (!rivalLb.mIsEnabled) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();

	double(__cdecl* GetTimeLapse)(unsigned int timerID, LR2::Timer* T) = (decltype(GetTimeLapse))0x4B6B10;
	if ((game.KeyInput.p1_buttonInput[13] == 2 || game.KeyInput.p2_buttonInput[13] == 2) && (game.KeyInput.p1_buttonInput[4] == 2 || game.KeyInput.p2_buttonInput[4] == 2) && GetTimeLapse(175, &game.timer1) == -1.0 && game.KeyInput.p1_buttonInput[11] == 0 && game.KeyInput.p1_buttonInput[10] == 0 && game.KeyInput.inputID[0xC8] == 0 && game.KeyInput.inputID[0xD0] == 0 && game.sSelect.panel != 1) {
		rivalLb.UpdateRivalData();
		if (rivalLb.mCurSongRivalData.empty()) return;
		game.net.rankingData.rankingCount = rivalLb.mCurSongRivalData.size();
		for (int i = 0; i < rivalLb.mCurSongRivalData.size(); i++) {
			auto& rival = rivalLb.mCurSongRivalData.at(i);
			auto& ranking = game.net.rankingData.ranking[i];
			LR2RANKINGPLAYER_COPY(ranking, rival);
		}
		return;
	}
}

void RivalLeaderboard::OnFillBarForEnd(SafetyHookContext& regs) {
	RivalLeaderboard& rivalLb = *(RivalLeaderboard*)LR2HackBox::Get().mRivalLeaderboard.get();
	if (!rivalLb.mIsEnabled) return;

	int i = regs.edi / sizeof(LR2::RANKINGPLAYER);
	LR2::game& game = *LR2HackBox::Get().GetGame();
	game.sSelect.prevList[i].level = game.net.rankingData.ranking[i].ranking;
}

bool RivalLeaderboard::Init(uintptr_t moduleBase) {
	RivalLeaderboard::mModuleBase = moduleBase;

	ConfigManager& config = *LR2HackBox::Get().mConfig;
	mIsEnabled = config.ReadValue("bRivalLeaderboard", mIsEnabled);
	GetRivalList();

	mMidHooks.push_back(safetyhook::create_mid(moduleBase + 0x0282D1, OnCheckLeaderboardInput));
	mMidHooks.push_back(safetyhook::create_mid(moduleBase + 0x02859A, OnFillBarForEnd));

	return true;
}

bool RivalLeaderboard::Deinit() {
	return true;
}

void RivalLeaderboard::SetEnabled(bool value) {
	mIsEnabled = value;
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	config.WriteValueAndSave("bRivalLeaderboard", mIsEnabled);
}

bool RivalLeaderboard::GetEnabled() {
	return mIsEnabled;
}