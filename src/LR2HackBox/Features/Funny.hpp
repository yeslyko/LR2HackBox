#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>

#include <stdint.h>
class Funny : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

private:
	static void OnDrawNote(SafetyHookContext& regs);
	static int OnProcSinglenote(void* g, int lane, int keypress, int timing, int player);
	safetyhook::InlineHook oProcSinglenote;

	int(__stdcall* FMOD_Channel_GetPan)(void*, float*) = nullptr;
	int(__stdcall* FMOD_Channel_SetPan)(void*, float) = nullptr;

	std::vector<SafetyHookMid> mMidHooks;

	bool mIsInvisibleScratch = false;
	bool mIsMetronome = false;
	bool mIsSpatialKeysounds = false;
};