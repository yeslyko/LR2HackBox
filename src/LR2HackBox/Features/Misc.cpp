#define NOMINMAX

#include "Misc.hpp"

#include <iostream>
#include <print>
#include <ranges>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <Gdiplus.h>
#include "LR2HackBox/LR2HackBox.hpp"
#include "AnalogInput.hpp"
#include "Numbers.hpp"
#include "ScoreCannon.hpp"
#include "RivalLeaderboard.hpp"
#include "BattleFixes.hpp"

#include "ImGuiInjector/ImGuiInjector.hpp"
#include <safetyhook.hpp>
#include "minhook/include/MinHook.h"
#include "imgui/imgui.h"

#pragma comment(lib, "Gdiplus.lib")

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

static void SqliteGetColumn(std::string* output, void* pStmt, int columnIdx) {
	typedef LR2::CSTR(__cdecl* tSQL_GetColumn)(int i, void* pStmt);
	tSQL_GetColumn SQL_GetColumn = (tSQL_GetColumn)0x4444F0;
	output->assign(SQL_GetColumn(columnIdx, pStmt).body);
}

static void SqliteGetColumn(int* output, void* pStmt, int columnIdx) {
	typedef int(__cdecl* tsqlite3_column_int)(void* pStmt, int iCol);
	tsqlite3_column_int sqlite3_column_int = (tsqlite3_column_int)0x47AEC0;
	*output = sqlite3_column_int(pStmt, columnIdx);
}

template<typename T>
bool Misc::SqliteGetColumn(T* output, std::string querry, int columnIdx) {
	typedef int(__cdecl* tSQL_Prepare)(LR2::CSTR queryStr, void* sql, void** ppStmt);
	tSQL_Prepare SQL_Prepare = (tSQL_Prepare)0x4443f0;

	typedef int(__cdecl* tsqlite3_step)(void* stmt);
	tsqlite3_step sqlite3_step = (tsqlite3_step)0x48C480;

	typedef int(__cdecl* tsqlite3_finalize)(void* stmt);
	tsqlite3_finalize sqlite3_finalize = (tsqlite3_finalize)0x485FD0;

	void* sqlite = LR2HackBox::Get().GetSqlite();
	void* pStmt;
	bool success = false;

	SQL_Prepare(querry.c_str(), sqlite, &pStmt);
	if (sqlite3_step(pStmt) == 100) {
		::SqliteGetColumn(output, pStmt, columnIdx);
		success = true;
	}
	sqlite3_finalize(pStmt);

	return success;
}

static void StopKeysounds() {
	typedef int(__cdecl* tStopSound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound);
	tStopSound StopSound = (tStopSound)0x4B8140;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	for (int i = 0; i < 6480; i++) {
		StopSound(&game.audio, &game.gameplay.keysound[i]);
	}
}

void Misc::OnSetRetryFlag(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsRetryTweaks) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if ((game.KeyInput.p1_buttonInput[1] || game.KeyInput.p1_buttonInput[3] || game.KeyInput.p1_buttonInput[5] || game.KeyInput.p1_buttonInput[7] ||
		game.KeyInput.p2_buttonInput[1] || game.KeyInput.p2_buttonInput[3] || game.KeyInput.p2_buttonInput[5] || game.KeyInput.p2_buttonInput[7]) &&
		(game.KeyInput.p1_buttonInput[2] || game.KeyInput.p2_buttonInput[2])) 
	{
		game.gameplay.flag_retry = 0;
		game.gameplay.randomseed = 0;
		game.gameplay.bmsResourceLoaded = 1;

		game.config.play.gaugeOption[0] = misc.mOrigGaugeType;
	}
}

void Misc::QuickRestart(bool newRandom) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	game.procPhase = 0;
	game.procSelecter = 4;
	game.gameplay.flag_retry = newRandom ? 0 : 1;
	game.gameplay.randomseed = newRandom ? 0 : game.gameplay.randomseed;
	game.gameplay.bmsResourceLoaded = 1;

	// Reset fast/slow stats.
	game.net.rankingData.clearPlayers[2] = 0;
	game.net.rankingData.clearPlayers[3] = 0;
	game.net.rankingData.clearPlayers[4] = 0;

	// Reset gauge type for GAS.
	game.config.play.gaugeOption[0] = mOrigGaugeType;

	if (game.gameplay.courseType != -1 && game.gameplay.courseStageNow != 0) {
		game.gameplay.courseStageNow = 0;
		game.gameplay.bmsResourceLoaded = 0;
		game.gameplay.flag_retry = 0;
	}

	StopKeysounds();
}

static LR2::SOUNDDATA* metronomeMeasureFx;
static LR2::SOUNDDATA* metronomeBeatFx;

void Misc::OnPlayIBeforeInput(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());

	LR2::game& game = *LR2HackBox::Get().GetGame();

	if (!misc.mIsRetryTweaks) return;
	switch (misc.mBlockEscape) {
	case 1:
		if (game.KeyInput.inputID[0x01] == 3) misc.mBlockEscape = 0;
		break;
	case 2:
		if (game.KeyInput.mouse_buttonR == 3) misc.mBlockEscape = 0;
		break;
	case 3:
		if (game.KeyInput.p1_buttonInput[12] == 3 || game.KeyInput.p1_buttonInput[13] == 3) misc.mBlockEscape = 0;
		break;
	case 4:
		if (game.KeyInput.p2_buttonInput[12] == 3 || game.KeyInput.p2_buttonInput[13] == 3) misc.mBlockEscape = 0;
		break;
	default: break;
	}
}

void Misc::OnPlayIHandleEscape(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());

	LR2::game& game = *LR2HackBox::Get().GetGame();

	if (!misc.mIsRetryTweaks) return;
	if (!misc.mBlockEscape) return;

	auto ResetTimeLapse = (int(__cdecl*)(int timerID, LR2::Timer * T))0x4B67D0;
	ResetTimeLapse(2, &game.timer1);
	game.procPhase = 1;
	game.gameplay.flag_closingPhase = 0;
}

void Misc::OnPlayISetSelecter(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());

	LR2::game& game = *LR2HackBox::Get().GetGame();

	if (!(game.procPhase == 2 || game.procPhase == 3)) return;

	if (!misc.mIsRetryTweaks) return;

	if (game.gameplay.replay.status > 1) return;
	if (game.gameplay.player[0].totalnotes <= game.gameplay.player[0].note_current) return;
	
	auto wants_escape = [](LR2::game& game) {
		LR2::inputStructure& input = game.KeyInput;
		if (input.inputID[0x01] == 1 || input.inputID[0x01] == 2) return 1;
		if (game.config.play.disableleftclickexit == 0 && (input.mouse_buttonR == 1 || input.mouse_buttonR == 2)) return 2;
		if (input.p1_buttonInput[12] == 1 || input.p1_buttonInput[13] == 1) return 3;
		if (input.p2_buttonInput[12] == 1 || input.p2_buttonInput[13] == 1) return 4;
		return 0;
	};
	if (game.KeyInput.p1_buttonInput[12] == 1 || game.KeyInput.p2_buttonInput[12] == 1) {
		misc.mBlockEscape = wants_escape(game);
		misc.QuickRestart(true);
	}
	else if (game.KeyInput.p1_buttonInput[13] == 1 || game.KeyInput.p2_buttonInput[13] == 1) {
		misc.mBlockEscape = wants_escape(game);
		misc.QuickRestart(false);
	}
}

static void PreventKeysoundLeakOnPlayInit() {
	char* flag = (char*)0x4B0830;
	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(flag, 3, PAGE_EXECUTE_READWRITE, &oldProtection);
	memset(flag, 0x90, 3); // for (int i = 0; i < 1296; i++) gp->keysound[i].load = 0; @LR2input.cpp1404
	DWORD discard = 0;
	VirtualProtect(flag, 3, oldProtection, &discard);
}

void Misc::OnInit(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
}

void Misc::OnInitPlay(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	misc.OnInit(regs);
}

