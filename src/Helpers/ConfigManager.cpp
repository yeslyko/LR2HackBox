#include "ConfigManager.hpp"

#include <fstream>
#include <sstream>

ConfigManager::ConfigManager(std::string path, bool load) {
	ConfigManager::path = path;
	
	if (load) LoadConfig();
}

template<>
void ConfigManager::WriteValue(std::string name, std::string value) {
	config[name] = value;
}

template<>
void ConfigManager::WriteValue(std::string name, const char* value) {
	WriteValue<std::string>(name, value);
}

template<>
void ConfigManager::WriteValue(std::string name, bool value) {
	WriteValue(name, value ? "true" : "false");
}

template <>
void ConfigManager::WriteValue(std::string name, int value) {
	WriteValue(name, std::to_string(value));
}

template <>
void ConfigManager::WriteValue<float>(std::string name, float value) {
	WriteValue(name, std::to_string(value));
}

template <typename T>
void ConfigManager::WriteValue(std::string name, T value) {
	WriteValue(name, value);
}

template <>
std::string ConfigManager::ReadValue(std::string name, std::string def) {
	std::string value = config[name];
	if (value.empty()) {
		WriteValueAndSave(name, def);
		return def;
	}
	return value;
}

template <>
bool ConfigManager::ReadValue(std::string name, bool def) {
	std::string value = config[name];
	if (value.empty()) {
		WriteValueAndSave(name, def);
		return def;
	}
	return value == "true" ? true : false;
}

template <>
int ConfigManager::ReadValue(std::string name, int def) {
	try {
		return std::stoi(config[name]);
	}
	catch (...) {
		WriteValueAndSave(name, def);
		return def;
	}
}

template <>
float ConfigManager::ReadValue(std::string name, float def) {
	try {
		return std::stof(config[name]);
	}
	catch (...) {
		WriteValueAndSave(name, def);
		return def;
	}
}

template <typename T>
T ConfigManager::ReadValue(std::string name, T def) {
	return ReadValue(name, def);
}

bool ConfigManager::ValueExists(std::string name) {
	return config.contains(name);
}

void ConfigManager::SaveConfig() {
	std::ofstream file(path);
	for (auto& [name, value] : config) {
		file << name << " = " << value << std::endl;
	}
}

void ConfigManager::LoadConfig() {
	std::ifstream file(path);
	std::string line;
	while (std::getline(file, line))
	{
		std::string name = line.substr(0, line.find_first_of('='));
		name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());

		std::string value = line.substr(line.find_first_of('=') + 1, line.size() - 1);
		if (*value.begin() == ' ') value.erase(value.begin());

		config[name] = value;
	}
}