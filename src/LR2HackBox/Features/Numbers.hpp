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

private:
#pragma pack(push, 1)
	struct JudgeCounter {
		int pgreat = 0;
		int great = 0;
		int good = 0;
		int bad = 0;
		int poor = 0;
		int epoor = 0;
		int fast = 0;
		int slow = 0;
		int cb = 0;
		int noteCount = 0;
	};
#pragma pack(pop)
	std::array<JudgeCounter, 20> mJudgeCountColumns = {};
	std::vector<int> mGuiMapping = { 0, 1, 2, 3, 4, 5, 6, 7 };

	static void CalcJudgementSN(int lane, int keypress, int timing, int player, void* note, int multibadIndent = 1);
	static void CalcJudgementLN(int lane, int keypress, int timing, int player, void* note);
	static void OnProcSNOrLNFork(SafetyHookContext& regs);

	int mKeymode = 7;
	int mKeycount = 8;
	bool mIsBattle = false;
	bool mIsP2Flip = false;
	bool mIsWindow = false;
	::ColumnStatsMenu mColumnStatsMenu;

	std::vector<SafetyHookMid> mMidHooks;

};