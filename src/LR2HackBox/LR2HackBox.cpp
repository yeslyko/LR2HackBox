#define SHOWIMGUIDEMO 0

#include "Features/MemoryTracker.hpp"
#include "LR2HackBox/LR2HackBox.hpp"

#include "Helpers/Helpers.hpp"
#include "ImGuiInjector/ImGuiInjector.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "minhook/include/MinHook.h"

#include <iostream>
#include <string>
#include <thread>

#include "Features/Unrandomizer.hpp"
#include "Features/Funny.hpp"
#include "Features/Misc.hpp"
#include "Features/AnalogInput.hpp"
#include "Features/Numbers.hpp"
#include "Features/Version.hpp"
#include "Features/GameOptions.hpp"
#include "Features/ScoreCannon.hpp"
#include "Features/RivalLeaderboard.hpp"
#include "Features/JudgeLimiter.hpp"

#pragma comment(lib, "Helpers.lib")
#pragma comment(lib, "ImGuiInjector.lib")
#pragma comment(lib, "BaseModels.lib")
#pragma comment(lib, "LR2Mem.lib")

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

LR2HackBox& LR2HackBox::Get() {
	static LR2HackBox instance;
	return instance;
}

bool LR2HackBox::EarlyHook() {
	mModuleBase = (uintptr_t)GetModuleHandle(NULL);

	mLogger.SetPath("./LR2HackBox.log");

	
	mConfig.reset(new ConfigManager("LR2HackBox.ini"));

	LoadConfig();

	if (mIsCheckUpdate) {
		StartVersionCheck();
	}

	MH_Initialize();

	IFMEMORYTRACKER(mMemoryTracker.reset(new MemoryTracker()));
	IFMEMORYTRACKER(mMemoryTracker->Init(mModuleBase));

	mUnrandomizer.reset(new Unrandomizer());
	mFunny.reset(new Funny());
	mMisc.reset(new Misc());
	mAnalogInput.reset(new AnalogInput());
	mNumbers.reset(new Numbers());
	mGameOptions.reset(new GameOptions());
	mScoreCannon.reset(new ScoreCannon());
	mRivalLeaderboard.reset(new RivalLeaderboard());
	mJudgeLimiter.reset(new JudgeLimiter());

	mUnrandomizer->EarlyInit(mModuleBase);
	mFunny->EarlyInit(mModuleBase);
	mMisc->EarlyInit(mModuleBase);
	mAnalogInput->EarlyInit(mModuleBase);
	mNumbers->EarlyInit(mModuleBase);
	mGameOptions->EarlyInit(mModuleBase);
	mScoreCannon->EarlyInit(mModuleBase);
	mRivalLeaderboard->EarlyInit(mModuleBase);
	mJudgeLimiter->EarlyInit(mModuleBase);

	return true;
}

bool LR2HackBox::Hook() {
	while (!LR2::isInit) Sleep(1);

	mInitTime = std::chrono::system_clock::now();

	ImGuiInjector::Get().AddMenu(&(LR2HackBox::mMenu));

	ImGuiInjector::Get().SetGlobalScale(mGlobalScale);
	
	mMenu.InitBindings();

	mUnrandomizer->Init(mModuleBase);
	mFunny->Init(mModuleBase);
	mMisc->Init(mModuleBase);
	mAnalogInput->Init(mModuleBase);
	mNumbers->Init(mModuleBase);
	mGameOptions->Init(mModuleBase);
	mScoreCannon->Init(mModuleBase);
	mRivalLeaderboard->Init(mModuleBase);
	mJudgeLimiter->Init(mModuleBase);

	return true;
}

bool LR2HackBox::Unhook() {
	mUnrandomizer->Deinit();
	mFunny->Deinit();
	mMisc->Deinit();
	mAnalogInput->Deinit();
	mNumbers->Deinit();
	mGameOptions->Deinit();
	mScoreCannon->Deinit();
	mRivalLeaderboard->Deinit();
	mJudgeLimiter->Deinit();
	return true;
}

LR2::game* LR2HackBox::GetGame() {
	if (!LR2::isInit) return nullptr;
	return (LR2::game*)LR2::pGame;
}

void* LR2HackBox::GetSqlite() {
	if (!LR2::isInit) return nullptr;
	return LR2::pSqlite;
}

