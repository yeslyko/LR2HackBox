#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>

class GameOptions : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();
	void Menu();

	int mAdjustP2 = 0;
	bool mAutoadjustP2 = false;
private:
	std::vector<SafetyHookMid> mMidHooks;

	float mIRWaitTime = 10.f;
};