void Misc::OnInitRetry(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	misc.OnInit(regs);
}

void Misc::OnRandomMixInput(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.sSelect.bmsList[game.sSelect.cur_song].folderType != 9 && std::string(game.sSelect.bmsList[game.sSelect.cur_song].filepath.body) != "customselect") return;
	regs.ecx = 0;

	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsCustomSelect) return;

	misc.mOnCustomSelEntry = true;

	game.procSelecter = 3;
}

static void UpdateSongdataStrings() {
	LR2::game* g = LR2HackBox::Get().GetGame();

	typedef int(__cdecl* tSetObjectString)(uint32_t num, LR2::CSTR string, LR2::CSTR* objectList);
	tSetObjectString SetObjectString = (tSetObjectString)0x4B6C40;

	typedef int(__cdecl* tSetObjectStringInt)(int at, int val, LR2::CSTR* arr);
	tSetObjectStringInt SetObjectStringInt = (tSetObjectStringInt)0x4B6D00;

	SetObjectString(10, g->sSelect.bmsList[g->sSelect.cur_song].title, g->txtStruct.objectStr);
	SetObjectString(11, g->sSelect.bmsList[g->sSelect.cur_song].subtitle, g->txtStruct.objectStr);
	SetObjectString(12, g->sSelect.bmsList[g->sSelect.cur_song].fulltitle, g->txtStruct.objectStr);
	SetObjectString(13, g->sSelect.bmsList[g->sSelect.cur_song].genre, g->txtStruct.objectStr);
	SetObjectString(14, g->sSelect.bmsList[g->sSelect.cur_song].artist, g->txtStruct.objectStr);
	SetObjectString(15, g->sSelect.bmsList[g->sSelect.cur_song].subartist, g->txtStruct.objectStr);
	SetObjectString(16, g->sSelect.bmsList[g->sSelect.cur_song].tag, g->txtStruct.objectStr);
	SetObjectStringInt(17, g->sSelect.bmsList[g->sSelect.cur_song].level, g->txtStruct.objectStr);
	SetObjectStringInt(18, g->sSelect.bmsList[g->sSelect.cur_song].difficulty, g->txtStruct.objectStr);
	SetObjectStringInt(19, g->sSelect.bmsList[g->sSelect.cur_song].exlevel, g->txtStruct.objectStr);

	SetObjectString(20, g->sSelect.bmsList[g->sSelect.cur_song].title, g->txtStruct.objectStr);
	SetObjectString(21, g->sSelect.bmsList[g->sSelect.cur_song].subtitle, g->txtStruct.objectStr);
	SetObjectString(22, g->sSelect.bmsList[g->sSelect.cur_song].fulltitle, g->txtStruct.objectStr);
	SetObjectString(23, g->sSelect.bmsList[g->sSelect.cur_song].genre, g->txtStruct.objectStr);
	SetObjectString(24, g->sSelect.bmsList[g->sSelect.cur_song].artist, g->txtStruct.objectStr);
	SetObjectString(25, g->sSelect.bmsList[g->sSelect.cur_song].subartist, g->txtStruct.objectStr);
	SetObjectString(26, g->sSelect.bmsList[g->sSelect.cur_song].tag, g->txtStruct.objectStr);
	SetObjectStringInt(27, g->sSelect.bmsList[g->sSelect.cur_song].level, g->txtStruct.objectStr);
	SetObjectStringInt(28, g->sSelect.bmsList[g->sSelect.cur_song].difficulty, g->txtStruct.objectStr);
	SetObjectStringInt(29, g->sSelect.bmsList[g->sSelect.cur_song].exlevel, g->txtStruct.objectStr);
}

static void FillBMSMETA(LR2::BMSMETA& meta, const LR2::SONGDATA& song) {
	meta.artist.assign(song.artist.body);
	meta.filepath.assign(song.filepath.body);
	meta.subartist.assign(song.subartist.body);
	meta.title.assign(song.title.body);
	meta.subtitle.assign(song.subtitle.body);

	meta.selLevel = song.level;
	meta.exlevel = song.exlevel;
	meta.difficulty = song.difficulty;
	meta.keymode = song.keymode;
}

void Misc::CustomSelect() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	game.gameplay.replay.status = 0;
	game.gameplay.isAutoplay = 0;

	std::string querry;
	if (std::string_view(game.sSelect.bmsList[game.sSelect.cur_song].tag.body).size() <= sizeof("NOTAFOLDER")) {
		querry = std::format("{} ORDER BY {} LIMIT 1", game.sSelect.stack_query[game.sSelect.cur].body, game.sSelect.bmsList[game.sSelect.cur_song].hash.body);
	}
	else {
		querry = std::format("{} AND ({}) ORDER BY {} LIMIT 1", game.sSelect.stack_query[game.sSelect.cur].body, std::string_view(game.sSelect.bmsList[game.sSelect.cur_song].tag.body).substr(sizeof("NOTAFOLDER")), game.sSelect.bmsList[game.sSelect.cur_song].hash.body);
	}
	std::string songhash;

	if (SqliteGetColumn(&songhash, querry, 0)) {
		for (int i = 0; i < game.sSelect.bmsListCount; i++) {
			if (songhash == game.sSelect.bmsList[i].hash.body) {
				game.sSelect.cur_song = i;
				break;
			}
		}
	}

	FillBMSMETA(game.sSelect.metaSelected, game.sSelect.bmsList[game.sSelect.cur_song]);
	UpdateSongdataStrings();

	mOnCustomSelEntry = false;
}

void Misc::OnDecideInit() {
	if (mIsCustomSelect && mOnCustomSelEntry) CustomSelect();
}

void Misc::OnPlayInit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	Numbers& numbers = *(Numbers*)(LR2HackBox::Get().mNumbers.get());
	BattleFixes& battleFixes = *(BattleFixes*)(LR2HackBox::Get().mBattleFixes.get());

	numbers.SceneInit();
	battleFixes.SceneInit();

	mOrigGaugeType = game.config.play.gaugeOption[0];
	mCurrentDrawingLNObj = nullptr;

	if (game.config.play.autojudge == 3) game.config.play.judgetiming = mAutoadjustResetLastVal;

	mMetronomeLastPlayedBeat = 0;
	mMetronomePrevMeasureIdx = -1;
	if (mIsMetronome) {
		typedef int(__cdecl* tLoadSound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound, LR2::CSTR filepath, int loop, int disableDSP, int previewFlag);
		tLoadSound LoadSound = (tLoadSound)0x4B8BB0;

		LoadSound(&game.audio, metronomeMeasureFx, LR2::CSTR("LR2files\\Sound\\LR2HackBox\\metronome-measure.wav"), 0, game.config.sound.disabledsp, 0);
		LoadSound(&game.audio, metronomeBeatFx, LR2::CSTR("LR2files\\Sound\\LR2HackBox\\metronome-beat.wav"), 0, game.config.sound.disabledsp, 0);
	}
}

void Misc::OnResultInit() {
	ScoreCannon& spammer = *(ScoreCannon*)LR2HackBox::Get().mScoreCannon.get();
	spammer.mAlreadySent = false;
}

void Misc::OnCourseResultInit() {
	ScoreCannon& spammer = *(ScoreCannon*)LR2HackBox::Get().mScoreCannon.get();
	spammer.mAlreadySent = false;
}

void Misc::OnSceneInitSwitch(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	Numbers& numbers = *(Numbers*)(LR2HackBox::Get().mNumbers.get());

	switch (regs.eax) {
	case 3:
		misc.OnDecideInit();
		break;
	case 4:
		misc.OnPlayInit();
		break;
	case 5:
		misc.OnResultInit();
		break;
	case 13:
		misc.OnCourseResultInit();
		break;
	}
}

void Misc::OnSceneProcSwitch(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());

	switch (regs.eax) {
	}
}

void Misc::OnSelectExit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	mAutoadjustResetLastVal = game.config.play.judgetiming;
}

