#include "BaseModels/ModFeature.hpp"
#include "BaseModels/ImGuiMenu.hpp"

#include <stdint.h>
#include <array>

#include <safetyhook.hpp>

class ColumnStatsMenu : public ImGuiMenu {
public:
	void Loop();
};

class Numbers : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

	void SceneInit();
	void ColumnStatsMenu();
	void SetOpenColumnStatsMenu(bool value);
	void ToggleColumnStatsMenu();

#pragma pack(push, 1)
	struct Judgements {
		unsigned int epg = 0;
		unsigned int lpg = 0;
		unsigned int egr = 0;
		unsigned int lgr = 0;
		unsigned int egd = 0;
		unsigned int lgd = 0;
		unsigned int ebd = 0;
		unsigned int lbd = 0;
		unsigned int epr = 0;
		unsigned int lpr = 0;
		unsigned int cb = 0;
		unsigned int fast = 0;
		unsigned int slow = 0;
		unsigned int noteCount = 0;
	};
#pragma pack(pop)
	Judgements GetTotalJudgements();

private:
	std::array<Judgements, 20> mJudgeCountColumns = {};
	std::vector<int> mGuiMapping = { 0, 1, 2, 3, 4, 5, 6, 7 };

	static void CalcJudgementSN(int lane, int keypress, int timing, int player, void* note, int multibadIndent = 1);
	static void CalcJudgementLN(int lane, int keypress, int timing, int player, void* note);
	static void OnProcSNOrLNFork(SafetyHookContext& regs);
	std::array<bool, 20> mLastLNFast = {};

	int mKeymode = 7;
	int mKeycount = 8;
	bool mIsBattle = false;
	bool mIsP2Flip = false;
	bool mIsWindow = false;
	::ColumnStatsMenu mColumnStatsMenu;

	std::vector<SafetyHookMid> mMidHooks;

};