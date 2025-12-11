#define NOMINMAX
#include "ScoreCannon.hpp"

#include <print>
#include <codecvt>
#include <exception>

#include "LR2HackBox/LR2HackBox.hpp"
#include "Unrandomizer.hpp"
#include "Helpers/Helpers.hpp"

#include <safetyhook.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "imgui/imgui.h"

#define RGB(R, G, B) R << 16 | G << 8 | B

constexpr const char* lamps[] = { "NO PLAY", "FAILED", "EASY CLEAR", "GROOVE CLEAR", "HARD CLEAR", "FULL COMBO", "PERFECT", "MAX", "ASSIST CLEAR", "NONE" };
constexpr const char* grades[] = { "F", "E", "D", "C", "B", "A", "AA", "AAA", "MAX" };
constexpr const char* targets[] = { "NO TARGET", "MY BEST", "RANK AAA", "RANK AA", "RANK A", "TARGET%", "IR TOP", "IR NEXT", "IR AVERAGE", "ERROR" };
constexpr const char* randoms[] = { "NONRAN", "MIRROR", "RANDOM", "S-RANDOM", "H-RANDOM", "ALL-SCR" };

enum Grade {
	F,
	E,
	D,
	C,
	B,
	A,
	AA,
	AAA,
	MAX
};