void Misc::OnPlayExit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.autojudge == 3) game.config.play.judgetiming = mAutoadjustResetLastVal;

	typedef int(__cdecl* tReleaseSound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound);
	tReleaseSound ReleaseSound = (tReleaseSound)0x4B8040;

	ReleaseSound(&game.audio, metronomeMeasureFx);
	ReleaseSound(&game.audio, metronomeBeatFx);
}

void Misc::OnSceneExitSwitch(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());

	switch (regs.edi) {
	case 2:
		misc.OnSelectExit();
		break;
	case 4:
		misc.OnPlayExit();
		break;
	}
}

static void AddCustomSelectBar(const char* title, const char* condition = "", const char* order = "random()") {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	LR2::SONGDATA bar;
	typedef void(__cdecl* tSongDataInit)(LR2::SONGDATA* song);
	tSongDataInit SongDataInit = (tSongDataInit)0x444730;
	SongDataInit(&bar);
	bar.folderType = 9;
	bar.title.assign(title);
	bar.fulltitle.assign(title);
	bar.filepath.assign("customselect");
	bar.tag.assign(("NOTAFOLDER " + std::string(condition)).c_str());
	std::string_view(order).empty() ? bar.hash.assign("random()") : bar.hash.assign(order);
	typedef LR2::SONGDATA*(__thiscall* tSongDataAssign)(LR2::SONGDATA* pThis, LR2::SONGDATA* other);
	tSongDataAssign SongDataAssign = (tSongDataAssign)0x404FE0;
	SongDataAssign(&game.sSelect.bmsList[game.sSelect.bmsListCount], &bar);
	game.sSelect.bmsListCount++;
}

void Misc::OnOpenFolderPlaySound(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsCustomSelect) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.sSelect.stack_isFolder[game.sSelect.cur] == 0) {
		for (auto& entry : misc.mCustomSelectEntries) {
			AddCustomSelectBar(entry.title.c_str(), entry.condition.c_str(), entry.order.c_str());
		}
	}
}


static std::unordered_map<double, int> bpmRefcount;
void Misc::OnAddToAvgBpmSum(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsMainBPM) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.hsfix != 3) return;

	double bpm = 0;
	__asm {
		fld ST(1)
		fstp bpm
	}

	if (!bpmRefcount.contains(bpm)) {
		bpmRefcount[bpm] = 1;
	}
	else {
		bpmRefcount[bpm] = bpmRefcount[bpm] + 1;
	}
}

void Misc::OnCalcAvgSpeedmult(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsMainBPM) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();

	// Calculate mainBPM with longest duration bpm.
	/*std::unordered_map<double, double> bpmDuration;

	double lastNoteTime = 0;
	for (int i = 0; i < game.gameplay.bmsobj.count; i++) {
		if (10 <= game.gameplay.bmsobj.notes[i].op && game.gameplay.bmsobj.notes[i].op < 29)
			lastNoteTime = game.gameplay.bmsobj.notes[i].realTiming;
	}
	for (int i = 0; i < game.gameplay.bpmt_count - 1; i++) {
		if (game.gameplay.bpmt_data[i].realtime > lastNoteTime) break;
		double bpm = game.gameplay.bpmt_data[i].BPM;
		double timePointDuration = game.gameplay.bpmt_data[i + 1].realtime - game.gameplay.bpmt_data[i].realtime;
		if (!bpmDuration.contains(bpm)) {
			bpmDuration[bpm] = timePointDuration;
		}
		else {
			bpmDuration[bpm] = bpmDuration[bpm] + timePointDuration;
		}
	}

	double mainBpm = 0.f;
	double highestDuration = 0;
	for (auto& [bpm, duration] : bpmDuration) {
		if (duration > highestDuration) {
			highestDuration = duration;
			mainBpm = bpm;
		}
	}*/

	// Calculate mainBPM with bpm most notes use.
	double mainBpm = 0;
	int highestRefcount = 0;
	for (auto& [bpm, refcount] : bpmRefcount) {
		if (refcount > highestRefcount) {
			highestRefcount = refcount;
			mainBpm = bpm;
		}
	}
	bpmRefcount.clear();

	__asm {
		fld mainBpm
		fstp ST(1) // gameplay.speedmultiplier = 150.f / ST(0)
	}

	return;
}

static double measureSize = 0;
static int beatsPerMeasure = 0;
void Misc::OnDrawNotesGetSongtimer(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsMetronome) return;

	double songtimer = 0;
	__asm {
		fst songtimer
	}

	LR2::game& game = *LR2HackBox::Get().GetGame();
	int currentMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, misc.mMetronomePrevMeasureIdx + 1);
	int nextMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, currentMeasureIdx + 1);
	if (game.gameplay.bmsobj_line.notes[nextMeasureIdx].bmsTiming <= songtimer) {
		misc.mMetronomePrevMeasureIdx = currentMeasureIdx;
		currentMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, misc.mMetronomePrevMeasureIdx + 1);
		nextMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, currentMeasureIdx + 1);
	}

	const LR2::NoteStruct& currentMeasure = game.gameplay.bmsobj_line.notes[currentMeasureIdx];
	int currentBeat = (songtimer - currentMeasure.bmsTiming) / 480.f;

	typedef int(__cdecl* tGamePlaySound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound, LR2::FMOD_CHANNELGROUP* channelgroup, int stage);
	tGamePlaySound GamePlaySound = (tGamePlaySound)0x4B8F20;
	if (currentBeat != misc.mMetronomeLastPlayedBeat) {
		misc.mMetronomeLastPlayedBeat = currentBeat;
		if (currentBeat == 0) {
			LR2::SOUNDDATA* sfx = metronomeBeatFx->load == 1 ? metronomeMeasureFx : &game.audio.sysSound.folder_open;
			GamePlaySound(&game.audio, sfx, game.audio.chnBgm, -1);
		}
		else {
			LR2::SOUNDDATA* sfx = metronomeBeatFx->load == 1 ? metronomeBeatFx : &game.audio.sysSound.folder_close;
			GamePlaySound(&game.audio, sfx, game.audio.chnBgm, -1);
		}
	}
}

static std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

typedef int(__cdecl* tSaveDrawScreenToPNG)(int x1, int y1, int x2, int y2, const char* FileName, int CompressionLevel);
tSaveDrawScreenToPNG SaveDrawScreenToPNG = (tSaveDrawScreenToPNG)0x510060;
int Misc::OnSaveDrawScreenToPNG(int x1, int y1, int x2, int y2, const char* FileName, int CompressionLevel) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	LR2::game& game = *LR2HackBox::Get().GetGame();

	std::string directory = "screenshots\\";
	std::string path = misc.mIsRerouteScreenshots ? directory + FileName : FileName;
	if (misc.mIsRerouteScreenshots && !std::filesystem::directory_entry(directory).exists())
		std::filesystem::create_directories(directory);
	int result = SaveDrawScreenToPNG(x1, y1, x2, y2, path.c_str(), CompressionLevel);

	if (misc.mIsScreenshotsCopybuffer) {
		ULONG_PTR gdiplusToken = NULL;
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) return result;

		Gdiplus::Status status;
		Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(s2ws(path).c_str());
		if (!bitmap) return result;

		HBITMAP hbitmap;
		status = bitmap->GetHBITMAP(0, &hbitmap);
		if (status != Gdiplus::Ok) return result;

		BITMAP bm;
		GetObject(hbitmap, sizeof bm, &bm);
		DIBSECTION ds;
		if (sizeof ds == GetObject(hbitmap, sizeof ds, &ds))
		{
			HDC hdc = GetDC(NULL);
			HBITMAP hbitmap_ddb = CreateDIBitmap(hdc, &ds.dsBmih, CBM_INIT,
				ds.dsBm.bmBits, (BITMAPINFO*)&ds.dsBmih, DIB_RGB_COLORS);
			ReleaseDC(NULL, hdc);
			if (OpenClipboard(ImGuiInjector::Get().GetWindowHandle()))
			{
				EmptyClipboard();
				SetClipboardData(CF_BITMAP, hbitmap_ddb);
				CloseClipboard();
			}
			DeleteObject(hbitmap_ddb);
		}
		DeleteObject(hbitmap);

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

	if (game.procSelecter == 5 || game.procSelecter == 13) {
		ScoreCannon& spammer = *(ScoreCannon*)LR2HackBox::Get().mScoreCannon.get();
		if (spammer.mIsEnabled && !spammer.mAlreadySent) {
			ScoreCannon::Score score(&game);
			spammer.PostScore(score, path);
		}
	}

	return result;
}

