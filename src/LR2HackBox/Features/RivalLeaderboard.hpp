#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>
#include <LR2Mem/LR2Typedefs.hpp>

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
	static void RANKINGPLAYER_COPY(RANKINGPLAYER& copyTo, LR2::RANKINGPLAYER& copyFrom);
	static void LR2RANKINGPLAYER_COPY(LR2::RANKINGPLAYER& copyTo, RANKINGPLAYER& copyFrom);

	static void OnCheckLeaderboardInput(SafetyHookContext& regs);
	static void OnFillBarForEnd(SafetyHookContext& regs);
	std::vector<SafetyHookMid> mMidHooks;

	void GetRivalList();
	void UpdateRivalData();

	std::unordered_map<int, bool> mRivals;
	std::vector<RANKINGPLAYER> mCurSongRivalData;
	std::string mCurSongHash;

	bool mIsEnabled = false;
};