#include "Features/JudgeLimiter.hpp"
#include "Features/Misc.hpp"

#include "LR2HackBox/LR2HackBox.hpp"

#include <safetyhook.hpp>
#include "imgui/imgui.h"

/*
struct gameplay {
    char unused[0x73de9];
    char flag_closingPhase;
    char unused2[0x7d22];
    int pg, gr, gd, bd, pr;
    char unused3[0x2f0];
    int totalnotes;
};

struct game {
    char unused[0x23db8];
    int procPhase;
    int unused2;
    struct gameplay gameplay;
};

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T **ppOriginal) {
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID *>(ppOriginal));
}

typedef int (__cdecl *ApplyJudgeNoteType)(int, game *, int, int, void *, char);
ApplyJudgeNoteType originalApplyJudgeNote = (ApplyJudgeNoteType)0x405fb0;
int max_judge[4];
int max_bp;
float min_percent_score;

typedef int(__cdecl *SetTimeLapseType)(int, void *);
SetTimeLapseType SetTimeLapse = (SetTimeLapseType)0x4b6b80;

int __cdecl hkApplyJudgeNote(int judge, game *g, int player, int lane, void *T, char isReplay) {
    int ret = originalApplyJudgeNote(judge, g, player, lane, T, isReplay);
    float score_percent = (g->gameplay.pg * 2 + g->gameplay.gd) / (float)g->gameplay.totalnotes;
    if (g->gameplay.gr >= max_judge[0]
        || g->gameplay.gd >= max_judge[1]
        || g->gameplay.bd >= max_judge[2]
        || g->gameplay.pr >= max_judge[3]
        || (g->gameplay.bd + g->gameplay.pr) >= max_bp
        || score_percent < min_percent_score) {
        SetTimeLapse(2, T);
        g->procPhase = 2;
        g->gameplay.flag_closingPhase = 1;
    }
    return ret;
}
*/

int JudgeLimiter::OnApplyJudgeNote(int judge, LR2::game *g, int player, int lane, LR2::Timer *T, char isReplay) {
    JudgeLimiter &limiter = *(JudgeLimiter *)(LR2HackBox::Get().mJudgeLimiter.get());
    int ret = limiter.oApplyJudgeNote.ccall<int>(judge, g, player, lane, T, isReplay);

    if (!limiter.mEnabled) {
        return ret;
    }

    Misc &misc = *(Misc *)(LR2HackBox::Get().mMisc.get());
    int bp = g->gameplay.player[0].judgecount[1] + g->gameplay.player[0].judgecount[2];
    if (g->gameplay.player[0].rate < limiter.mMinScore
        || (limiter.mMaxGreat >= 0 && g->gameplay.player[0].judgecount[4] >= limiter.mMaxGreat)
        || (limiter.mMaxGood >= 0 && g->gameplay.player[0].judgecount[3] >= limiter.mMaxGood)
        || (limiter.mMaxBP >= 0 && bp >= limiter.mMaxBP)) {
        misc.QuickRestart(true);
    }

    return ret;
}

bool JudgeLimiter::Init(uintptr_t moduleBase) {
    JudgeLimiter::mModuleBase = moduleBase;
    LR2::game &game = *LR2HackBox::Get().GetGame();
    ConfigManager &config = *LR2HackBox::Get().mConfig;

    mEnabled = config.ReadValue("bLimiterEnabled", 0);
    mMaxGreat = config.ReadValue("iLimiterMaxGreat", -1);
    mMaxGood = config.ReadValue("iLimiterMaxGood", -1);
    mMaxBP = config.ReadValue("iLimiterMaxBP", -1);
    mMinScore = config.ReadValue("iLimiterMinScore", 0) / 100.0f;

    oApplyJudgeNote = safetyhook::create_inline((void *)(mModuleBase + 0x005fb0), OnApplyJudgeNote);
    return true;
}

bool JudgeLimiter::Deinit() {
    return true;
}

// should probably make a better ui for this but whatever
void JudgeLimiter::Menu() {
    ConfigManager &config = *LR2HackBox::Get().mConfig;

    if (ImGui::Checkbox("Enabled", &mEnabled)) {
        config.WriteValueAndSave("bEnabled", mEnabled);
    }

    ImGui::TextUnformatted("Max. GREAT");
    ImGui::SameLine();
    if (ImGui::InputInt("##maxGreat", &mMaxGreat)) {
        config.WriteValueAndSave("iLimiterMaxGreat", mMaxGreat);
    }

    ImGui::TextUnformatted("Max. GOOD");
    ImGui::SameLine();
    if (ImGui::InputInt("##maxGood", &mMaxGood)) {
        config.WriteValueAndSave("iLimiterMaxGood", mMaxGood);
    }

    ImGui::TextUnformatted("Max. BP");
    ImGui::SameLine();
    if (ImGui::InputInt("##maxBP", &mMaxBP)) {
        config.WriteValueAndSave("iLimiterMaxBP", mMaxBP);
    }

    ImGui::TextUnformatted("Min. Score");
    ImGui::SameLine();
    if (ImGui::InputFloat("##minScore", &mMinScore)) {
        config.WriteValueAndSave("iLimiterMinScore", mMinScore * 100.0f);
    }
}