void Misc::SetMetronome(bool value) {
	mIsMetronome = value;
}

void Misc::MirrorGearshift(bool mirror) {
	const unsigned char* p1_buttonInputOffsets = (unsigned char*)0x1FD8C;

	uintptr_t* gearshiftDownButton1 = (uintptr_t*)0x4277AE;
	uintptr_t* gearshiftDownButton2 = (uintptr_t*)0x4277B7;
	uintptr_t* gearshiftDownButton3 = (uintptr_t*)0x4277C0;

	uintptr_t* gearshiftUpButton1 = (uintptr_t*)0x4277E5;
	uintptr_t* gearshiftUpButton2 = (uintptr_t*)0x4277EE;

	uintptr_t* lanecoverUpButton = (uintptr_t*)0x4278BC;
	uintptr_t* lanecoverDownButton = (uintptr_t*)0x4278DB;

	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(gearshiftDownButton1, (uintptr_t)lanecoverDownButton - (uintptr_t)gearshiftDownButton1 + 4, PAGE_EXECUTE_READWRITE, &oldProtection);

	if (mirror) {
		*gearshiftDownButton1 = (uintptr_t)&p1_buttonInputOffsets[3];
		*gearshiftDownButton2 = (uintptr_t)&p1_buttonInputOffsets[5];
		*gearshiftDownButton3 = (uintptr_t)&p1_buttonInputOffsets[7];

		*gearshiftUpButton1 = (uintptr_t)&p1_buttonInputOffsets[4];
		*gearshiftUpButton2 = (uintptr_t)&p1_buttonInputOffsets[6];

		*lanecoverUpButton = (uintptr_t)&p1_buttonInputOffsets[2];
		*lanecoverDownButton = (uintptr_t)&p1_buttonInputOffsets[1];
	}
	else {
		*gearshiftDownButton1 = (uintptr_t)&p1_buttonInputOffsets[1];
		*gearshiftDownButton2 = (uintptr_t)&p1_buttonInputOffsets[3];
		*gearshiftDownButton3 = (uintptr_t)&p1_buttonInputOffsets[5];

		*gearshiftUpButton1 = (uintptr_t)&p1_buttonInputOffsets[2];
		*gearshiftUpButton2 = (uintptr_t)&p1_buttonInputOffsets[4];

		*lanecoverUpButton = (uintptr_t)&p1_buttonInputOffsets[6];
		*lanecoverDownButton = (uintptr_t)&p1_buttonInputOffsets[7];
	}

	DWORD discard = 0;
	hResult = VirtualProtect(gearshiftDownButton1, (uintptr_t)lanecoverDownButton - (uintptr_t)gearshiftDownButton1 + 4, oldProtection, &discard);
}

void Misc::OnBeforeAddDrawingBuffer_LN(SafetyHookContext& regs) {
	uintptr_t** offset1 = (uintptr_t**)(regs.esp + 0x68);
	uintptr_t* offset2 = (uintptr_t*)(regs.esp + 0xA0);
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	misc.mCurrentDrawingLNObj = (void*)(**offset1 + *offset2);
}

typedef int (__cdecl* tAddDrawingBuffer_LN)(LR2::DrawingBuf* drb, LR2::SRCstruct* srcLs, LR2::SRCstruct* srcLe, LR2::SRCstruct* srcLb, LR2::DSTstruct* dst, LR2::Timer* T, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY);
tAddDrawingBuffer_LN AddDrawingBuffer_LN = (tAddDrawingBuffer_LN)0x49D240;
int Misc::OnAddDrawingBuffer_LN_Fixed(void* drbIn, void* srcLsIn, void* srcLeIn, void* srcLbIn, void* dstIn, void* TIn, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY, void* lnObjIn) {
	LR2::DrawingBuf* drb = (LR2::DrawingBuf*)drbIn;
	LR2::SRCstruct* srcLs = (LR2::SRCstruct*)srcLsIn;
	LR2::SRCstruct* srcLe = (LR2::SRCstruct*)srcLeIn;
	LR2::SRCstruct* srcLb = (LR2::SRCstruct*)srcLbIn;
	LR2::DSTstruct* dst = (LR2::DSTstruct*)dstIn;
	LR2::Timer* T = (LR2::Timer*)TIn;
	LR2::NoteStruct* lnObj = (LR2::NoteStruct*)lnObjIn;
	LR2::game& game = *LR2HackBox::Get().GetGame();

	typedef LR2::DSTdraw(__cdecl* tSetDSTdrawByTime)(LR2::DSTstruct dst, double time);
	tSetDSTdrawByTime SetDSTdrawByTime = (tSetDSTdrawByTime)0x49A840;
	typedef double(__cdecl* tGetTimeLapse)(int timerID, LR2::Timer* T);
	tGetTimeLapse GetTimeLapse = (tGetTimeLapse)0x4B6B10;
	typedef int(__cdecl* tGetSRCcycleNow)(LR2::SRCstruct src, double time);
	tGetSRCcycleNow GetSRCcycleNow = (tGetSRCcycleNow)0x49ABF0;
	typedef int(__cdecl* tAddDrawingBuffer)(LR2::DrawingBuf* drb, int grHandle, LR2::DSTdraw* dstd);
	tAddDrawingBuffer AddDrawingBuffer = (tAddDrawingBuffer)0x49CEB0;

	LR2::DSTdraw tDstd;
	int grh;

	float absoluteScrollSpeed = (game.config.play.basespeed / 100.0) * game.config.play.hiSpeed[0] * game.gameplay.speedmultiplier * game.gameplay.BPM / 150.f;
	float blinkAdjust = absoluteScrollSpeed * 0.00004;

	if (dst->dstCount <= 0 || dst->dataSize <= 0 || srcLs->graphcount <= 0 || srcLe->graphcount <= 0 || srcLb->graphcount <= 0) return 0;

	//body
	tDstd = SetDSTdrawByTime(*dst, GetTimeLapse(dst->timer, T));
	tDstd.w += sizeX;
	tDstd.h += sizeY;
	tDstd.x -= sizeX * 0.5;
	tDstd.y -= sizeY * 0.5;
	if (srcLb->timer == dst->timer) {
		if (lnObj->active > 0) {
			grh = srcLb->grHandles[GetSRCcycleNow(*srcLb, GetTimeLapse(srcLb->timer, T) - dst->draw->time)];
		}
		else {
			grh = srcLb->grHandles[0];
		}
	}
	else {
		if (lnObj->active > 0) {
			grh = srcLb->grHandles[GetSRCcycleNow(*srcLb, GetTimeLapse(srcLb->timer, T))];
		}
		else {
			grh = srcLb->grHandles[0];
		}
	}
	tDstd.x += shiftX;
	tDstd.subHandle = dst->n;
	tDstd.blend = 1;
	tDstd.a = alpha;
	tDstd.y += longY + tDstd.h - blinkAdjust;
	tDstd.h = std::max(shiftY - longY - tDstd.h + blinkAdjust, 0.f);
	if (tDstd.time != -1 && grh != -1) AddDrawingBuffer(drb, grh, &tDstd);

	//end
	tDstd = SetDSTdrawByTime(*dst, GetTimeLapse(dst->timer, T));
	tDstd.w += sizeX;
	tDstd.h += sizeY;
	tDstd.x -= sizeX * 0.5;
	tDstd.y -= sizeY * 0.5;
	tDstd.sortID += 1;
	if (srcLe->timer == dst->timer) {
		if (lnObj->active > 0) {
			grh = srcLe->grHandles[GetSRCcycleNow(*srcLe, GetTimeLapse(srcLe->timer, T) - dst->draw->time)];
		}
		else {
			grh = srcLe->grHandles[0];
		}
	}
	else {
		if (lnObj->active > 0) {
			grh = srcLe->grHandles[GetSRCcycleNow(*srcLe, GetTimeLapse(srcLe->timer, T))];
		}
		else {
			grh = srcLe->grHandles[0];
		}
	}
	tDstd.x += shiftX;
	tDstd.subHandle = dst->n;
	tDstd.blend = 1;
	tDstd.a = alpha;
	tDstd.y += longY;
	float tailBottomY = tDstd.y;
	if (tDstd.time != -1 && grh != -1) AddDrawingBuffer(drb, grh, &tDstd);

	//start
	tDstd = SetDSTdrawByTime(*dst, GetTimeLapse(dst->timer, T));
	tDstd.w += sizeX;
	tDstd.h += sizeY;
	tDstd.x -= sizeX * 0.5;
	tDstd.y -= sizeY * 0.5;
	tDstd.sortID += 2;
	if (srcLs->timer == dst->timer) {
		if (lnObj->active > 0) {
			grh = srcLs->grHandles[GetSRCcycleNow(*srcLs, GetTimeLapse(srcLs->timer, T) - dst->draw->time)];
		}
		else {
			grh = srcLs->grHandles[0];
		}
	}
	else {
		if (lnObj->active > 0) {
			grh = srcLs->grHandles[GetSRCcycleNow(*srcLs, GetTimeLapse(srcLs->timer, T))];
		}
		else {
			grh = srcLs->grHandles[0];
		}
	}
	tDstd.x += shiftX;
	tDstd.subHandle = dst->n;
	tDstd.blend = 1;
	tDstd.a = alpha;
	tDstd.y += shiftY;
	if (alpha != 255) {
		float shrinkFromTail = std::min(tDstd.y - tDstd.h - tailBottomY, 0.f);
		if (shrinkFromTail < 0.f) shrinkFromTail = std::max(shrinkFromTail, -tDstd.h);
		tDstd.h += shrinkFromTail;
		tDstd.y -= shrinkFromTail;
	}
	if (tDstd.time != -1 && grh != -1) AddDrawingBuffer(drb, grh, &tDstd);

	return 1;
}

