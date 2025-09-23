#define NOMINMAX
#include "BattleFixes.hpp"

#include "LR2HackBox/LR2HackBox.hpp"
#include "GameOptions.hpp"
#include <print>
#include <intrin.h>

#include <safetyhook.hpp>
#include "imgui/imgui.h"

#pragma intrinsic(_ReturnAddress)

void BattleFixes::OnSetSongtimer(SafetyHookContext& regs) {
    double songtimerP1;
    __asm {
        fst songtimerP1
    }
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    GameOptions& options = *(GameOptions*)(LR2HackBox::Get().mGameOptions.get());
    double(__cdecl * GetTimeLapse)(unsigned int timerID, LR2::Timer * T) = (decltype(GetTimeLapse))0x4B6B10;
    double(__cdecl * RealTimeToBMSTime)(LR2::gameplay * gp, double time) = (decltype(RealTimeToBMSTime))0x4AD7E0;
    double songtimerP2 = RealTimeToBMSTime(&game.gameplay, GetTimeLapse(41, &game.timer1) + options.mAdjustP2);
    if (battleFixes.bpmChangedBmstimeP2 > 0) {
        songtimerP2 = battleFixes.bpmChangedBmstimeP2 * 2 - songtimerP2;
    }
    battleFixes.songtimerP1 = songtimerP1;
    battleFixes.songtimerP2 = songtimerP2;
}

void BattleFixes::OnCheckMeasurelineAppear(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    battleFixes.lastLineBmstime = *(double*)regs.ecx;
    double highestTimer = std::max(battleFixes.songtimerP1, battleFixes.songtimerP2);
    __asm {
        FFREEP ST(0)
        FLD highestTimer
    }
}

void BattleFixes::OnSetBattleP2Y(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    double p2timer = battleFixes.songtimerP2;
    __asm {
        FFREEP ST(0)
        FLD p2timer
    }
}

void BattleFixes::OnCheckMeasurelineDisappear(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    constexpr unsigned short CMP_FALSE = 30759;
    constexpr unsigned short CMP_TRUE = 14375;
    double lowestTimer = std::min(battleFixes.songtimerP1, battleFixes.songtimerP2);
    double& lineBmstiming = *(double*)(regs.edx + regs.ecx);
    if (lineBmstiming < lowestTimer) regs.eax = CMP_TRUE;
    else regs.eax = CMP_FALSE;
}

void BattleFixes::OnDrawKeyLoop(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    int keyIdx = regs.esi;
    double& songtimer = *(double*)(regs.esp + 0x80);
    if (keyIdx < 10) songtimer = battleFixes.songtimerP1;
    else songtimer = battleFixes.songtimerP2;
}

void BattleFixes::OnSetBpmChangedBmstime(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    GameOptions& options = *(GameOptions*)(LR2HackBox::Get().mGameOptions.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    double(__cdecl * GetTimeLapse)(unsigned int timerID, LR2::Timer * T) = (decltype(GetTimeLapse))0x4B6B10;
    double(__cdecl * RealTimeToBMSTime)(LR2::gameplay * gp, double time) = (decltype(RealTimeToBMSTime))0x4AD7E0;
    battleFixes.bpmChangedBmstimeP1 = regs.eax;
    battleFixes.bpmChangedBmstimeP2 = RealTimeToBMSTime(&game.gameplay, GetTimeLapse(41, &game.timer1) + options.mAdjustP2);
}

int BattleFixes::OnAddDrawingBuffer_PlayArea(LR2::DrawingBuf* drb, LR2::SRCstruct* src, LR2::DSTstruct* dst, LR2::Timer* T, float shiftX, float shiftY, int alpha, float sizeX, float sizeY, char flag) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1 || battleFixes.lastLineBmstime < 0.) return battleFixes.oAddDrawingBuffer_PlayArea.ccall<int>(drb, src, dst, T, shiftX, shiftY, alpha, sizeX, sizeY, flag);
    uintptr_t returnAddress = (uintptr_t)_ReturnAddress();
    switch (returnAddress) {
    case 0x406FF6: 
        if (battleFixes.lastLineBmstime >= battleFixes.songtimerP1) return battleFixes.oAddDrawingBuffer_PlayArea.ccall<int>(drb, src, dst, T, shiftX, shiftY, alpha, sizeX, sizeY, flag);
        else return 1;
    case 0x40703D: 
        if (battleFixes.lastLineBmstime >= battleFixes.songtimerP2) return battleFixes.oAddDrawingBuffer_PlayArea.ccall<int>(drb, src, dst, T, shiftX, shiftY, alpha, sizeX, sizeY, flag);
        else return 1;
    default: return battleFixes.oAddDrawingBuffer_PlayArea.ccall<int>(drb, src, dst, T, shiftX, shiftY, alpha, sizeX, sizeY, flag);
    }
}

void BattleFixes::OnBeginProcNote(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    battleFixes.autoadjust_lastPlayer = *(int*)(regs.esp + 0x14);
    battleFixes.autoadjust_lastMidCount = game.gameplay.autojudge_midcount;
    battleFixes.autoadjust_lastMidSum = game.gameplay.autojudge_midsum;    
}

