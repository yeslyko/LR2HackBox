#include "BaseModels/ModBody.hpp"
#include "BaseModels/ImGuiMenu.hpp"
#include "BaseModels/ModFeature.hpp"
#include "BaseModels/ConfigManager.hpp"

#include "LR2Mem/LR2Bindings.hpp"

#include <unordered_map>
#include <string>
#include <chrono>

#ifndef NDEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif

#if MEMORYTRACKER
#define IFMEMORYTRACKER(x) x
#else
#define IFMEMORYTRACKER(x)
#endif

#if SHOWIMGUIDEMO
#define IFSHOWIMGUIDEMO(x) x
#else
#define IFSHOWIMGUIDEMO(x)
#endif

class LR2HackBoxMenu : public ImGuiMenu {
public:
	void Loop();
	void MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void InitBindings();
private:
	void BindingsMenu();

	bool mIsRebindMenu = false;
	IFSHOWIMGUIDEMO(bool mIsDemoMenu = false);

	std::unordered_map<std::string, int> mMenuBindings;
	std::pair<std::string, bool> mMenuBindingAwaitsRebind{ "NONE", false };
};

class LR2HackBox : public ModBody {
public:

	static LR2HackBox& Get();
	bool Hook();
	bool Unhook();

	LR2::game* GetGame();
	void* GetSqlite();

	LR2HackBoxMenu mMenu;

	ModFeature* mUnrandomizer = nullptr;
	ModFeature* mFunny = nullptr;
	ModFeature* mMisc = nullptr;
	ModFeature* mAnalogInput = nullptr;
	ModFeature* mNumbers = nullptr;
	IFMEMORYTRACKER(ModFeature* mMemoryTracker = nullptr);

	ConfigManager* mConfig = nullptr;

	std::chrono::system_clock::time_point mInitTime;
	bool mVerCheckFin = false;
	bool mVerCheckSucc = false;
	bool mIsLastVer = false;
private:
	LR2HackBox() = default;
	~LR2HackBox() = default;
	LR2HackBox(const LR2HackBox&) = delete;
	LR2HackBox& operator=(const LR2HackBox&) = delete;
};