int Misc::OnAddDrawingBuffer_LN(void* drb, void* srcLs, void* srcLe, void* srcLb, void* dst, void* T, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (misc.mIsLNAnimFix && misc.mCurrentDrawingLNObj) {
		void* lnObj = misc.mCurrentDrawingLNObj;
		misc.mCurrentDrawingLNObj = nullptr;
		return misc.OnAddDrawingBuffer_LN_Fixed((LR2::DrawingBuf*)drb, (LR2::SRCstruct*)srcLs, (LR2::SRCstruct*)srcLe, (LR2::SRCstruct*)srcLb, (LR2::DSTstruct*)dst, (LR2::Timer*)T, shiftX, shiftY, longY, alpha, sizeX, sizeY, lnObj);
	}
	return AddDrawingBuffer_LN((LR2::DrawingBuf*)drb, (LR2::SRCstruct*)srcLs, (LR2::SRCstruct*)srcLe, (LR2::SRCstruct*)srcLb, (LR2::DSTstruct*)dst, (LR2::Timer*)T, shiftX, shiftY, longY, alpha, sizeX, sizeY);
}

typedef int(__cdecl* tAddDrawingBuffer_PlayArea)(LR2::DrawingBuf* drb, LR2::SRCstruct* src, LR2::DSTstruct* dst, LR2::Timer* T, float shiftX, float shiftY, int alpha, float sizeX, float sizeY, char flag);
tAddDrawingBuffer_PlayArea AddDrawingBuffer_PlayArea = (tAddDrawingBuffer_PlayArea)0x49D630;
int Misc::OnAddDrawingBuffer_PlayArea(void* drb, void* src, void* dst, void* T, float shiftX, float shiftY, int alpha, float sizeX, float sizeY, char flag) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	return AddDrawingBuffer_PlayArea((LR2::DrawingBuf*)drb, (LR2::SRCstruct*)src, (LR2::DSTstruct*)dst, (LR2::Timer*)T, shiftX, shiftY, alpha, sizeX, sizeY, flag);
}

void Misc::OnAutoadjustDec(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsAutoadjustClamp) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.gameplay.autojudge_midsum == 0) return;
	if (game.config.play.judgetiming <= misc.mAutoadjustClampMin) game.config.play.judgetiming++;
}

void Misc::OnAutoadjustInc(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsAutoadjustClamp) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.judgetiming >= misc.mAutoadjustClampMax) game.config.play.judgetiming--;
}

void Misc::SetAutoadjustReset(bool enable) {
	char* autoadjustModeUpperLimit = (char*)0x425007;

	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(autoadjustModeUpperLimit, 1, PAGE_EXECUTE_READWRITE, &oldProtection);

	*autoadjustModeUpperLimit = enable ? 3 : 2;

	DWORD discard = 0;
	hResult = VirtualProtect(autoadjustModeUpperLimit, 1, oldProtection, &discard);
}

void Misc::SetSkipResultWaitIR(bool enable) {
	unsigned short* netHandleJne = (unsigned short*)0x409708;

	constexpr unsigned short def = 0x4D75;
	constexpr unsigned short mod = 0x9090;

	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(netHandleJne, sizeof(unsigned short), PAGE_EXECUTE_READWRITE, &oldProtection);

	*netHandleJne = enable ? mod : def;

	DWORD discard = 0;
	hResult = VirtualProtect(netHandleJne, sizeof(unsigned short), oldProtection, &discard);
}

void Misc::SetSkipResultSub(bool enable) {
	unsigned short* isNoSaveJne = (unsigned short*)0x409719;

	constexpr unsigned short def = 0x850F;
	constexpr unsigned short mod = 0xE990;

	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(isNoSaveJne, sizeof(unsigned short), PAGE_EXECUTE_READWRITE, &oldProtection);

	*isNoSaveJne = enable ? mod : def;

	DWORD discard = 0;
	hResult = VirtualProtect(isNoSaveJne, sizeof(unsigned short), oldProtection, &discard);
}

typedef int(__cdecl* tSetObjectString)(unsigned int num, LR2::CSTR string, LR2::CSTR* objectList);
tSetObjectString SetObjectString = (tSetObjectString)0x4B6C40;
int Misc::OnSetObjectString(unsigned int num, void* string, void** objectList) {
	if (!LR2HackBox::Get().GetGame()) return SetObjectString(num, (LR2::CSTR&)string, (LR2::CSTR*)objectList);
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (num == 80 && game.config.play.autojudge == 3) {
		return SetObjectString(80, "RESET", game.txtStruct.objectStr);
	}
	return SetObjectString(num, (LR2::CSTR&)string, (LR2::CSTR*)objectList);
}
#include <Helpers/Helpers.hpp>
safetyhook::InlineHook oSetFirstSkins;
int Misc::OnSetFirstSkins(void* g) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsCourseResultFix) return oSetFirstSkins.ccall<int>(g);

	LR2::game& game = *(LR2::game*)g;
	void* hXml = malloc(0x48);
	typedef void*(__thiscall* tTiXmlDocumentCtor)(void* pThis, const char* filename);
	tTiXmlDocumentCtor TiXmlDocumentCtor = (tTiXmlDocumentCtor)0x4C2FE0;
	const char* configName = (const char*)0x7381A8;
	void* result1 = TiXmlDocumentCtor(hXml, configName);
	typedef bool(__thiscall* tTiXmlDocumentLoadFile)(void* pThis, int encoding);
	tTiXmlDocumentLoadFile TiXmlDocumentLoadFile = (tTiXmlDocumentLoadFile)0x4C4150;
	bool result2 = TiXmlDocumentLoadFile(hXml, 0);
	typedef int(__cdecl* tReadXml_Str)(const char* level1, const char* level2, const char* level3, const LR2::CSTR initvalue, LR2::CSTR* oBuf, void* xmlData);
	tReadXml_Str ReadXml_Str = (tReadXml_Str)0x43C0E0;
	ReadXml_Str("config", "skin", "courseresult", "", &game.config.skin.skinFilePath[15], hXml);
	free(hXml);

	return oSetFirstSkins.ccall<int>(g);
}

