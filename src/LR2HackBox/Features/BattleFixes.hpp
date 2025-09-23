#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>
#include <LR2Mem/LR2Typedefs.hpp>

class BattleFixes : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);

	void SceneInit();
	void SetEnabled(bool value);
	bool GetEnabled();
private:
	static void OnSetSongtimer(SafetyHookContext& regs);
	static void OnCheckMeasurelineAppear(SafetyHookContext& regs);
	static void OnSetBattleP2Y(SafetyHookContext& regs);
	static void OnCheckMeasurelineDisappear(SafetyHookContext& regs);
	static void OnDrawKeyLoop(SafetyHookContext& regs);
	static void OnSetBpmChangedBmstime(SafetyHookContext& regs);
	static int OnAddDrawingBuffer_PlayArea(LR2::DrawingBuf* drb, LR2::SRCstruct* src, LR2::DSTstruct* dst, LR2::Timer* T, float shiftX, float shiftY, int alpha, float sizeX, float sizeY, char flag);
	static void OnBeginProcNote(SafetyHookContext& regs);
	static void OnEndProcNote(SafetyHookContext& regs);
	static void OnCheckAutoadjustCondition(SafetyHookContext& regs);
	static void OnCheckMousewheelLanecover(SafetyHookContext& regs);
	static void OnShuffleNotesLoop(SafetyHookContext& regs);
	SafetyHookInline oAddDrawingBuffer_PlayArea;
	SafetyHookInline oProcNote;
	std::vector<SafetyHookMid> mMidHooks;

	bool mIsEnabled = false;
	double songtimerP1 = 0.;
	double songtimerP2 = 0.;
	double lastLineBmstime = -1.f;
	int bpmChangedBmstimeP1 = -1;
	int bpmChangedBmstimeP2 = -1;
	int autoadjust_midcountP2 = 0;
	int autoadjust_midsumP2 = 0;
	int autoadjust_lastPlayer = 0;
	int autoadjust_lastMidCount = 0;
	int autoadjust_lastMidSum = 0;
};