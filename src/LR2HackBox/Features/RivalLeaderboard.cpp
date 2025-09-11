#include "RivalLeaderboard.hpp"

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <fstream>
#include <codecvt>
#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"

#include <safetyhook.hpp>

int RivalLeaderboard::OnRankingAutoUpdate(void* g) {
	RivalLeaderboard& rivalLb = *(RivalLeaderboard*)LR2HackBox::Get().mRivalLeaderboard.get();
	if (!rivalLb.mIsEnabled) return rivalLb.oRankingAutoUpdate.ccall<int>(g);
	LR2::game& game = *(LR2::game*)g;
	if (std::string_view(game.sSelect.bmsList[game.sSelect.cur_song].hash.body) == rivalLb.mCurSongHash) return rivalLb.oRankingAutoUpdate.ccall<int>(g);
	rivalLb.mCurSongHash = game.sSelect.bmsList[game.sSelect.cur_song].hash.body;
	rivalLb.mCurSongRivalData.clear();
	{
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
		rivalLb.mCurSongRivalData.push_back(self);
	}

	struct RivalPath {
		std::filesystem::path db;
		std::filesystem::path lr2folder;
	};
	std::vector<RivalPath> rivalPaths;
	{
		std::unordered_map<std::string, std::filesystem::path> dbs;
		std::unordered_map<std::string, std::filesystem::path> lr2folders;
		for (auto& file : std::filesystem::directory_iterator("LR2files/Rival")) {
			auto path = file.path();
			if (path.extension().string() == ".db") dbs[path.stem().string()] = path;
			else if (path.extension().string() == ".lr2folder") lr2folders[path.stem().string()] = path;
		}
		for (auto& db : dbs) {
			if (!lr2folders.contains(db.first)) continue;
			rivalPaths.push_back({ db.second, lr2folders[db.first] });
		}
	}
	for (auto& path : rivalPaths) {
		RANKINGPLAYER rival;
		{
			std::ifstream file(path.lr2folder);
			std::string line;
			while (std::getline(file, line)) {
				if (line.starts_with("#MAXTRACKS")) {
					std::string sLr2id = line.substr(11);
					int lr2id{};
					auto [ptr, ec] = std::from_chars(sLr2id.data(), sLr2id.data() + sLr2id.size(), lr2id);
					if (ec == std::errc()) {
						rival.id = lr2id;
					}
				}
				else if (line.starts_with("#TITLE")) {
					std::string name = line.substr(7);
					rival.name = name;
				}
			}
		}
		{
			int(__cdecl* SQL_Run)(LR2::CSTR, void*) = (decltype(SQL_Run))0x4442F0;
			int(__cdecl* SQL_Prepare)(LR2::CSTR, void*, void**) = (decltype(SQL_Prepare))0x4443F0;
			int(__cdecl* sqlite3_step)(void*) = (decltype(sqlite3_step))0x48C480;
			int(__cdecl* sqlite3_finalize)(void*) = (decltype(sqlite3_finalize))0x485FD0;
			int(__cdecl* sqlite3_column_int)(void*, int) = (decltype(sqlite3_column_int))0x47AEC0;

			void* sqlite = LR2HackBox::Get().GetSqlite();
			void* pStmt;
			std::string attachQuerry = std::format("ATTACH '{}' AS rivaldb", path.db.string());
			std::string querry = std::format("SELECT r_clear,r_totalnotes,r_maxcombo,r_perfect,r_great,r_good,r_bad,r_poor,r_minbp FROM rival WHERE hash = '{}'", game.sSelect.bmsList[game.sSelect.cur_song].hash.body);
			SQL_Run("DETACH rivaldb", sqlite);
			SQL_Run(attachQuerry.c_str(), sqlite);
			SQL_Prepare(querry.c_str(), sqlite, &pStmt);
			bool success = false;
			if (sqlite3_step(pStmt) == 100) {
				rival.clear = sqlite3_column_int(pStmt, 0);
				rival.notes = sqlite3_column_int(pStmt, 1);
				rival.combo = sqlite3_column_int(pStmt, 2);
				rival.pg = sqlite3_column_int(pStmt, 3);
				rival.gr = sqlite3_column_int(pStmt, 4);
				rival.gd = sqlite3_column_int(pStmt, 5);
				rival.bd = sqlite3_column_int(pStmt, 6);
				rival.pr = sqlite3_column_int(pStmt, 7);
				rival.minbp = sqlite3_column_int(pStmt, 8);
				success = true;
			}
			sqlite3_finalize(pStmt);
			SQL_Run("DETACH rivaldb", sqlite);
			if (!success) continue;
		}
		rivalLb.mCurSongRivalData.push_back(rival);
	}
	std::sort(rivalLb.mCurSongRivalData.begin(), rivalLb.mCurSongRivalData.end(), [](RANKINGPLAYER& first, RANKINGPLAYER& second) {
		int exFirst = first.pg * 2 + first.gr;
		int exSecond = second.pg * 2 + second.gr;
		return exFirst >= exSecond;
		});
	return rivalLb.oRankingAutoUpdate.ccall<int>(g);
}