void BattleFixes::OnEndProcNote(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    GameOptions& options = *(GameOptions*)(LR2HackBox::Get().mGameOptions.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    if (battleFixes.autoadjust_lastPlayer == 1) {
        if (battleFixes.autoadjust_lastMidCount != game.gameplay.autojudge_midcount) {
            battleFixes.autoadjust_midcountP2++;
            battleFixes.autoadjust_midsumP2 += game.gameplay.autojudge_midsum - battleFixes.autoadjust_lastMidSum;
            game.gameplay.autojudge_midcount = battleFixes.autoadjust_lastMidCount;
            game.gameplay.autojudge_midsum = battleFixes.autoadjust_lastMidSum;
        }
    }
}

void BattleFixes::OnCheckAutoadjustCondition(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    GameOptions& options = *(GameOptions*)(LR2HackBox::Get().mGameOptions.get());
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    if (game.config.play.autojudge && game.gameplay.autojudge_midcount > 9) {
        if (game.gameplay.autojudge_midsum > 0) game.config.play.judgetiming++;
        else if (game.gameplay.autojudge_midsum < 0) game.config.play.judgetiming--;

        game.gameplay.autojudge_midcount = 0;
        game.gameplay.autojudge_midsum = 0;
    }
    if (options.mAutoadjustP2 && battleFixes.autoadjust_midcountP2 > 9) {
        if (battleFixes.autoadjust_midsumP2 > 0) options.mAdjustP2++;
        else if (battleFixes.autoadjust_midsumP2 < 0) options.mAdjustP2--;

        battleFixes.autoadjust_midcountP2 = 0;
        battleFixes.autoadjust_midsumP2 = 0;
    }
}

void BattleFixes::OnCheckMousewheelLanecover(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled || game.config.play.battle != 1) return;
    int& mouseWheel = game.KeyInput.mousewheel;
    if (game.config.play.p1_lanecover == 1) {
        if (game.KeyInput.p2_buttonInput[12] || game.KeyInput.p2_buttonInput[13]) {
            game.config.play.p1_lanecoverv += mouseWheel;
        }
    }
    if (game.config.play.p2_lanecover == 1) {
        if (game.KeyInput.p2_buttonInput[12] || game.KeyInput.p2_buttonInput[13]) {
            game.config.play.p2_lanecoverv -= mouseWheel;
        }
    }
}

void BattleFixes::OnShuffleNotesLoop(SafetyHookContext& regs) {
    BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());
    LR2::game& game = *LR2HackBox::Get().GetGame();
    if (!battleFixes.mIsEnabled) return;
    unsigned int& isBattle = *(unsigned int*)(regs.esp + 0x48);
    isBattle = 0;
}

void BattleFixes::SceneInit() {
    bpmChangedBmstimeP1 = -1;
    bpmChangedBmstimeP2 = -1;
    lastLineBmstime = -1.;
    autoadjust_midcountP2 = 0;
    autoadjust_midsumP2 = 0;
    autoadjust_lastPlayer = 0;
    autoadjust_lastMidCount = 0;
    autoadjust_lastMidSum = 0;
}

static void ApplyP2ShutterFix(bool enable) {
    unsigned char* p2ShutterUp = (unsigned char*)0x4279FB;
    unsigned char* p2ShutterDown = (unsigned char*)0x427A20;

    constexpr unsigned char ADD = 0x01;
    constexpr unsigned char SUB = 0x29;

    DWORD oldProtection = 0;
    VirtualProtect(p2ShutterUp, sizeof(unsigned char), PAGE_EXECUTE_READWRITE, &oldProtection);
    VirtualProtect(p2ShutterDown, sizeof(unsigned char), PAGE_EXECUTE_READWRITE, &oldProtection);

    if (enable) {
        *p2ShutterUp = SUB;
        *p2ShutterDown = ADD;
    }
    else {
        *p2ShutterUp = ADD;
        *p2ShutterDown = SUB;
    }

    DWORD discard = 0;
    VirtualProtect(p2ShutterUp, sizeof(unsigned short), oldProtection, &discard);
    VirtualProtect(p2ShutterDown, sizeof(unsigned short), oldProtection, &discard);
}

bool BattleFixes::Init(uintptr_t moduleBase) {
    BattleFixes::mModuleBase = moduleBase;
    LR2::game &game = *LR2HackBox::Get().GetGame();
    ConfigManager &config = *LR2HackBox::Get().mConfig;

    mIsEnabled = config.ReadValue("bBattleFixes", mIsEnabled);

    mMidHooks.push_back(safetyhook::create_mid(0x406DAD, OnSetSongtimer));
    mMidHooks.push_back(safetyhook::create_mid(0x406E42, OnCheckMeasurelineAppear));
    mMidHooks.push_back(safetyhook::create_mid(0x406F86, OnSetBattleP2Y));
    mMidHooks.push_back(safetyhook::create_mid(0x407056, OnCheckMeasurelineDisappear));
    mMidHooks.push_back(safetyhook::create_mid(0x407091, OnDrawKeyLoop));
    mMidHooks.push_back(safetyhook::create_mid(0x42BC62, OnSetBpmChangedBmstime));
    oAddDrawingBuffer_PlayArea = safetyhook::create_inline(0x49D630, OnAddDrawingBuffer_PlayArea);

    mMidHooks.push_back(safetyhook::create_mid(0x419320, OnBeginProcNote));
    mMidHooks.push_back(safetyhook::create_mid(0x419410, OnEndProcNote));
    mMidHooks.push_back(safetyhook::create_mid(0x419432, OnEndProcNote));
    mMidHooks.push_back(safetyhook::create_mid(0x42C832, OnCheckAutoadjustCondition));

    mMidHooks.push_back(safetyhook::create_mid(0x427779, OnCheckMousewheelLanecover));

    mMidHooks.push_back(safetyhook::create_mid(0x4B4A30, OnShuffleNotesLoop));

    ApplyP2ShutterFix(mIsEnabled);

    return true;
}

void BattleFixes::SetEnabled(bool value) {
    ConfigManager& config = *LR2HackBox::Get().mConfig;
    mIsEnabled = value;
    ApplyP2ShutterFix(mIsEnabled);
    config.WriteValueAndSave("bBattleFixes", mIsEnabled);
}

bool BattleFixes::GetEnabled() {
    return mIsEnabled;
}