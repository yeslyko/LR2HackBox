#pragma once

#include <string>
#include <map>

class ConfigManager {
public:
	ConfigManager(std::string path, bool load = true);
	template <typename T> void WriteValue(std::string name, T value);
	template <typename T> void WriteValueAndSave(std::string name, T value) {
		WriteValue(name, value);
		SaveConfig();
	}
	template <typename T> T ReadValue(std::string name, T def);
	bool ValueExists(std::string name);

	void SaveConfig();
	void LoadConfig();
private:
	std::string path;
	std::map<std::string, std::string> config;
};