void Misc::OnWriteConfigXml(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsCourseResultFix) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();

	FILE* pFile = (FILE*)regs.esi;
	typedef void(__cdecl* tWriteXML_Tab2Str)(FILE* hFile, const char* tag, LR2::CSTR str);
	tWriteXML_Tab2Str WriteXML_Tab2Str = (tWriteXML_Tab2Str)0x43CF50;
	WriteXML_Tab2Str(pFile, "courseresult", game.config.skin.skinFilePath[15]);
}

void Misc::CustomSelectAddDefault() {
	std::array<CustomSelectEntry, 5> defaults{
		CustomSelectEntry("RANDOM SELECT"),
		CustomSelectEntry("RANDOM SELECT UNPLAYED", "clear IS NULL", "random()"),
		CustomSelectEntry("RANDOM SELECT FAILED", "clear = 1", "random()"),
		CustomSelectEntry("RANDOM SELECT <HC", "clear < 4 OR clear IS NULL", "random()"),
		CustomSelectEntry("RANDOM SELECT <AAA", "CAST((perfect * 2 + great) AS float) / (totalnotes * 2) * 100 < 88.88 OR perfect IS NULL", "random()")
	};
	mCustomSelectEntries.insert(mCustomSelectEntries.end(), defaults.begin(), defaults.end());
}

void Misc::CustomSelectLoadConfig() {
	ConfigManager& config = *LR2HackBox::Get().mConfig.get();
	if (!config.ValueExists("sCustomSelectEntries")) {
		CustomSelectAddDefault();
	}
	std::vector<std::string> configEntries;
	config.ReadArray("sCustomSelectEntries", configEntries);
	for (auto& configEntry : configEntries) {
		std::stringstream values(configEntry);
		std::string value;
		CustomSelectEntry entry;
		int valueIdx = 0;
		while (std::getline(values, value, ';')) {
			switch (valueIdx) {
			case 0: entry.title = value; break;
			case 1: entry.condition = value; break;
			case 2: entry.order = value; break;
			}
			valueIdx++;
		}
		mCustomSelectEntries.push_back(entry);
	}
	mSelectedCustomSelectEntry = mCustomSelectEntries.end();
}

void Misc::CustomSelectSaveConfig() {
	ConfigManager& config = *LR2HackBox::Get().mConfig.get();
	std::vector<std::string> configEntries;
	for (auto& entry : mCustomSelectEntries) {
		std::stringstream entryString;
		entryString << entry.title << ";" << entry.condition << ";" << entry.order;
		configEntries.push_back(entryString.str());
	}
	config.WriteArrayAndSave("sCustomSelectEntries", configEntries);
}

void Misc::OnIrSendSuccess(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsResultQuickIR) return;
	int* resetFlag = (int*)regs.esp;
	*resetFlag = 0;
	LR2::NETWORK& ir = LR2HackBox::Get().GetGame()->net;
	std::vector<std::string_view> values;
	for (const auto value : std::ranges::split_view(std::string_view(ir.httpResult.body), ',')) {
		values.emplace_back(value.begin(), value.end());
	}
	if (values.size() < 2) return;
	{
		int rank{};
		std::string_view sRank = values[0];
		auto [ptr, ec] = std::from_chars(sRank.data(), sRank.data() + sRank.size(), rank);
		if (ec == std::errc()) ir.rankingData.myRanking = rank;
	}
	{
		int rankCount{};
		std::string_view sRankCount = values[1];
		auto [ptr, ec] = std::from_chars(sRankCount.data(), sRankCount.data() + sRankCount.size(), rankCount);
		if (ec == std::errc()) ir.rankingData.rankingCount = rankCount;
	}
}

void Misc::OnGhostDecodeAdd(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc.get());
	if (!misc.mIsGhostFix) return;
	
	auto& rep = regs.esi;
	if (rep == 0) rep = 1;
}

bool Misc::EarlyInit(uintptr_t moduleBase) {
	Misc::mModuleBase = moduleBase;

	LoadConfig();
	SetHooks();

	return true;
}

bool Misc::Init(uintptr_t moduleBase) {
	Misc::mModuleBase = moduleBase;

	PreventKeysoundLeakOnPlayInit();

	metronomeBeatFx = new LR2::SOUNDDATA();
	metronomeMeasureFx = new LR2::SOUNDDATA();

	return true;
}

bool Misc::Deinit() {
	delete(metronomeBeatFx);
	delete(metronomeMeasureFx);
	return true;
}

void Misc::LoadConfig() {
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	mIsRetryTweaks = config.ReadValue("bRetryTweaks", mIsRetryTweaks);
	mIsCustomSelect = config.ReadValue("bCustomSelect", mIsCustomSelect);
	mIsMainBPM = config.ReadValue("bMainBPM", mIsMainBPM);
	mIsRerouteScreenshots = config.ReadValue("bRerouteScreenshots", mIsRerouteScreenshots);
	mIsScreenshotsCopybuffer = config.ReadValue("bScreenshotsCopybuffer", mIsScreenshotsCopybuffer);
	mIsMirrorGearshift = config.ReadValue("bMirrorGearshift", mIsMirrorGearshift);
	mIsAnalogInput = config.ReadValue("bAnalogInput", mIsAnalogInput);
	mIsLNAnimFix = config.ReadValue("bLNAnimFix", mIsLNAnimFix);
	mIsAutoadjustClamp = config.ReadValue("bAutoadjustClamp", mIsAutoadjustClamp);
	mIsAutoadjustReset = config.ReadValue("bAutoadjustReset", mIsAutoadjustReset);
	mIsCourseResultFix = config.ReadValue("bCourseResultFix", mIsCourseResultFix);
	mIsSkipResultWaitIR = config.ReadValue("bSkipResultWaitIR", mIsSkipResultWaitIR);
	mIsSkipResultSub = config.ReadValue("bSkipResultSub", mIsSkipResultSub);
	mIsResultQuickIR = config.ReadValue("bResultQuickIR", mIsResultQuickIR);
	mIsGhostFix = config.ReadValue("bGhostFix", mIsGhostFix);
	mAutoadjustClampMin = config.ReadValue("iAutoadjustClampMin", mAutoadjustClampMin);
	mAutoadjustClampMax = config.ReadValue("iAutoadjustClampMax", mAutoadjustClampMax);

	CustomSelectLoadConfig();

	((AnalogInput*)LR2HackBox::Get().mAnalogInput.get())->SetEnabled(mIsAnalogInput);
	MirrorGearshift(mIsMirrorGearshift);
	SetAutoadjustReset(mIsAutoadjustReset);
	SetSkipResultWaitIR(mIsSkipResultWaitIR);
	SetSkipResultSub(mIsSkipResultSub);
}

