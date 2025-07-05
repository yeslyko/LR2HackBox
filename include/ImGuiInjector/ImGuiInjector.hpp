#include <vector>
#include <windows.h>
#include "BaseModels/ImGuiMenu.hpp"
#include <imgui/imgui.h>

class ImGuiInjector {
public:
	static ImGuiInjector& Get();
	int Init();
	void SetWndProcHook(HWND hWnd);
	HWND GetWindowHandle();
	WNDPROC GetPreviousWndProc();
	bool WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void AddMenu(ImGuiMenu* pMenu);
	void RunMenus();
	bool IsMenuRunning();
	bool WantsMouseInput();
	bool WantsKeyboardInput();
	void ResetInput();
	void UpdateInput();
	void LoadJapaneseFont();
	void SetStartingStyle(const ImGuiStyle& style);
	void SetGlobalScale(float scale);
	void UpdateGlobalScale();
private:
	ImGuiInjector();
	~ImGuiInjector() = default;
	ImGuiInjector(const ImGuiInjector&) = delete;
	ImGuiInjector& operator=(const ImGuiInjector&) = delete;

	void HookDinput();
	int mDinputVer = 0;

	bool mWantsMouseInput = false;
	bool mWantsKeyboardInput = false;

	HWND mWindowHandle = NULL;
	LONG_PTR mWndProcPtr = NULL;
	WNDPROC mPreviousWndProc = nullptr;

	ImGuiStyle mStartingStyle;
	float mGlobalScale = 1.f;
	bool mScaleChanged = false;

	std::vector<ImGuiMenu*> mMenus;
};