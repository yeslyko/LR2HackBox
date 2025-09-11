#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>

#include <vector>
#include <string>

class RivalLeaderboard : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void SetEnabled(bool value);
	bool GetEnabled();

private:
	struct RANKINGPLAYER {
		std::string name;
		int id = 0;
		int sp = 0;
		int dp = 0;
		int clear = 0;
		int notes = 0;
		int combo = 0;
		int pg = 0;
		int gr = 0;
		int gd = 0;
		int bd = 0;
		int pr = 0;
		int minbp = 0;
		int option = 0;
		int sussussuspected = 0;
		int playcount = 0;
		int ranking = 0;
		std::string comment;
	};
	static int OnRankingAutoUpdate(void* g);
	static void OnCheckLeaderboardInput(SafetyHookContext& regs);
	safetyhook::InlineHook oRankingAutoUpdate;
	std::vector<SafetyHookMid> mMidHooks;

	std::vector<RANKINGPLAYER> mCurSongRivalData;
	std::string mCurSongHash;

	bool mIsEnabled = false;
};