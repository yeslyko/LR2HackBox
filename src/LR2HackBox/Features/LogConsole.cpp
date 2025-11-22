#include "LogConsole.hpp"

#include "LR2HackBox/LR2HackBox.hpp"

#include "imgui/imgui.h"

bool LogConsole::EarlyInit(uintptr_t moduleBase) {
	mModuleBase = moduleBase;
    Clear();
    mIsInit = true;
	return true;
}

bool LogConsole::Deinit() {
    mIsInit = false;
	return true;
}

void LogConsole::Clear() {
    mLogItems.clear();
}

void LogConsole::Menu() {
    struct IdPopper { IdPopper(const char* id) { ImGui::PushID(id); };  ~IdPopper() { ImGui::PopID(); } } _id_popper{ "LogConsole" };

    if (ImGui::BeginPopup("Options")) {
        ImGui::Checkbox("Auto-scroll", &mIsAutoScroll);
        ImGui::EndPopup();
    }

    if (ImGui::Button("Options")) ImGui::OpenPopup("Options");
    ImGui::SameLine();
    bool clear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");

    ImGui::Separator();

    if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        /*if (test) {
            static int counter = 0;
            const char* categories[3] = { "info", "warn", "error" };
            const char* words[] = { "Bumfuzzled", "Cattywampus", "Snickersnee", "Abibliophobia", "Absquatulate", "Nincompoop", "Pauciloquent" };
            for (int n = 0; n < 5; n++)
            {
                LogLevel category = LogLevel(counter % 3);
                const char* word = words[counter % IM_ARRAYSIZE(words)];
                AddLog(category, "{:05d} {} Hello, current time is {:.2f}, here's a word: '{}'\n",
                    ImGui::GetFrameCount(), categories[category], ImGui::GetTime(), word);
                counter++;
            }
        }*/
        if (clear) Clear();
        if (copy) ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        {
            ImGuiListClipper clipper;
            clipper.Begin(mLogItems.size());
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    LogItem& message = mLogItems[line_no];
                    ImVec4 color;
                    switch (message.level) {
                    case LogLevel::LOG_INFO:
                        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        break;
                    case LogLevel::LOG_WARNING:
                        color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                        break;
                    case LogLevel::LOG_ERROR:
                        color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                        break;
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(message.message.c_str());
                    ImGui::PopStyleColor();
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (mIsAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}