void RivalLeaderboard::OnCheckLeaderboardInput(SafetyHookContext& regs) {
	RivalLeaderboard& rivalLb = *(RivalLeaderboard*)LR2HackBox::Get().mRivalLeaderboard.get();
	if (!rivalLb.mIsEnabled) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();

	double(__cdecl* GetTimeLapse)(unsigned int timerID, LR2::Timer* T) = (decltype(GetTimeLapse))0x4B6B10;
	int(__cdecl* SetTimeLapse)(int timerID, LR2::Timer* T) = (decltype(SetTimeLapse))0x4B6B80;
	int(__cdecl* ResetTimeLapse)(int timerID, LR2::Timer* T) = (decltype(ResetTimeLapse))0x4B67D0;

	if ((game.KeyInput.p1_buttonInput[13] == 2 || game.KeyInput.p2_buttonInput[13] == 2) && (game.KeyInput.p1_buttonInput[4] == 2 || game.KeyInput.p2_buttonInput[4] == 2) && GetTimeLapse(175, &game.timer1) == -1.0 && game.KeyInput.p1_buttonInput[11] == 0 && game.KeyInput.p1_buttonInput[10] == 0 && game.KeyInput.inputID[0xC8] == 0 && game.KeyInput.inputID[0xD0] == 0 && game.sSelect.panel != 1) {
		game.net.rankingData.rankingCount = rivalLb.mCurSongRivalData.size();
		for (int i = 0; i < rivalLb.mCurSongRivalData.size(); i++) {
			auto& rival = rivalLb.mCurSongRivalData.at(i);
			auto& ranking = game.net.rankingData.ranking[i];
			ranking.name.assign(rival.name.c_str());
			ranking.id = rival.id;
			ranking.clear = rival.clear;
			ranking.notes = rival.notes;
			ranking.combo = rival.combo;
			ranking.pg = rival.pg;
			ranking.gr = rival.gr;
			ranking.gd = rival.gd;
			ranking.bd = rival.bd;
			ranking.pr = rival.pr;
			ranking.minbp = rival.minbp;
			ranking.playcount = 1;
			ranking.ranking = i + 1;
		}
		return;
	}
}

bool RivalLeaderboard::Init(uintptr_t moduleBase) {
	RivalLeaderboard::mModuleBase = moduleBase;

	ConfigManager& config = *LR2HackBox::Get().mConfig;
	mIsEnabled = config.ReadValue("bRivalLeaderboard", mIsEnabled);

	oRankingAutoUpdate = safetyhook::create_inline(moduleBase + 0x0181b0, OnRankingAutoUpdate);
	mMidHooks.push_back(safetyhook::create_mid(moduleBase + 0x0282D1, OnCheckLeaderboardInput));

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