static std::wstring s2ws(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

static std::string ws2utf(const std::wstring& str) {
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	std::string converted_str = converter.to_bytes(str);
	return converted_str;
}

static std::string s2utf(const std::string& str) {
	return ws2utf(s2ws(str));
}

static const std::string GetLampText(const ScoreCannon::Score& score) {
	if (score.lampBest == ScoreCannon::Score::Lamp::NONE) {
		return ":fireworks: First play!";
	}
	else if (score.lamp > score.lampBest) {
		return std::format(":arrow_up: Raised from: {}", lamps[score.lampBest]).c_str();
	}
	else if (score.lamp < score.lampBest) {
		return std::format(":arrow_down: Best clear: {}", lamps[score.lampBest]).c_str();
	}
	else {
		return ":arrow_right: Matched best lamp";
	}
}

static const unsigned int GetLampRGB(const ScoreCannon::Score& score) {
	switch (score.lamp) {
	case ScoreCannon::Score::Lamp::FAILED:
		return RGB(138,0,0);
	case ScoreCannon::Score::Lamp::EASYCLEAR:
		return RGB(0,215,15);
	case ScoreCannon::Score::Lamp::GROOVECLEAR:
		return RGB(34,154,255);
	case ScoreCannon::Score::Lamp::HARDCLEAR:
		return RGB(254,54,54);
	case ScoreCannon::Score::Lamp::FULLCOMBO:
		return RGB(120,255,247);
	case ScoreCannon::Score::Lamp::PERFECT:
		return RGB(251,230,255);
	case ScoreCannon::Score::Lamp::MAX:
		return RGB(255,255,255);
	case ScoreCannon::Score::Lamp::NOPLAY:
		return RGB(127,127,127);
	case ScoreCannon::Score::Lamp::ASSISTCLEAR:
		return RGB(37,150,190);
	default:
		return 0;
	}
}

static const std::string GetDelta(const int& val1, const int& val2) {
	int delta = val1 - val2;
	if (delta >= 0) {
		return std::format("+{}", delta);
	}
	else {
		return std::to_string(delta);
	}
}

static const std::string GetDeltaNotation(const int& val1, const int& val2) {
	return val1 != val2 ? val1 > val2 ? ":arrow_up:" : ":arrow_down:" : ":arrow_right:";
}

static const float GetExPercentage(const int& exScore, const int& exScoreMax) {
	return static_cast<float>(exScore) / exScoreMax * 100.f;
}

static const std::string GetExGradeDelta(const int& exScore, const int& exScoreMax) {
	float score = static_cast<float>(exScore);
	float scoreMax = static_cast<float>(exScoreMax);
	Grade gradeNotation(F);
	int gradeScore = 0;
	if (score / scoreMax > 8.5f / 9.f) {
		gradeNotation = MAX;
		gradeScore = scoreMax;
	}
	else if (score / scoreMax > 7.5f / 9.f) {
		gradeNotation = AAA;
		gradeScore = static_cast<int>(scoreMax * 8.f / 9.f);
	}
	else if (score / scoreMax > 6.5f / 9.f) {
		gradeNotation = AA;
		gradeScore = static_cast<int>(scoreMax * 7.f / 9.f);
	}
	else if (score / scoreMax > 5.5f / 9.f) {
		gradeNotation = A;
		gradeScore = static_cast<int>(scoreMax * 6.f / 9.f);
	}
	else if (score / scoreMax > 4.5f / 9.f) {
		gradeNotation = B;
		gradeScore = static_cast<int>(scoreMax * 5.f / 9.f);
	}
	else if (score / scoreMax > 3.5f / 9.f) {
		gradeNotation = C;
		gradeScore = static_cast<int>(scoreMax * 4.f / 9.f);
	}
	else if (score / scoreMax > 2.5f / 9.f) {
		gradeNotation = D;
		gradeScore = static_cast<int>(scoreMax * 3.f / 9.f);
	}
	else {
		gradeNotation = E;
		gradeScore = static_cast<int>(scoreMax * 2.f / 9.f);
	}

	return std::format("{}{}", grades[gradeNotation], GetDelta(score, gradeScore));
}

static const Grade GetExGrade(const int& exScore, const int& exScoreMax) {
	float score = static_cast<float>(exScore);
	float scoreMax = static_cast<float>(exScoreMax);
	if (exScore == exScoreMax) {
		return MAX;
	}
	else if (score / scoreMax >= 8.f / 9.f) {
		return AAA;
	}
	else if (score / scoreMax >= 7.f / 9.f) {
		return AA;
	}
	else if (score / scoreMax >= 6.f / 9.f) {
		return A;
	}
	else if (score / scoreMax >= 5.f / 9.f) {
		return B;
	}
	else if (score / scoreMax >= 4.f / 9.f) {
		return C;
	}
	else if (score / scoreMax >= 3.f / 9.f) {
		return D;
	}
	else if (score / scoreMax >= 2.f / 9.f) {
		return E;
	}
	else {
		return F;
	}
}

std::string ScoreCannon::GetJsonString(const Score& score) {
	nlohmann::ordered_json data = {
		{"username", mDiscordName},
		{"avatar_url", mDiscordAvatarUrl},
		{"embeds", {
			{
				{"title", std::format("{} {} {} {}", score.folder, score.title, score.subtitle, lamps[score.lamp])},
				{"color", GetLampRGB(score)},
				{"author", {
					{"name", std::format("{} just got a new score!", mGameName)}
				}},
				{"image", {
					{"url", "attachment://screenshot.png"}
				}},
				{"footer", {
					{"text", "LR2HackBox Scorecard"}
				}}
			}
		}},
		{"attachments", {
			{
				{"id", 0},
				{"description", "screenshot"},
				{"filename", "screenshot.png"}
			}
		}}
	};

	switch (mMessageFormat) {
	case COMPACT: {
		data["embeds"][0]["description"] = std::format(
			"**DJ LEVEL:** {}\n"
			"**EX SCORE:** {}\n"
			"**BP:** {}\n"
			"**IR RANK:** {}\n"
			"**ARRANGE:** {}",
			std::format("**{}** ({:.2f}%) {}", // DJ LEVEL
				GetExGradeDelta(score.exScore, score.exScoreMax),
				GetExPercentage(score.exScore, score.exScoreMax),
				GetDeltaNotation(GetExGrade(score.exScore, score.exScoreMax), GetExGrade(score.exScoreBest, score.exScoreMax))
			),
			std::format("**{}** ({}) {}", // EX SCORE
				score.exScore,
				GetDelta(score.exScore, score.exScoreBest),
				GetDeltaNotation(score.exScore, score.exScoreBest)
			),
			std::format("**{}** ({}) {}", // BP
				score.missCount,
				GetDelta(score.missCount, score.missCountBest),
				GetDeltaNotation(score.missCountBest, score.missCount)
			),
			std::format("**{}/{}** ({}) {}", // IR RANK
				score.irPosition,
				score.irCount,
				GetDelta(score.irPosition, score.irPositionBest),
				GetDeltaNotation(score.irPositionBest, score.irPosition)
			),
			// ARRANGE
			score.random == Score::Random::NONRAN || score.random == Score::Random::MIRROR || score.random == Score::Random::RANDOM ?
			std::format("**{}** ({})", score.arrange, score.rseed) :
			std::format("**{}** ({})", randoms[score.random], score.rseed)
		);
		break;
	}
	case FULL: {
		data["embeds"][0]["fields"] = {
			{
				{"name", std::format("Arrange: {}", randoms[score.random])},
				{"value", score.random == Score::Random::NONRAN || score.random == Score::Random::MIRROR || score.random == Score::Random::RANDOM
					? std::format("Order: {} ({})", score.arrange, score.rseed)
					: std::format("Seed: {}", score.rseed)
				}
			},
			{
				{"name", std::format("Rank: {} ({:.2f}%) {}", GetExGradeDelta(score.exScore, score.exScoreMax), GetExPercentage(score.exScore, score.exScoreMax), GetDeltaNotation(GetExGrade(score.exScore, score.exScoreMax), GetExGrade(score.exScoreBest, score.exScoreMax)))},
				{"value", score.target == Score::Target::PERCENT ? std::format("Target: {:.0f}%, ({})", GetExPercentage(score.exScoreTarget, score.exScoreMax), GetDelta(score.exScore, score.exScoreTarget))
					: std::format("Target: {} ({})", targets[score.target], GetDelta(score.exScore, score.exScoreTarget))
				}
			},
			{
				{"name", std::format("EX Score: {} {}", score.exScore, GetDeltaNotation(score.exScore, score.exScoreBest))},
				{"value", std::format("({})", GetDelta(score.exScore, score.exScoreBest))}
			},
			{
				{"name", std::format("Max Combo: {}/{} {}", score.maxCombo, score.exScoreMax / 2, GetDeltaNotation(score.maxCombo, score.maxComboBest))},
				{"value", std::format("({})", GetDelta(score.maxCombo, score.maxComboBest))}
			},
			{
				{"name", std::format("Miss Count: {} {}", score.missCount, GetDeltaNotation(score.missCountBest, score.missCount))},
				{"value", std::format("({})", GetDelta(score.missCount, score.missCountBest))}
			},
			{
				{"name", std::format("IR Position: {}/{} {}", score.irPosition, score.irCount, GetDeltaNotation(score.irPositionBest, score.irPosition))},
				{"value", std::format("({})", GetDelta(score.irPosition, score.irPositionBest))}
			}
		};
		break;
	default: break;
	}
	}
	return data.dump(4);
}

bool ScoreCannon::PostScore(const Score& score, const std::string& screenshotPath) {
	std::string data = GetJsonString(score);
#if _DEBUG
	{
		std::println("{}", data);
		fflush(stdout);
		std::ofstream dump("ScoreCannonDump.json");
		dump << data;
	}
#endif
	for (auto& url : mUrls) {
		std::thread([url, data, screenshotPath]() {
			cpr::Response r = cpr::Post(
				cpr::Url{ url },
				cpr::Header{ {"User-Agent", "DiscordBot (https://discord.com, 1.0)"} },
				cpr::Multipart{
					{"payload_json", data},
					{"files[0]", cpr::File(screenshotPath, "screenshot.png")}
				}
			);
			if (r.error.code != cpr::ErrorCode::OK) {
				std::println("{}", data);
				std::println("[LR2HackBox] Coulnd't POST to {}: {}", url, r.error.message);
				std::fflush(stdout);
			}
			else if (r.status_code != 200) {
				std::println("{}", data);
				std::println("[LR2HackBox] Message rejected by {}: {}", url, r.status_line);
				std::fflush(stdout);
			}
		}).detach();
	}
	mAlreadySent = true;
	return true;
}

ScoreCannon::Score::Score(std::string folder, std::string title, std::string subtitle, Lamp lamp, Lamp lampBest,
	Target target, Random random, std::string arrange, int rseed,
	int exScore, int exScoreMax, int exScoreBest, int exScoreTarget,
	int maxCombo, int maxComboBest, int missCount, int missCountBest, 
	int irPosition, int irPositionBest, int irCount) :
	folder(folder), title(title), subtitle(subtitle), lamp(lamp), lampBest(lampBest),
	target(target), random(random), arrange(arrange), rseed(rseed),
	exScore(exScore), exScoreMax(exScoreMax), exScoreBest(exScoreBest), exScoreTarget(exScoreTarget),
	maxCombo(maxCombo), maxComboBest(maxComboBest), missCount(missCount), missCountBest(missCountBest), 
	irPosition(irPosition), irPositionBest(irPositionBest), irCount(irCount) {

}

ScoreCannon::Score::Score(void* g) {
	LR2::game& game = *(LR2::game*)g;

	folder = s2utf(game.sSelect.stack_searchTitle[game.sSelect.cur].body);
	if (game.gameplay.courseType == 0 || game.gameplay.courseType == 2) {
		if (game.procSelecter == 5) {
			title = s2utf(game.sSelect.bmsList[game.sSelect.cur_song].courseTitle[game.gameplay.courseStageNow].body);
			subtitle = s2utf(game.sSelect.bmsList[game.sSelect.cur_song].courseSubtitle[game.gameplay.courseStageNow].body);
		}
		else if (game.procSelecter == 13) {
			title = s2utf(game.sSelect.bmsList[game.sSelect.cur_song].title.body);
			subtitle = s2utf(game.sSelect.bmsList[game.sSelect.cur_song].subtitle.body);
		}
	}
	else {
		title = s2utf(game.sSelect.metaSelected.title.body);
		subtitle = s2utf(game.sSelect.metaSelected.subtitle.body);
	}
	lamp = Lamp(game.gameplay.player[0].clearType);
	lampBest = game.sSelect.old.playcount <= 0 && game.sSelect.old.stat_exscore <= 0 ? Lamp::NONE : Lamp(game.sSelect.old.clear);
	target = Target(game.config.play.p1_target);
	random = Random(game.config.play.random[0]);
	arrange = random != RANDOM ? random != MIRROR ?  "1234567" : "7654321" : ((Unrandomizer*)LR2HackBox::Get().mUnrandomizer.get())->GetLastRandom();
	rseed = game.gameplay.randomseed;
	exScore = game.gameplay.player[0].judgecount[5] * 2 + game.gameplay.player[0].judgecount[4];
	exScoreMax = game.gameplay.player[0].totalnotes * 2;
	exScoreBest = game.sSelect.old.stat_exscore;
	exScoreTarget = game.gameplay.targetScore.exscore;
	maxCombo = game.procSelecter == 13 ? game.gameplay.player[0].max_combo_course : game.gameplay.player[0].max_combo;
	maxComboBest = game.sSelect.old.stat_maxcombo;
	missCount = game.gameplay.player[0].judgecount[1] + game.gameplay.player[0].judgecount[2] + game.gameplay.player[0].totalnotes - game.gameplay.player[0].note_current;
	missCountBest = game.sSelect.old.playcount <= 0 ? 0 : game.sSelect.old.minbp;
	irPosition = game.net.rankingData.myRanking;
	irPositionBest = game.sSelect.old.IRranking;
	irCount = game.net.rankingData.rankingCount;
}

bool ScoreCannon::Init(uintptr_t moduleBase) {
	ScoreCannon::mModuleBase = moduleBase;
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	mIsEnabled = config.ReadValue("bIsScoreCannon", mIsEnabled);
	mMessageFormat = static_cast<MessageFormat>(config.ReadValue("iScoreCannonFormat", (int)mMessageFormat));
	mDiscordName = config.ReadValue("sScoreCannonDiscordName", mDiscordName);
	mDiscordAvatarUrl = config.ReadValue("sScoreCannonDiscordAvatarUrl", mDiscordAvatarUrl);
	config.ReadArray("sScoreCannonDiscordWebhookUrls", mUrls);

	LR2::game& game = *LR2HackBox::Get().GetGame();
	mGameName = s2utf(game.sSelect.playerID.body);

	return true;
}

bool ScoreCannon::Deinit() {
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

void ScoreCannon::Menu() {
	struct IdPopper { IdPopper(const char* id) { ImGui::PushID(id); };  ~IdPopper() { ImGui::PopID(); } } _id_popper{ "ScoreCannon" };

	ConfigManager& config = *LR2HackBox::Get().mConfig;

	char discordName[64];
	char discordAvatarUrl[256];
	char discordWebhookUrl[256];
	memset(discordName, 0, sizeof(discordName));
	memset(discordAvatarUrl, 0, sizeof(discordAvatarUrl));
	memset(discordWebhookUrl, 0, sizeof(discordAvatarUrl));
	memcpy(discordName, mDiscordName.data(), std::min(mDiscordName.size(), sizeof(discordAvatarUrl)));
	memcpy(discordAvatarUrl, mDiscordAvatarUrl.data(), std::min(mDiscordAvatarUrl.size(), sizeof(discordAvatarUrl)));
	memcpy(discordWebhookUrl, mEditingUrl.data(), std::min(mEditingUrl.size(), sizeof(discordWebhookUrl)));

	if (ImGui::Checkbox("Enable", &mIsEnabled)) {
		config.WriteValueAndSave("bIsScoreCannon", mIsEnabled);
	}
	ImGui::SameLine();
	HelpMarker("When enabled, sends score textcards to specified Discord webhooks, if screenshot is taken on result screen");

	bool formatChanged = false;
	if (ImGui::RadioButton("Lamp only", (int*)&mMessageFormat, LAMP_ONLY)) {
		formatChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Compact", (int*)&mMessageFormat, COMPACT)) {
		formatChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Full", (int*)&mMessageFormat, FULL)) {
		formatChanged = true;
	}
	if (formatChanged) config.WriteValueAndSave("iScoreCannonFormat", (int)mMessageFormat);

	if (ImGui::InputText("Discord Name", discordName, sizeof(discordName))) {
		mDiscordName = discordName;
		config.WriteValueAndSave("sScoreCannonDiscordName", mDiscordName);
	}
	if (ImGui::InputText("Discord Avatar URL", discordAvatarUrl, sizeof(discordAvatarUrl))) {
		mDiscordAvatarUrl = discordAvatarUrl;
		config.WriteValueAndSave("sScoreCannonDiscordAvatarUrl", mDiscordAvatarUrl);
	}
	if (ImGui::InputText("Discord Webhook URL", discordWebhookUrl, sizeof(discordWebhookUrl))) {
		mEditingUrl = discordWebhookUrl;
	}
	ImGui::SameLine();
	if (ImGui::Button("+")) {
		if (!mEditingUrl.empty()) {
			bool repeating = false;
			for (auto& url : mUrls) {
				if (url == mEditingUrl) {
					repeating = true;
					break;
				}
			}
			if (!repeating) {
				mUrls.push_back(mEditingUrl);
				config.WriteArrayAndSave("sScoreCannonDiscordWebhookUrls", mUrls);
			}
			mEditingUrl.clear();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("-")) {
		for (auto url = mUrls.begin(); url != mUrls.end(); url++) {
			if (*url == mEditingUrl) {
				mUrls.erase(url);
				config.WriteArrayAndSave("sScoreCannonDiscordWebhookUrls", mUrls);
				break;
			}
		}
		mEditingUrl.clear();
	}

	int flags = ImGuiTableFlags(ImGuiTableFlags_ScrollY) | ImGuiTableFlags(ImGuiTableFlags_RowBg)
		| ImGuiTableFlags(ImGuiTableFlags_BordersOuter) | ImGuiTableFlags(ImGuiTableFlags_Resizable)
		| ImGuiTableFlags(ImGuiTableFlags_SizingStretchSame);
	float outer_size = ImGui::GetTextLineHeightWithSpacing() * 4;
	if (ImGui::BeginTable("ScoreCannonUrls", 1, flags, ImVec2(0, outer_size))) {
		ImGui::TableSetupColumn("Current Webhook URL List");
		ImGui::TableHeadersRow();

		for (auto& url : mUrls) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(url.c_str());
			if (ImGui::IsItemHovered()) {
				ImGui::TableSetBgColor(ImGuiTableBgTarget(ImGuiTableBgTarget_CellBg), IM_COL32(110, 90, 20, 255));
				if (ImGui::IsMouseDoubleClicked(0)) {
					mEditingUrl = url;
				}
			}
		}
		ImGui::EndTable();
	}
}