void Misc::SetHooks() {
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x9573), OnSetRetryFlag));
	mMidHooks.push_back(safetyhook::create_mid(mModuleBase + 0x0197DF, OnPlayIBeforeInput));
	mMidHooks.push_back(safetyhook::create_mid(mModuleBase + 0x019864, OnPlayIHandleEscape));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x0198C1), OnPlayISetSelecter));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x02D36C), OnInitPlay));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x0AD24D), OnInitRetry));

	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x029AFA), OnRandomMixInput));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x031BB6), OnSceneInitSwitch));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x033746), OnSceneProcSwitch));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x033830), OnSceneExitSwitch));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x01EE32), OnOpenFolderPlaySound));

	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x0B32AD), OnAddToAvgBpmSum));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x0B4366), OnCalcAvgSpeedmult));

	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x6D86), OnDrawNotesGetSongtimer));

	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x7A83), OnBeforeAddDrawingBuffer_LN));

	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x02C84F), OnAutoadjustInc));
	mMidHooks.push_back(safetyhook::create_mid((void*)(mModuleBase + 0x02C854), OnAutoadjustDec));

	oSetFirstSkins = safetyhook::create_inline(mModuleBase + 0x9D80, OnSetFirstSkins);
	mMidHooks.push_back(safetyhook::create_mid(mModuleBase + 0x03E4F8, OnWriteConfigXml));

	mMidHooks.push_back(safetyhook::create_mid(mModuleBase + 0x0BC8FF, OnIrSendSuccess));

	mMidHooks.push_back(safetyhook::create_mid(mModuleBase + 0x0A9C23, OnGhostDecodeAdd));

	if (MH_CreateHookEx((LPVOID)SaveDrawScreenToPNG, &OnSaveDrawScreenToPNG, &SaveDrawScreenToPNG) != MH_OK)
	{
		std::cout << "Couldn't hook SaveDrawScreenToPNG" << std::endl;
	}

	if (MH_CreateHookEx((LPVOID)AddDrawingBuffer_PlayArea, &OnAddDrawingBuffer_PlayArea, &AddDrawingBuffer_PlayArea) != MH_OK)
	{
		std::cout << "Couldn't hook AddDrawingBuffer_PlayArea" << std::endl;
	}

	if (MH_CreateHookEx((LPVOID)AddDrawingBuffer_LN, &OnAddDrawingBuffer_LN, &AddDrawingBuffer_LN) != MH_OK)
	{
		std::cout << "Couldn't hook AddDrawingBuffer_LN" << std::endl;
	}

	if (MH_CreateHookEx((LPVOID)SetObjectString, &OnSetObjectString, &SetObjectString) != MH_OK)
	{
		std::cout << "Couldn't hook SetObjectString" << std::endl;
	}

	if (MH_QueueEnableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		std::cout << ("Couldn't enable misc hooks") << std::endl;
	}
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

void Misc::CustomSelectMenu() {
	if (ImGui::TreeNode("Custom Select setup")) {
		char title[64];
		char condition[256];
		char order[256];
		memset(title, 0, sizeof(title));
		memset(condition, 0, sizeof(condition));
		memset(order, 0, sizeof(order));
		memcpy(title, mEditingCustomSelectEntry.title.data(), std::min(mEditingCustomSelectEntry.title.size(), sizeof(title)));
		memcpy(condition, mEditingCustomSelectEntry.condition.data(), std::min(mEditingCustomSelectEntry.condition.size(), sizeof(condition)));
		memcpy(order, mEditingCustomSelectEntry.order.data(), std::min(mEditingCustomSelectEntry.order.size(), sizeof(order)));
		
		ImGui::TextUnformatted("Title:");
		ImGui::SameLine();
		if (ImGui::InputText("##title", title, sizeof(title))) {
			mEditingCustomSelectEntry.title = title;
		}

		ImGui::TextUnformatted("SELECT hash WHERE");
		ImGui::SameLine();
		HelpMarker("Can be empty, then only order will be used.");
		ImGui::SameLine();
		if (ImGui::InputText("##condition", condition, sizeof(condition))) {
			mEditingCustomSelectEntry.condition = condition;
		}

		ImGui::TextUnformatted("ORDER BY");
		ImGui::SameLine();
		HelpMarker("Can be empty. Defaults to 'random()'.");
		ImGui::SameLine();
		if (ImGui::InputText("##order", order, sizeof(order))) {
			mEditingCustomSelectEntry.order = order;
		}

		if (mSelectedCustomSelectEntry != mCustomSelectEntries.end()) {
			if (ImGui::Button("Save") && !mEditingCustomSelectEntry.title.empty()) {
				*mSelectedCustomSelectEntry = mEditingCustomSelectEntry;
				mSelectedCustomSelectEntry = mCustomSelectEntries.end();
				mEditingCustomSelectEntry = CustomSelectEntry();
				CustomSelectSaveConfig();
			}
		}
		else {
			if (ImGui::Button("Add") && !mEditingCustomSelectEntry.title.empty()) {
				mCustomSelectEntries.push_back(mEditingCustomSelectEntry);
				mSelectedCustomSelectEntry = mCustomSelectEntries.end();
				mEditingCustomSelectEntry = CustomSelectEntry();
				CustomSelectSaveConfig();
			}
		}

		int flags = ImGuiTableFlags(ImGuiTableFlags_ScrollY) | ImGuiTableFlags(ImGuiTableFlags_RowBg)
			| ImGuiTableFlags(ImGuiTableFlags_BordersOuter) | ImGuiTableFlags(ImGuiTableFlags_Resizable)
			| ImGuiTableFlags(ImGuiTableFlags_SizingStretchSame);
		float outer_size = ImGui::GetTextLineHeightWithSpacing() * 4;
		if (ImGui::BeginTable("CustomSelectTable", 2, flags, ImVec2(0, outer_size))) {
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Buttons");

			decltype(mCustomSelectEntries.begin()) toDelete = mCustomSelectEntries.end();
			for (auto it = mCustomSelectEntries.begin(); it != mCustomSelectEntries.end(); it++) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				std::string sRowIdx = std::to_string(ImGui::TableGetRowIndex());
				ImGui::TextUnformatted(it->title.c_str());
				if (ImGui::IsItemHovered()) {
					ImGui::TableSetBgColor(ImGuiTableBgTarget(ImGuiTableBgTarget_CellBg), IM_COL32(110, 90, 20, 255));
					if (ImGui::IsMouseDoubleClicked(0)) {
						mSelectedCustomSelectEntry = it;
						mEditingCustomSelectEntry = *it;
					}
				}
				ImGui::TableSetColumnIndex(1);
				std::string deleteButtonId = "-##" + sRowIdx;
				if (ImGui::Button(deleteButtonId.c_str())) {
					toDelete = it;
					if (mSelectedCustomSelectEntry != mCustomSelectEntries.end())
						mEditingCustomSelectEntry = CustomSelectEntry();
				}
				if (ImGui::TableGetRowIndex() != 0) {
					ImGui::SameLine();
					
					std::string upButtonId = "up##" + sRowIdx;
					if (ImGui::ArrowButton(upButtonId.c_str(), ImGuiDir_Up)) {
						auto other = it - 1;
						std::swap(*it, *other);
						if (mSelectedCustomSelectEntry != mCustomSelectEntries.end())
							mSelectedCustomSelectEntry = other;
						CustomSelectSaveConfig();
					}
				}
				if (ImGui::TableGetRowIndex() != mCustomSelectEntries.size() - 1) {
					ImGui::SameLine();
					std::string downButtonId = "down##" + sRowIdx;
					if (ImGui::ArrowButton(downButtonId.c_str(), ImGuiDir_Down)) {
						auto other = it + 1;
						std::swap(*it, *other);
						if (mSelectedCustomSelectEntry != mCustomSelectEntries.end())
							mSelectedCustomSelectEntry = other;
						CustomSelectSaveConfig();
					}
				}
			}
			if (toDelete != mCustomSelectEntries.end()) {
				mCustomSelectEntries.erase(toDelete);
				mSelectedCustomSelectEntry = mCustomSelectEntries.end();
				CustomSelectSaveConfig();
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Button("Add Defaults")) {
				CustomSelectAddDefault();
				mSelectedCustomSelectEntry = mCustomSelectEntries.end();
				mEditingCustomSelectEntry = CustomSelectEntry();
				CustomSelectSaveConfig();
			}
			ImGui::EndTable();
		}
		ImGui::TreePop();
	}
	
}

