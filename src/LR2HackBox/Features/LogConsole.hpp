#pragma once
#include "BaseModels/ModFeature.hpp"

#include <LR2HackBox/LR2HackBox.hpp>
#include <string>
#include <vector>
#include <format>

#include <imgui/imgui.h>

enum LogLevel {
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

class LogConsole : public ModFeature {
public:
	bool EarlyInit(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

    void Clear();

	template<typename... Args>
	static void AddLog(LogLevel level, std::string_view fmt, Args&&... args) {
		LogConsole* pLogger = (LogConsole*)LR2HackBox::Get().mConsole.get();
		if (!pLogger) return;
		LogConsole& logger = *pLogger;
		logger.mLogItems.emplace_back(level, std::format("[{}] {}", logger.mLogPrefix[level], std::vformat(fmt, std::make_format_args(args...))));
	};

private:
	const char* mLogPrefix[3] = {"INFO", "WARN", "ERROR"};
	struct LogItem {
		LogLevel level;
		std::string message;
	};
	std::vector<LogItem> mLogItems;
	bool mIsInit = false;
	bool mIsAutoScroll = true;  // Keep scrolling if already at the bottom.
};