#include "BaseModels/ModFeature.hpp"
#include "BaseModels/ImGuiMenu.hpp"

#include <string>
#include <vector>
#include <filesystem>

#include <json/single_include/nlohmann/json.hpp>

class TableManagerMenu : public ImGuiMenu {
public:
	void Loop();
};

class TableManager : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

	void Gui();
	bool CreateTable(std::string name, std::string symbol);
	bool RemoveTable(std::string name);
	bool RenameTable(std::string name, std::string newName);
	bool MoveTable(std::string name, std::string newName);
	void ToggleMenu();

private:
	class Entry {
	public:
		Entry() = default;
		Entry(std::string level, std::string md5, std::string title, std::string artist, nlohmann::ordered_json originalJson);
		std::string level;
		std::string md5;
		std::string title;
		std::string artist;
		nlohmann::ordered_json originalJson;
	};
	class Table {
	public:
		Table() = default;
		Table(std::string name, std::string symbol, std::filesystem::path path, std::string dataUrl, nlohmann::ordered_json originalJson);
		void AddEntry(std::string level, std::string md5, std::string title, std::string artist);
		void AddEntry(std::string level, std::string md5, std::string title, std::string artist, nlohmann::ordered_json originalJson);
		void WriteFile();

		std::string name;
		std::filesystem::path path;
		std::string symbol;
		std::filesystem::path dataPath;
		std::vector<Entry> entries;
		nlohmann::ordered_json originalJson;
	};

	void Reload();
	::TableManagerMenu mMenu;
	int mOutputIdx = -1;
	std::vector<std::filesystem::path> mOutputPaths;
	std::vector<Table> mTables;
	std::vector<std::string> mTableNames;
	Table mNewTable;
};