void LR2HackBox::StartVersionCheck() {
	if (mVerCheckFin) return;
	std::thread([&] {
		Version::Status version = Version::Check();
		if (version.success) {
			mIsLastVer = version.isLast;
			mVerCheckSucc = version.success;
		}
		mVerCheckFin = true;
		}
	).detach();
}

void LR2HackBox::LoadConfig() {
	mIsCheckUpdate = mConfig->ReadValue("bCheckUpdate", mIsCheckUpdate);
	mGlobalScale = mConfig->ReadValue("fGlobalScale", mGlobalScale);
}

void LR2HackBox::SettingsMenu() {
	ImGui::SetNextItemWidth(200.f * ImGui::GetStyle().FontScaleMain);
	ImGui::SliderFloat("Interface scale", &mGlobalScale, 0.5f, 2.f, "%.2f");
	ImGui::SameLine();
	if (ImGui::Button("Apply")) {
		ImGuiInjector::Get().SetGlobalScale(mGlobalScale);
		mConfig->WriteValueAndSave("fGlobalScale", mGlobalScale);
	}

	if (ImGui::Checkbox("Update Notification", &mIsCheckUpdate)) {
		mConfig->WriteValueAndSave("bCheckUpdate", mIsCheckUpdate);

		if (mIsCheckUpdate) {
			StartVersionCheck();
		}
	}
}

void LR2HackBoxMenu::Loop() {
	IFSHOWIMGUIDEMO(if (mIsDemoMenu) ImGui::ShowDemoWindow(&mIsDemoMenu));

	ImGui::Begin("LR2HackBox", &(LR2HackBoxMenu::mIsOpen));

	if (LR2HackBox::Get().mIsCheckUpdate) {
		if (!LR2HackBox::Get().mVerCheckFin) {
			ImGui::TextUnformatted("Checking version...");
		}
		else {
			if (LR2HackBox::Get().mVerCheckSucc) {
				if (!LR2HackBox::Get().mIsLastVer) {
					ImGui::TextUnformatted("New update available");
					ImGui::SameLine();
					ImGui::TextLinkOpenURL("here", "https://github.com/MatVeiQaaa/LR2HackBox/releases/tag/latest");
				}
			}
			else {
				ImGui::TextUnformatted("Coulnd't check version!");
			}
		}
	}

	ImRect titleBarRect = ImGui::GetCurrentWindow()->TitleBarRect();
	ImGui::PushClipRect(titleBarRect.Min, titleBarRect.Max, false);
	{
	std::chrono::hh_mm_ss timeSinceInit(std::chrono::system_clock::now() - LR2HackBox::Get().mInitTime);
		ImGuiStyle& style = ImGui::GetStyle();
		float x = titleBarRect.Max.x - (style.FramePadding.x + ImGui::GetCurrentContext()->FontSize + style.ItemInnerSpacing.x + ImGui::CalcTextSize("Time running: 00:00:00").x);
		float y = titleBarRect.Min.y + style.FramePadding.y;
		ImGui::GetWindowDrawList()->AddText({ x, y }, IM_COL32_WHITE, std::format("Time running: {:%X}", timeSinceInit).c_str());
	}
	ImGui::PopClipRect();

	IFSHOWIMGUIDEMO(ImGui::Checkbox("Show Demo", &mIsDemoMenu));

	if (ImGui::CollapsingHeader("Unrandomizer")) {
		LR2HackBox::Get().mUnrandomizer->Menu();
	}

	if (ImGui::CollapsingHeader("Miscellaneous")) {
		LR2HackBox::Get().mMisc->Menu();
	}

	if (ImGui::CollapsingHeader("Funny")) {
		LR2HackBox::Get().mFunny->Menu();
	}

	if (ImGui::CollapsingHeader("Numbers")) {
		LR2HackBox::Get().mNumbers->Menu();
	}

	if (ImGui::CollapsingHeader("Discord Score Webhook")) {
		LR2HackBox::Get().mScoreCannon->Menu();
	}

	if (ImGui::CollapsingHeader("Judge Limiter")) {
		LR2HackBox::Get().mJudgeLimiter->Menu();
	}

	if (ImGui::CollapsingHeader("Game Options")) {
		LR2HackBox::Get().mGameOptions->Menu();
	}

	if (ImGui::CollapsingHeader("Settings")) {
		LR2HackBox::Get().SettingsMenu();
	}

	IFMEMORYTRACKER(
		if (ImGui::CollapsingHeader("MemoryTracker")) {
			LR2HackBox::Get().mMemoryTracker->Menu();
		}
	)

	{
		ImVec2 oldCursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(-10000.f, -10000.f));
		ImGui::Button("Binds##offscreen");
		ImGui::SetCursorPos(oldCursorPos);
		ImVec2 size = ImGui::GetItemRectSize();
		float cursorOffset = ImGui::GetCursorPosY() - ImGui::GetWindowHeight();
		ImVec2 pos;
		float scale = ImGui::GetStyle().FontScaleMain;
		// All that math can be optimized... but alas.
		if (cursorOffset > 0.f - size.y - 12.f * scale) {
			pos = { ImGui::GetWindowWidth() - ImGui::GetCurrentWindow()->ScrollbarSizes.x , ImGui::GetWindowHeight() + cursorOffset + size.y + 12.f * scale };
		}
		else {
			pos = { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() };
		}
		ImGui::SetCursorPos(ImVec2(pos.x - size.x - 10.f * scale, pos.y - size.y - 10.f * scale));
		if (ImGui::Button("Binds")) {
			mIsRebindMenu = true;
		}
		BindingsMenu();
	}

	ImGui::End();
}