void Misc::Menu() {
	LR2::game* game = LR2HackBox::Get().GetGame();
	ConfigManager& config = *LR2HackBox::Get().mConfig;

	if (ImGui::Checkbox("Restart Tweaks", &mIsRetryTweaks)) {
		config.WriteValueAndSave("bRetryTweaks", mIsRetryTweaks);
	}
	ImGui::SameLine();
	HelpMarker("When enabled:\n  Play: 'START' on fade-out to restart with a new random,\n  'SELECT' to restart with the same random. \n\n  Result: <any white key>+2 on fade-out\n  to restart with a new random.");

	if (ImGui::Checkbox("Custom Select", &mIsCustomSelect)) {
		config.WriteValueAndSave("bCustomSelect", mIsCustomSelect);
	}
	ImGui::SameLine();
	HelpMarker("Adds customizable entries to song folder, which can start a song from current folder matching the filter of the entry.\n\nFilters are SQL querries, where you can specify the condition and order.");

	if (mIsCustomSelect) {
		CustomSelectMenu();
	}

	if (ImGui::Checkbox("MainBPM hi-speed mode", &mIsMainBPM)) {
		config.WriteValueAndSave("bMainBPM", mIsMainBPM);
	}
	ImGui::SameLine();
	HelpMarker("Replaces the effect of AverageBPM hi-speed mode to that of MainBPM");

	if (ImGui::Checkbox("Reroute Screenshots", &mIsRerouteScreenshots)) {
		config.WriteValueAndSave("bRerouteScreenshots", mIsRerouteScreenshots);
	}
	ImGui::SameLine();
	HelpMarker("Reroutes screenshots to save in 'screenshots' folder");

	if (ImGui::Checkbox("Screenshots to Copybuffer", &mIsScreenshotsCopybuffer)) {
		config.WriteValueAndSave("bScreenshotsCopybuffer", mIsScreenshotsCopybuffer);
	}
	ImGui::SameLine();
	HelpMarker("Puts the screenshots in the copybuffer, to later access them with CTRL+V");

	if (ImGui::Checkbox("Mirror Lanecover Buttons", &mIsMirrorGearshift)) {
		config.WriteValueAndSave("bMirrorGearshift", mIsMirrorGearshift);

		MirrorGearshift(mIsMirrorGearshift);
	}
	ImGui::SameLine();
	HelpMarker("Mirrors controls for hi-speed and lanecover values, making lanecover on 1 and 2 instead of 6 and 7");

	if (ImGui::Checkbox("Fix LN Render", &mIsLNAnimFix)) {
		config.WriteValueAndSave("bLNAnimFix", mIsLNAnimFix);
	}
	ImGui::SameLine();
	HelpMarker("Fixes a bug, where all visible LNs of the same column would play the hold animation, instead of only the LN you are holding.\n\nSmoothes LN length inconsistency, which created small gaps between LN body, its tail and head.\n\nFixes brightness inconsistency for missed LNs, when they reach the judgeline.");

	if (ImGui::Checkbox("Autoadjust Clamp", &mIsAutoadjustClamp)) {
		config.WriteValueAndSave("bAutoadjustClamp", mIsAutoadjustClamp);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.f);
	if (ImGui::InputInt2("Min-Max", &mAutoadjustClampMin)) {
		config.WriteValue("iAutoadjustClampMin", mAutoadjustClampMin);
		config.WriteValue("iAutoadjustClampMax", mAutoadjustClampMax);
		config.SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Clamps the autoadjust to the bounds specified in 'Min-Max' boxes, not allowing it to adjust below min and above max");

	if (ImGui::Checkbox("Autoadjust Reset", &mIsAutoadjustReset)) {
		SetAutoadjustReset(mIsAutoadjustReset);
		config.WriteValueAndSave("bAutoadjustReset", mIsAutoadjustReset);
	}
	ImGui::SameLine();
	HelpMarker("Adds new type for autoadjust, which can be selected through settings menu of the game. It's called 'RESET', and after each play it resets the adjust to value before you started playing");

	if (ImGui::Checkbox("Fix Course Result Config", &mIsCourseResultFix)) {
		config.WriteValueAndSave("bCourseResultFix", mIsCourseResultFix);
	}
	ImGui::SameLine();
	HelpMarker("Fixes a problem where selected course result skin wouldn't save to the config file. Keep in mind that launching the game without this would lead to this setting being deleted again");

	if (ImGui::Checkbox("Skip Result Wait for IR", &mIsSkipResultWaitIR)) {
		config.WriteValueAndSave("bSkipResultWaitIR", mIsSkipResultWaitIR);
		SetSkipResultWaitIR(mIsSkipResultWaitIR);
	}
	ImGui::SameLine();
	HelpMarker("Disables blocking of input on result scene until IR finishes its process, allowing you to quit the scene early");

	if (ImGui::Checkbox("Skip Result Sub-Menu", &mIsSkipResultSub)) {
		config.WriteValueAndSave("bSkipResultSub", mIsSkipResultSub);
		SetSkipResultSub(mIsSkipResultSub);
	}
	ImGui::SameLine();
	HelpMarker("Skips the sub-menu of result scene, normally invoked upon the first input. Will still block for IR, unless disabled separately");

	if (ImGui::Checkbox("Quick IR on Result", &mIsResultQuickIR)) {
		config.WriteValueAndSave("bResultQuickIR", mIsResultQuickIR);
	}
	ImGui::SameLine();
	HelpMarker("Loads IR position for result screen as soon as the score sends, rather than waiting for all leaderboard scores to load.\nBest paired with \"Skip Result Wait for IR\".");

	RivalLeaderboard& rivalLb = *(RivalLeaderboard*)LR2HackBox::Get().mRivalLeaderboard.get();
	mIsRivalLeaderboard = rivalLb.GetEnabled();
	if (ImGui::Checkbox("Rival Leaderboard", &mIsRivalLeaderboard)) {
		rivalLb.SetEnabled(mIsRivalLeaderboard);
	}
	ImGui::SameLine();
	HelpMarker("While viewing global leaderboard holding 4, you can switch the leaderboard to display your rivals by holding 'SELECT'. Pressing 'SELECT' again switches it back. Uses cached data from 'LR2files/Rival' directory");

	BattleFixes& battleFixes = *(BattleFixes*)LR2HackBox::Get().mBattleFixes.get();
	mIsBattleFixes = battleFixes.GetEnabled();
	if (ImGui::Checkbox("Battle Fixes", &mIsBattleFixes)) {
		battleFixes.SetEnabled(mIsBattleFixes);
	}
	ImGui::SameLine();
	HelpMarker("Implements multiple fixes to make battle mode properly playable:\n\n Separate adjust value for P1 and P2, as well as autoadjust. See 'Game Options' section to change P2 values.\n\n Margin lanecover shift uninverted for P2 (black - up, white - down). P2 lanecover can be adjusted with mousewheel (or start+scratch on djdao controllers in LR2 mode) while holding P2 start/select.\n\n Fixes a bug where P1 side would get early game end animation, when P1 and P2 random types are different.");

	if (ImGui::Checkbox("Ghost Fix", &mIsGhostFix)) {
		config.WriteValueAndSave("bGhostFix", mIsGhostFix);
	}
	ImGui::SameLine();
	HelpMarker("Fixes a bug where last note of a ghost would sometimes get incorrect judgement, resulting in wrong score.");

	if (ImGui::Checkbox("Analog scratch support", &mIsAnalogInput)) {
		config.WriteValueAndSave("bAnalogInput", mIsAnalogInput);
		((AnalogInput*)LR2HackBox::Get().mAnalogInput.get())->SetEnabled(mIsAnalogInput);
	}
	ImGui::SameLine();
	HelpMarker("Lets you select a device and axis to use for scratch input. While enabled, set axis should be unbound in LR2 input settings");

	if (mIsAnalogInput) {
		LR2HackBox::Get().mAnalogInput->Menu();
	}

	/*if (ImGui::Button("Start Random Song")) {
		StartRandomFromFolder();
	}*/
}
