#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>

class ScoreCannon : public ModFeature {
public:
	struct Score {
		enum Lamp {
			NOPLAY,
			FAILED,
			EASYCLEAR,
			GROOVECLEAR,
			HARDCLEAR,
			FULLCOMBO,
			PERFECT,
			MAX,
			ASSISTCLEAR,
			NONE
		};
		enum Target {
			NOTARGET,
			MYBEST,
			RANKAAA,
			RANKAA,
			RANKA,
			PERCENT,
			IR_TOP,
			IR_NEXT,
			IR_AVERAGE,
			TARGETERROR
		};
		Score() = default;
		Score(std::string folder, std::string title, std::string subtitle, Lamp lamp, Lamp lampBest, 
			  Target target, int exScore, int exScoreMax, int exScoreBest, int exScoreTarget,
			  int missCount, int missCountBest);
		Score(void* game);
		std::string folder;
		std::string title;
		std::string subtitle;
		Score::Lamp lamp = NONE;
		Score::Lamp lampBest = NONE;
		Score::Target target = NOTARGET;
		int exScore = 0;
		int exScoreMax = 0;
		int exScoreBest = 0;
		int exScoreTarget = 0;
		int missCount = 0;
		int missCountBest = 0;
	};
	bool Init(uintptr_t moduleBase);
	bool Deinit();
	void Menu();

	bool PostScore(const Score& score);

	bool mIsEnabled = false;
	bool mAlreadySent = false;
private:
	std::string GetJsonString(const Score& score);

	std::vector<SafetyHookMid> mMidHooks;
	std::string mDiscordName;
	std::string mDiscordAvatarUrl;
	std::string mGameName;
	std::vector<std::string> mUrls;
	std::string mEditingUrl;
};