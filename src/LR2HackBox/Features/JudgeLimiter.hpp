#include "BaseModels/ModFeature.hpp"
#include <safetyhook.hpp>

#include "LR2HackBox/LR2HackBox.hpp"

class JudgeLimiter : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

private:
	static int OnApplyJudgeNote(int judge, LR2::game *g, int player, int lane, LR2::Timer *T, char isReplay);

	SafetyHookInline oApplyJudgeNote;

	bool mIsEnabled = false;
	bool mIsNewRandom = false;
	int mMaxGreat = -1;
	int mMaxGood = -1;
	int mMaxBP = -1;
	float mMinScoreRate = 0.f;
};