#include "BaseModels/ModFeature.hpp"

#include <safetyhook.hpp>

#include <stdint.h>

class Misc : public ModFeature {
public:
	bool EarlyInit(uintptr_t moduleBase);
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void SetMetronome(bool value);

	void Menu();

	template<typename T>
	static bool SqliteGetColumn(T* output, std::string querry, int columnIdx);

private:
	struct CustomSelectEntry {
		std::string title;
		std::string condition;
		std::string order;
		CustomSelectEntry() = default;
		CustomSelectEntry(std::string title, std::string condition = "", std::string order = "") 
			: title(title), condition(condition), order(order) {};
	};
	void LoadConfig();
	void SetHooks();

	static void OnSetRetryFlag(SafetyHookContext& regs);
	static void OnPlayISetSelecter(SafetyHookContext& regs);
	void OnInit(SafetyHookContext& regs);
	static void OnInitPlay(SafetyHookContext& regs);
	static void OnInitRetry(SafetyHookContext& regs);

	int mOrigGaugeType = 0;

	static void OnRandomMixInput(SafetyHookContext& regs);
	static void OnSceneInitSwitch(SafetyHookContext& regs);
	static void OnSceneProcSwitch(SafetyHookContext& regs);
	static void OnSceneExitSwitch(SafetyHookContext& regs);
	static void OnOpenFolderPlaySound(SafetyHookContext& regs);
	void OnDecideInit();
	void OnPlayInit();
	void OnResultInit();
	void OnCourseResultInit();
	void OnSelectExit();
	void OnPlayExit();

	void CustomSelect();
	void CustomSelectMenu();
	void CustomSelectAddDefault();
	void CustomSelectLoadConfig();
	void CustomSelectSaveConfig();
	bool mOnCustomSelEntry = false;
	std::vector<CustomSelectEntry> mCustomSelectEntries;
	decltype(mCustomSelectEntries.begin()) mSelectedCustomSelectEntry = mCustomSelectEntries.end();
	CustomSelectEntry mEditingCustomSelectEntry;

	static void OnAddToAvgBpmSum(SafetyHookContext& regs);
	static void OnCalcAvgSpeedmult(SafetyHookContext& regs);

	std::unordered_map<double, int> mMainBPMBpmRefcount;

	static void OnDrawNotesGetSongtimer(SafetyHookContext& regs);

	int mMetronomeLastPlayedBeat = 0;
	int mMetronomePrevMeasureIdx = -1;

	static int OnSaveDrawScreenToPNG(int x1, int y1, int x2, int y2, const char* FileName, int CompressionLevel);

	static void OnBeforeAddDrawingBuffer_LN(SafetyHookContext& regs);
	static int OnAddDrawingBuffer_LN(void* drb, void* srcLs, void* srcLe, void* srcLb, void* dst, void* T, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY);
	int OnAddDrawingBuffer_LN_Fixed(void* drb, void* srcLs, void* srcLe, void* srcLb, void* dst, void* T, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY, void* lnObj);

	void* mCurrentDrawingLNObj = nullptr;

	static void OnAutoadjustInc(SafetyHookContext& regs);
	static void OnAutoadjustDec(SafetyHookContext& regs);

	static int OnSetFirstSkins(void* g);
	static void OnWriteConfigXml(SafetyHookContext& regs);

	int mAutoadjustClampMin = -100;
	int mAutoadjustClampMax = 100;

	static int OnSetObjectString(unsigned int num, void* string, void** objectList);

	void SetAutoadjustReset(bool value);

	int mAutoadjustResetLastVal = 0;

	void MirrorGearshift(bool mirror);

	void SetSkipResultWaitIR(bool value);
	void SetSkipResultSub(bool value);

	static void OnIrSendSuccess(SafetyHookContext& regs);

	std::vector<SafetyHookMid> mMidHooks;

	bool mIsRetryTweaks = false;
	bool mIsCustomSelect = false;
	bool mIsMainBPM = false;
	bool mIsMetronome = false;
	bool mIsRerouteScreenshots = false;
	bool mIsScreenshotsCopybuffer = false;
	bool mIsMirrorGearshift = false;
	bool mIsAnalogInput = false;
	bool mIsLNAnimFix = false;
	bool mIsAutoadjustClamp = false;
	bool mIsAutoadjustReset = false;
	bool mIsCourseResultFix = false;
	bool mIsSkipResultWaitIR = false;
	bool mIsSkipResultSub = false;
	bool mIsResultQuickIR = false;
};