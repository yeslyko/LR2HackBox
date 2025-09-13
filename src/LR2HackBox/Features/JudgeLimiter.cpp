#define NOMINMAX
#include "JudgeLimiter.hpp"

#include "LR2HackBox/LR2HackBox.hpp"
#include "Features/Misc.hpp"

#include <safetyhook.hpp>
#include "imgui/imgui.h"

int JudgeLimiter::OnApplyJudgeNote(int judge, LR2::game *g, int player, int lane, LR2::Timer *T, char isReplay) {
    JudgeLimiter &limiter = *(JudgeLimiter *)(LR2HackBox::Get().mJudgeLimiter.get());
    int ret = limiter.oApplyJudgeNote.ccall<int>(judge, g, player, lane, T, isReplay);

    if (!limiter.mIsEnabled) return ret;

    Misc &misc = *(Misc *)(LR2HackBox::Get().mMisc.get());
    LR2::PLAYERSTATUS& playerstats = g->gameplay.player[0];
    int bp = playerstats.judgecount[1] + playerstats.judgecount[2];
    float predictedRate = (static_cast<float>(playerstats.judgecount[5] * 2 + playerstats.judgecount[4] + (playerstats.totalnotes - playerstats.note_current) * 2) / (playerstats.totalnotes * 2)) * 100.f;
    if ((limiter.mMinScoreRate >= 0.f && predictedRate < limiter.mMinScoreRate)
        || (limiter.mMaxGreat >= 0 && playerstats.judgecount[4] > limiter.mMaxGreat)
        || (limiter.mMaxGood >= 0 && playerstats.judgecount[3] > limiter.mMaxGood)
        || (limiter.mMaxBP >= 0 && bp > limiter.mMaxBP)) 
    {
        misc.QuickRestart(limiter.mIsNewRandom);
    }

    return ret;
}

bool JudgeLimiter::Init(uintptr_t moduleBase) {
    JudgeLimiter::mModuleBase = moduleBase;
    LR2::game &game = *LR2HackBox::Get().GetGame();
    ConfigManager &config = *LR2HackBox::Get().mConfig;

    mIsEnabled = config.ReadValue("bLimiterEnabled", false);
    mIsNewRandom = config.ReadValue("bLimiterNewRandom", false);
    mMaxGreat = config.ReadValue("iLimiterMaxGreat", -1);
    mMaxGood = config.ReadValue("iLimiterMaxGood", -1);
    mMaxBP = config.ReadValue("iLimiterMaxBP", -1);
    mMinScoreRate = config.ReadValue("fLimiterMinScore", -1.f);

    oApplyJudgeNote = safetyhook::create_inline((void *)(mModuleBase + 0x5FB0), OnApplyJudgeNote);

    return true;
}

bool JudgeLimiter::Deinit() {
    return true;
}

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void JudgeLimiter::Menu() {
    ConfigManager &config = *LR2HackBox::Get().mConfig;

    if (ImGui::Checkbox("Enable", &mIsEnabled)) {
        config.WriteValueAndSave("bLimiterEnabled", mIsEnabled);
    }
    ImGui::SameLine();
    HelpMarker("When enabled, if the set stat conditions can no longer be met during gameplay, the play will restart automatically. Set value to -1 to ignore a condition.");

    if (ImGui::Checkbox("Restart with new random", &mIsNewRandom)) {
        config.WriteValueAndSave("bLimiterNewRandom", mIsNewRandom);
    }

    ImVec2 oldCursorPos = ImGui::GetCursorPos();
    ImGui::Dummy(ImGui::CalcTextSize("Max. GREAT"));
    ImGui::SameLine();
    const float MAX_NOTATION_WIDTH = ImGui::GetCursorPosX();
    ImGui::SetCursorPos(oldCursorPos);

    ImGui::PushItemWidth(ImGui::CalcTextSize("100.00X").x);

    ImGui::TextUnformatted("Max. GREAT");
    ImGui::SameLine();
    ImGui::SetCursorPosX(MAX_NOTATION_WIDTH);
    if (ImGui::InputInt("##maxGreat", &mMaxGreat, 0)) {
        mMaxGreat = std::max(mMaxGreat, -1);
        config.WriteValueAndSave("iLimiterMaxGreat", mMaxGreat);
    }

    ImGui::TextUnformatted("Max. GOOD");
    ImGui::SameLine();
    ImGui::SetCursorPosX(MAX_NOTATION_WIDTH);
    if (ImGui::InputInt("##maxGood", &mMaxGood, 0)) {
        mMaxGood = std::max(mMaxGood, -1);
        config.WriteValueAndSave("iLimiterMaxGood", mMaxGood);
    }

    ImGui::TextUnformatted("Max. BP");
    ImGui::SameLine();
    ImGui::SetCursorPosX(MAX_NOTATION_WIDTH);
    if (ImGui::InputInt("##maxBP", &mMaxBP, 0)) {
        mMaxBP = std::max(mMaxBP, -1);
        config.WriteValueAndSave("iLimiterMaxBP", mMaxBP);
    }

    ImGui::TextUnformatted("Min. EX");
    ImGui::SameLine();
    ImGui::SetCursorPosX(MAX_NOTATION_WIDTH);
    if (ImGui::InputFloat("%##minScoreRate", &mMinScoreRate, 0.f, 0.f, "%.2f")) {
        mMinScoreRate = std::clamp(mMinScoreRate, -1.f, 100.f);
        config.WriteValueAndSave("fLimiterMinScore", mMinScoreRate);
    }

    ImGui::PopItemWidth();
}