void LR2HackBoxMenu::BindingsMenu() {
	if (!mIsRebindMenu) return;

	ImGui::Begin("Rebind Buttons", &mIsRebindMenu);
	ImGui::SetWindowFocus();
	for (auto& binding : mMenuBindings) {
		ImGui::PushID(binding.first.c_str());
		ImGui::TextUnformatted(binding.first.c_str());
		ImGui::SameLine();
		std::string keycodeName = "NONE";
		if (binding.second.vKey) {
			WORD vkeyChar = LOWORD(MapVirtualKey(binding.second.vKey, MAPVK_VK_TO_CHAR));
			if (vkeyChar) {
				keycodeName = vkeyChar;
			}
			else {
				keycodeName = std::string("VK_") + std::to_string(binding.second.vKey);
			}
		}

		if (mMenuBindingAwaitsRebind.first == binding.first) {
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Button), IM_COL32(0, 0, 210, 255));
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(230, 230, 230, 255));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Button), IM_COL32(0, 0, 139, 255));
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(230, 230, 230, 255));
		}
		float scale = ImGui::GetStyle().FontScaleMain;
		if (ImGui::Button(mMenuBindingAwaitsRebind.first == binding.first ? "?" : keycodeName.c_str(), ImVec2(46.f * scale, 23.f * scale))) {
			mMenuBindingAwaitsRebind = { binding.first, true };
		}
		ImGui::PopStyleColor(2);
		if (mMenuBindingAwaitsRebind.first == binding.first && !binding.second.essential) {
			ImGui::SameLine();
			if (ImGui::Button("Remove", ImVec2(0.f, 23.f * scale))) {
				SetBind(binding.first.c_str(), 0);
				mMenuBindingAwaitsRebind = { "NONE", false };
			}
		}
		ImGui::PopID();
	}
	if (mMenuBindingAwaitsRebind.second) ImGui::SetNextFrameWantCaptureKeyboard(true);

	ImGui::End();
}

void LR2HackBoxMenu::SetBind(const char* name, int vKey) {
	mMenuBindings[name].vKey = vKey;

	ConfigManager& config = *LR2HackBox::Get().mConfig;
	std::string configEntry = name;
	configEntry.erase(std::remove_if(configEntry.begin(), configEntry.end(), isspace), configEntry.end());
	configEntry = "iBind" + configEntry;
	config.WriteValueAndSave(configEntry, vKey);
}

void LR2HackBoxMenu::InitBindings() {
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	mMenuBindings["Menu Open"] = Binding(config.ReadValue("iBindMenuOpen", VK_INSERT), true);
	mMenuBindings["Stats Open"] = Binding(config.ReadValue("iBindStatsOpen", 0), false);
}

void LR2HackBoxMenu::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_KEYDOWN: {
		if (wParam == mMenuBindings["Menu Open"].vKey) {
			LR2HackBoxMenu::ToggleOpen();
		}
		if (wParam == mMenuBindings["Stats Open"].vKey) {
			((Numbers*)LR2HackBox::Get().mNumbers.get())->ToggleColumnStatsMenu();
		}

		if (mMenuBindingAwaitsRebind.second) {
			SetBind(mMenuBindingAwaitsRebind.first.c_str(), wParam);
			mMenuBindingAwaitsRebind = { "NONE", false };
		}
		break;
	}
	}
}