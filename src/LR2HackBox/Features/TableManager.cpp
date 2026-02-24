#define NOMINMAX
#include "TableManager.hpp"

#include "Misc.hpp"
#include "LogConsole.hpp"

#include <LR2HackBox/LR2HackBox.hpp>
#include <ImGuiInjector/ImGuiInjector.hpp>
#include <filesystem>
#include <fstream>
#include <format>
#include <codecvt>
#include <ranges>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <json/single_include/nlohmann/json.hpp>

static std::wstring s2ws(const std::string_view str) {
	int size_needed = MultiByteToWideChar(CP_OEMCP, 0, str.data(), (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_OEMCP, 0, str.data(), (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

static std::string ws2utf(const std::wstring& str) {
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	std::string converted_str = converter.to_bytes(str);
	return converted_str;
}

static std::string s2utf(const std::string_view str) {
	return ws2utf(s2ws(str));
}

TableManager::Table::Table(std::string name, std::string symbol, std::filesystem::path path, std::string dataUrl, nlohmann::ordered_json originalJson) : name(std::move(name)), symbol(std::move(symbol)), path(std::move(path)), dataPath(!dataUrl.empty() ? std::move(dataUrl) : "./data.json"), originalJson(std::move(originalJson)) {}

TableManager::Entry::Entry(std::string level, std::string md5, std::string title, std::string artist, nlohmann::ordered_json originalJson) : level(std::move(level)), md5(std::move(md5)), title(std::move(title)), artist(std::move(artist)), originalJson(std::move(originalJson)) {
	if (this->title.empty()) {
		std::string dbTitle;
		Misc::SqliteGetColumn(&dbTitle, std::format("SELECT title FROM song WHERE hash = '{}'", this->md5), 0);
		std::string dbSubtitle;
		Misc::SqliteGetColumn(&dbSubtitle, std::format("SELECT subtitle FROM song WHERE hash = '{}'", this->md5), 0);
		this->title = dbSubtitle.empty()
			? s2utf(dbTitle)
			: s2utf(std::format("{} {}", dbTitle, dbSubtitle)); // IDC whitespace
	}
	if (this->artist.empty()) {
		std::string dbArtist;
		Misc::SqliteGetColumn(&dbArtist, std::format("SELECT artist FROM song WHERE hash = '{}'", this->md5), 0);
		std::string dbSubartist;
		Misc::SqliteGetColumn(&dbSubartist, std::format("SELECT subartist FROM song WHERE hash = '{}'", this->md5), 0);
		this->artist = dbSubartist.empty()
			? s2utf(dbArtist)
			: s2utf(std::format("{} {}", dbArtist, dbSubartist)); // IDC whitespace
	}
}
void TableManager::Table::AddEntry(std::string level, std::string md5, std::string title, std::string artist) {
	AddEntry(level, md5, title, artist, {
		{"level", level},
		{"md5", md5},
		{"title", title},
		{"artist", artist}
		});
}
void TableManager::Table::AddEntry(std::string level, std::string md5, std::string title, std::string artist, nlohmann::ordered_json originalJson) {
	entries.emplace_back(std::move(level), std::move(md5), std::move(title), std::move(artist), std::move(originalJson));
}

void TableManager::Table::WriteFile() {
	TableManager& manager = *(TableManager*)LR2HackBox::Get().mTableManager.get();
	if (!std::filesystem::exists(path))
		std::filesystem::create_directories(path);

	auto entries = this->entries;
	std::ranges::sort(entries, {}, [](const Entry& e) { return std::pair{ e.md5, e.level }; });

	std::ofstream headerFile(path / "header.json");
	nlohmann::ordered_json header = originalJson;
	header["name"] = name;
	if (header.contains("symbol")) header["symbol"] = symbol;
	else header.push_back({ "symbol", symbol });
	headerFile << header.dump(4);

	std::ofstream dataFile(path / dataPath);
	nlohmann::ordered_json data;
	for (auto& entry : entries) {
		auto& json = data.emplace_back(entry.originalJson);
		json["level"] = entry.level;
		json["md5"] = entry.md5;
		if (!entry.title.empty()) json["title"] = entry.title;
		if (!entry.artist.empty()) json["artist"] = entry.artist;
	}
	dataFile << data.dump(4);
}

bool TableManager::CreateTable(std::string name, std::string symbol) {
	auto path = mOutputPaths[mOutputIdx] / name;
	if (!std::ranges::contains(mTables, name, &Table::name) && !std::filesystem::exists(path)) {
		mTables.emplace_back(std::move(name), std::move(symbol), std::move(path), "data.json", nlohmann::ordered_json{
			{"name", name},
			{"symbol", symbol},
			{"data_url", "./data.json"}
		});
		mTables.back().WriteFile();
		mTableNames.push_back(mTables.back().name);
		return true;
	}
	return false;
}
bool TableManager::RemoveTable(std::string name) {
	if (auto table = std::ranges::find(mTables, name, &Table::name); table != mTables.end()) {
		try {
			std::filesystem::remove_all(table->path);
		}
		catch (std::filesystem::filesystem_error& e) {
			LogConsole::AddLog(LOG_INFO, "Failed to remove table folder: ", e.what());
			return false;
		}
		mTableNames.erase(std::remove(mTableNames.begin() + 1, mTableNames.end(), name), mTableNames.end());
		mTables.erase(table);
		return true;
	}
	return false;
}
bool TableManager::RenameTable(std::string name, std::string newName) {
	if (auto table = std::ranges::find(mTables, name, &Table::name); table != mTables.end() && !std::ranges::contains(mTables, newName, &Table::name)) {
		table->name = newName;
		int nameIdx = 1;
		auto tableName = std::ranges::find(mTableNames, std::move(name));
		*tableName = std::move(newName);
	}
	return false;
}
bool TableManager::MoveTable(std::string name, std::string newName) {
	if (auto table = std::ranges::find(mTables, name, &Table::name); table != mTables.end() && !std::ranges::contains(mTables, newName, &Table::name)) {
		Table renamedTable = *table;
		renamedTable.path = mOutputPaths[mOutputIdx] / newName;
		int nameIdx = 1;
		for (auto it = mTableNames.begin() + 1; it != mTableNames.end(); it++) {
			if (mTableNames[nameIdx] == name) break;
			nameIdx++;
		}
		if (RemoveTable(name)) {
			*table = renamedTable;
			table->WriteFile();
			mTableNames.insert(mTableNames.begin() + nameIdx, table->name);
			return true;
		}
	}
	return false;
}

void TableManager::Reload() {
	mTables.clear();
	mTableNames.clear();
	mTableNames.push_back("NEW TABLE");
	mNewTable.name = "NEW TABLE";
	if (mOutputIdx < 0 || mOutputPaths.empty()) return;
	std::filesystem::path selectedPath = mOutputPaths[mOutputIdx];
	std::filesystem::create_directories(selectedPath);
	for (auto& dir : std::filesystem::directory_iterator(selectedPath)) {
		if (!dir.is_directory()) continue;
		for (auto& file : std::filesystem::directory_iterator(dir.path())) {
			if (file.path().filename() != "header.json") continue;
			std::ifstream headerFile(file.path());
			if (!headerFile.good())
			{
				LogConsole::AddLog(LOG_WARNING, "Bad header file");
				continue;
			}
			try {
				nlohmann::ordered_json header = nlohmann::ordered_json::parse(headerFile);
				auto get_valid = [](const nlohmann::json& j, const char* key) {
					auto it = j.find(key); return it != j.end() && it->is_string() ? *it : "";
					};
				std::string tableName = get_valid(header, "name");
				std::string tableSymbol = get_valid(header, "symbol");
				std::string dataUrl = get_valid(header, "data_url");
				if (tableName.empty()) continue;
				if (dataUrl.empty()) continue;
				Table table(tableName, tableSymbol, file.path().parent_path().string(), dataUrl, header);
				if (std::ifstream dataFile(dir.path() / table.dataPath); dataFile.good()) {
					try {
						nlohmann::ordered_json data = nlohmann::ordered_json::parse(dataFile);
						for (auto& entry : data) {
							std::string entryLevel = get_valid(entry, "level");
							std::string entryMd5 = get_valid(entry, "md5");
							std::string entryTitle = get_valid(entry, "title");
							std::string entryArtist = get_valid(entry, "artist");
							if (entryLevel.empty()) continue;
							if (entryMd5.empty()) continue;
							table.AddEntry(entryLevel, entryMd5, entryTitle, entryArtist, entry);
						}
					}
					catch (const std::exception& e) {
						LogConsole::AddLog(LOG_WARNING, "parsing data file failed: ", e.what());
					}
				}
				if (!std::ranges::contains(mTables, tableName, &Table::name)) {
					mTables.emplace_back(table);
					mTableNames.push_back(mTables.back().name);
				}
			}
			catch (const std::exception& e) {
				LogConsole::AddLog(LOG_WARNING, "parsing header file failed: ", e.what());
				continue;
			}
		}
	}
}

bool TableManager::Init(uintptr_t moduleBase) {
	TableManager::mModuleBase = moduleBase;
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	config.ReadArray("sTableManagerOutputPaths", mOutputPaths);
	std::filesystem::path selectedPath = config.ReadValue<std::filesystem::path>("sTableManagerSelectedPath", "");
	int selectedOutputIdx = 0;
	bool hasOutput = false;
	for (auto& path : mOutputPaths) {
		if (path == selectedPath) {
			hasOutput = true;
			break;
		}
		selectedOutputIdx++;
	}
	mOutputIdx = mOutputPaths.empty() || !hasOutput ? -1 : selectedOutputIdx;
	Reload();

	ImGuiInjector::Get().AddMenu(&mMenu);

	return true;
}

bool TableManager::Deinit() {
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

void TableManager::Gui() {
	struct IdPopper { IdPopper(const char* id) { ImGui::PushID(id); };  ~IdPopper() { ImGui::PopID(); } } _id_popper{ "TableManager" };
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	if (mOutputIdx < 0) {
		ImGui::TextUnformatted("Select output to proceed...");

		ImVec2 tableSize(0.f, ImGui::GetTextLineHeightWithSpacing() * 5);
		int flags = ImGuiTableFlags(ImGuiTableFlags_ScrollY) | ImGuiTableFlags(ImGuiTableFlags_RowBg)
			| ImGuiTableFlags(ImGuiTableFlags_BordersOuter) | ImGuiTableFlags(ImGuiTableFlags_Resizable)
			| ImGuiTableFlags(ImGuiTableFlags_SizingStretchSame);
		if (ImGui::BeginTable("Table Manager Output Selector", 2, flags, tableSize)) {
			ImGui::TableSetupColumn("Path");
			ImGui::TableSetupColumn("Controls");

			auto toDelete = mOutputPaths.end();
			for (auto& path : mOutputPaths) {
				ImGui::TableNextRow();
				int rowIdx = ImGui::TableGetRowIndex();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(path.string().c_str());
				if (ImGui::IsItemHovered()) {
					ImGui::TableSetBgColor(ImGuiTableBgTarget(ImGuiTableBgTarget_CellBg), IM_COL32(110, 90, 20, 255));
					if (ImGui::IsMouseDoubleClicked(0)) {
						mOutputIdx = rowIdx;
						Reload();
						config.WriteValueAndSave("sTableManagerSelectedPath", path);
					}
				}
				ImGui::TableSetColumnIndex(1);
				std::string buttonImIndex = std::format("-##{}", rowIdx);
				if (ImGui::Button(buttonImIndex.c_str())) {
					toDelete = mOutputPaths.begin() + ImGui::TableGetRowIndex();
				}
			}
			if (toDelete != mOutputPaths.end()) {
				mOutputPaths.erase(toDelete);
				config.WriteArrayAndSave("sTableManagerOutputPaths", mOutputPaths);
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			static std::string newPath = "NEW OUTPUT";
			if (ImGui::InputText("", &newPath)) {
				if (newPath.empty()) newPath = "NEW OUTPUT";
			}
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button("+")) {
				if (!newPath.ends_with('/') && (!newPath.ends_with('\\') || !newPath.ends_with("\\\\"))) newPath += '/';
				if (!std::ranges::contains(mOutputPaths, newPath)) {
					mOutputPaths.push_back(newPath);
				}
				newPath = "NEW OUTPUT";
				config.WriteArrayAndSave("sTableManagerOutputPaths", mOutputPaths);
			}
			ImGui::EndTable();
		}
		return;
	}
	static int item_selected_idx = 0;
	static int last_item_selected_idx = 0;
	static bool force_resort = false;
	static bool force_find = false;
	static std::string name = "NEW TABLE";

	const char* combo_preview_value = mTableNames[item_selected_idx].c_str();
	if (ImGui::BeginCombo("Selected Table", combo_preview_value))
	{
		int n = 0;
		for (auto& tableName : mTableNames) {
			const bool is_selected = (item_selected_idx == n);
			std::string label = std::format("{}##{}", tableName, n);
			if (ImGui::Selectable(label.c_str(), is_selected)) {
				last_item_selected_idx = item_selected_idx;
				item_selected_idx = n;
				name = tableName;
			}

			if (is_selected)
				ImGui::SetItemDefaultFocus();
			n++;
		}
		ImGui::EndCombo();
	}

	static Table* selectedTable = &mNewTable;
	if (item_selected_idx != last_item_selected_idx) {
		selectedTable = item_selected_idx == 0 ? &mNewTable : &(*std::ranges::find(mTables, mTableNames[item_selected_idx], &Table::name));
		last_item_selected_idx = item_selected_idx;
		force_resort = true;
	}

	if (ImGui::InputText("Name", &name)) {
		if (name.empty()) name = "NO NAME";
		if (item_selected_idx == 0) {
			selectedTable->name = name;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Apply")) {
		if (item_selected_idx != 0) {
			RenameTable(selectedTable->name, name);
		}
	}
	ImGui::InputText("Symbol", &selectedTable->symbol);

	static std::optional<ptrdiff_t> tables_current_bottom_idx;
	static std::optional<ptrdiff_t> tables_move_to_this_idx;
	auto do_add = []{
		LR2::SONGSELECT& select = LR2HackBox::Get().GetGame()->sSelect;
		LR2::SONGDATA& curSong = select.bmsList[select.cur_song];
		if (curSong.folderType == 0 && curSong.courseType < 0 && !std::string_view(curSong.hash.body).empty()) {
			selectedTable->AddEntry("0", curSong.hash.body, ""/*load from db*/, ""/*load from db*/);
			force_resort = true;
		}
	};
	auto do_find = []{
		if (!tables_current_bottom_idx.has_value()) {
			return;
		}
		LR2::SONGSELECT& select = LR2HackBox::Get().GetGame()->sSelect;
		LR2::SONGDATA& curSong = select.bmsList[select.cur_song];
		std::string_view hash{ curSong.hash.body };
		auto nextEntryIdx = std::min(static_cast<size_t>(*tables_current_bottom_idx + 1), selectedTable->entries.size());
		auto it = std::ranges::find(selectedTable->entries.begin() + nextEntryIdx, selectedTable->entries.end(), hash, &Entry::md5);
		if (it == selectedTable->entries.end()) {
			it = std::ranges::find(selectedTable->entries.begin(), selectedTable->entries.begin() + (nextEntryIdx - 1), hash, &Entry::md5);
			if (it == selectedTable->entries.begin() + (nextEntryIdx - 1) /*sub-end*/) {
				it = selectedTable->entries.end();
			}
		}
		if (it != selectedTable->entries.end()) {
			tables_move_to_this_idx = std::distance(selectedTable->entries.begin(), it);
		}
	};
	constexpr int flags = ImGuiTableFlags(ImGuiTableFlags_ScrollY) | ImGuiTableFlags(ImGuiTableFlags_RowBg)
		| ImGuiTableFlags(ImGuiTableFlags_BordersOuter) | ImGuiTableFlags(ImGuiTableFlags_Resizable)
		| ImGuiTableFlags(ImGuiTableFlags_SizingStretchSame) | ImGuiTableFlags(ImGuiTableFlags_Sortable)
		| ImGuiTableFlags(ImGuiTableFlags_SortTristate) | ImGuiTableFlags(ImGuiTableFlags_SortMulti);
	const auto tableHeightForWindowSize = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - ImGui::GetCursorPosY() - ImGui::GetFrameHeightWithSpacing() * 3;
	const auto tableRowHeightRough = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
	if (ImGui::BeginTable("TableManagerTable", 2, flags, { 0., std::max(tableRowHeightRough * 3, tableHeightForWindowSize) })) {
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Title");
		ImGui::TableSetupColumn("Level");
		ImGui::TableHeadersRow();
		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
			if (sort_specs->SpecsDirty || force_resort) {
				std::sort(selectedTable->entries.begin(), selectedTable->entries.end(), [sort_specs](const Entry& left, const Entry& right) {
					for (int i = 0; i < sort_specs->SpecsCount; i++) {
						const ImGuiTableColumnSortSpecs& spec = sort_specs->Specs[i];
						if (spec.ColumnIndex == 0) {
							if (left.title != right.title)
								return spec.SortDirection == ImGuiSortDirection_Ascending ? left.title < right.title : left.title > right.title;
						     continue;
						}
						if (spec.ColumnIndex == 1) {
							bool stringMode = false;
							int leftLevel{};
							int rightLevel{};
							if (auto [ptr, ec] = std::from_chars(left.level.data(), left.level.data() + left.level.size(), leftLevel); ec != std::errc()) stringMode = true;
							if (auto [ptr, ec] = std::from_chars(right.level.data(), right.level.data() + right.level.size(), rightLevel); ec != std::errc()) stringMode = true;
							if (stringMode) {
								if (left.level != right.level)
									return spec.SortDirection == ImGuiSortDirection_Ascending ? left.level < right.level : left.level > right.level;
								continue;
							}
						    if (leftLevel != rightLevel) {
									return spec.SortDirection == ImGuiSortDirection_Ascending ? leftLevel < rightLevel : leftLevel > rightLevel;
							}
							continue;
						}
					}
					return false;
					});
				sort_specs->SpecsDirty = false;
				force_resort = false;
			}
		}
		tables_current_bottom_idx.reset();
		auto toDelete = selectedTable->entries.end();
		for (auto it = selectedTable->entries.begin(); it != selectedTable->entries.end(); it++) {
			auto& entry = *it;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(entry.title.empty() ? entry.md5.c_str() : entry.title.c_str());

			if (ImGui::IsItemVisible()) {
				tables_current_bottom_idx = it - selectedTable->entries.begin();
			}

			if (ImGui::IsItemHovered()) {
				ImGui::TableSetBgColor(ImGuiTableBgTarget(ImGuiTableBgTarget_CellBg), IM_COL32(110, 90, 20, 255));
				if (ImGui::IsMouseDoubleClicked(0)) {
					toDelete = it;
				}
			}

			ImGui::TableSetColumnIndex(1);
			std::string sRowIdx = std::to_string(ImGui::TableGetRowIndex());

			std::string levelInputId = "##level" + sRowIdx;
			ImGui::InputText(levelInputId.c_str(), &entry.level);

			if (it - selectedTable->entries.begin() == tables_move_to_this_idx) {
				ImGui::SetScrollHereY(0.f);
				tables_move_to_this_idx.reset();
			}
		}
		if (toDelete != selectedTable->entries.end()) selectedTable->entries.erase(toDelete);
		ImGui::EndTable();
	}
	if (ImGui::Button("Add Song")) {
		do_add();
		force_find = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Find") && !selectedTable->entries.empty()) {
		force_find = true;
	}

	if (force_find && !force_resort) {
		do_find();
		force_find = false;
	}

	if (ImGui::Button("Save")) {
		if (item_selected_idx == 0) {
			if (CreateTable(mNewTable.name, mNewTable.symbol)) {
				auto table = std::ranges::find(mTables, mNewTable.name, &Table::name);
				table->entries = mNewTable.entries;
				table->WriteFile();
			}
			mNewTable = Table();
			mNewTable.name = "NEW TABLE";
			name = "NEW TABLE";
		}
		else {
			selectedTable->WriteFile();
		}
	}
	ImGui::SameLine();
	static bool deleteConfirm = false;
	if (!deleteConfirm) {
		if (ImGui::Button("Delete")) deleteConfirm = true;
	}
	else {
		ImGui::Text("Are you sure?");
		ImGui::SameLine();
		if (ImGui::Button("Yes")) {
			deleteConfirm = false;
			if (item_selected_idx == 0) {
				mNewTable = Table();
				mNewTable.name = "NEW TABLE";
				name = "NEW TABLE";
			}
			else {
				RemoveTable(selectedTable->name);
				name = "NEW TABLE";
				last_item_selected_idx = item_selected_idx;
				item_selected_idx = 0;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("No")) deleteConfirm = false;
	}

	if (ImGui::Button("Reload")) {
		Reload();
		mNewTable = Table();
		mNewTable.name = "NEW TABLE";
		name = "NEW TABLE";
		last_item_selected_idx = item_selected_idx;
		item_selected_idx = 0;
	}
	ImGui::SameLine();
	if (ImGui::Button("Change Output")) {
		mOutputIdx = -1;
		mNewTable = Table();
		mNewTable.name = "NEW TABLE";
		name = "NEW TABLE";
		last_item_selected_idx = item_selected_idx;
		item_selected_idx = 0;
		config.WriteValueAndSave("sTableManagerSelectedPath", "");
	}
	ImGui::SameLine();
	if (item_selected_idx == 0)
		ImGui::Text("Current Output: '%s'", mOutputIdx < 0 ? "NONE" : mOutputPaths[mOutputIdx].string().c_str());
	else {
		ImGui::Text("Current Output: '%s'", selectedTable->path.string().c_str());
	}
}

void TableManager::Menu() {
	struct IdPopper { IdPopper(const char* id) { ImGui::PushID(id); };  ~IdPopper() { ImGui::PopID(); } } _id_popper{ "TableManager" };

	static bool showReadme = false;
	if (ImGui::Button("README")) showReadme = true;
	if (showReadme) {
		if (ImGui::Begin("Table Creator Readme", &showReadme)) {
			std::string readme =
				"Table creator allows you to create and manage table files for use in table managers like 'BeMusicSeeker' and 'LR2 OxyTabler'.\n\n"
				"You can use those local files in your table manager by importing the resulting 'header.json' in the table folder.\n\n"
				"First create and select an output folder where tables are gonna be created and loaded from. If this folder didn't exist before, it will be created. Only unique entries are allowed. To add an output path, enter it in the input box and press '+' button. Then, you can double click the created output to select it.\n\n"
				"After you selected the output, you can select the tables found in it in the 'Selected Table' dropdown menu. Valid table entry consists of a folder in the output path, containing 'header.json' file. Valid 'header.json' must include 'name' and 'data_url' elements, where 'name' must be unique among the tables in the current output path, and 'data_url' must point to the .json file with an array of charts in the table. Valid 'data.json' must include 'level' and 'md5' strings.\n\n"
				"To create a table, select 'NEW TABLE' at the top of table selector dropdown, specify its name and symbol, and press 'Save' button. If it succeeds, a new table will be available in the selector. Table won't be created if a table with same name already exists, or if the table folder with same name already exists in the output path.\n\n"
				"No changes to the table are written to its files before you press the 'Save' button.\n\n"
				"To add a song, hover over it in the LR2 song select wheel and press 'Add Song' button. It will appear in the list, and its level can be edited in the box right of its name. Song can be removed from the table with a double-click on its name.\n\n"
				"'Find' button scrolls to the next closest entry for the currently selected song.\n\n"
				"'Delete' button permanently deletes the table and its files. A confirmation is required after pressing it.\n\n"
				"'Reload' button reloads the contents of the current output path, resetting any unsaved changes to the tables.\n\n"
				"Table song list can be sorted by pressing the column header. If you want two-level sort, you can hold shift and click the second column header to sort against.\n\n"
				"Song entries in 'data.json' may only contain md5. The tool will try to load its title and other information from your song database. If such song is lacking from the database, it will have its md5 in the title column instead of the name, when no name is specified in the data.json.\n\n"
				"It can work with table files of other creation than its own. For example, you can export a table from BeMusicSeeker and manage the produced by it files.\n\n"
				"A button can be bound to open table creator menu in a separate window in the 'Binds' menu. This is the recommend method of using it.";

			ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_WordWrap;
			ImVec2 size(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() * 2 - ImGui::GetStyle().WindowPadding.y * 2);
			ImGui::InputTextMultiline("##TableCreatorReadmeText", &readme, size, flags);
			if (ImGui::Button("Close")) showReadme = false;
			ImGui::End();
		}
	}
	Gui();
}

void TableManager::ToggleMenu() {
	mMenu.ToggleOpen();
}

void TableManagerMenu::Loop() {
	ImGui::Begin("Table Creator", &mIsOpen);
	TableManager& manager = *(TableManager*)(LR2HackBox::Get().mTableManager.get());
	manager.Gui();
	ImGui::End();
}
