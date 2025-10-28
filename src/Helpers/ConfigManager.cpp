#include "ConfigManager.hpp"

#include <fstream>
#include <sstream>
#include <charconv>
#include <filesystem>

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

template<>
void ConfigManager::WriteValue(std::string name, std::filesystem::path value) {
	WriteValue(name, value.string());
}

template <typename T>
void ConfigManager::WriteValue(std::string name, T value) {
	WriteValue(name, value);
}

template <>
void ConfigManager::WriteArray<std::string>(std::string name, const std::vector<std::string>& array) {
	std::stringstream values;
	for (auto& value : array) {
		values << value << "<NEXT>";
	}
	config[name] = values.str();
}

template <>
void ConfigManager::WriteArray<const char*>(std::string name, const std::vector<const char*>& array) {
	std::stringstream values;
	for (auto& value : array) {
		values << value << "<NEXT>";
	}
	config[name] = values.str();
}

template <>
void ConfigManager::WriteArray<bool>(std::string name, const std::vector<bool>& array) {
	std::stringstream values;
	for (const auto& value : array) {
		std::string sValue = value ? "true" : "false";
		values << sValue << "<NEXT>";
	}
	config[name] = values.str();
}

template <>
void ConfigManager::WriteArray<int>(std::string name, const std::vector<int>& array) {
	std::stringstream values;
	for (auto& value : array) {
		values << value << "<NEXT>";
	}
	config[name] = values.str();
}

template <>
void ConfigManager::WriteArray<float>(std::string name, const std::vector<float>& array) {
	std::stringstream values;
	for (auto& value : array) {
		values << value << "<NEXT>";
	}
	config[name] = values.str();
}

template <>
void ConfigManager::WriteArray<std::filesystem::path>(std::string name, const std::vector<std::filesystem::path>& array) {
	std::stringstream values;
	for (auto& value : array) {
		values << value.string() << "<NEXT>";
	}
	config[name] = values.str();
}

template <typename T>
void ConfigManager::WriteArray(std::string name, const std::vector<T>& array) {
	WriteArray(name, array);
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

template <>
std::filesystem::path ConfigManager::ReadValue(std::string name, std::filesystem::path def) {
	try {
		return config[name];
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

template <>
void ConfigManager::ReadArray<std::string>(std::string name, std::vector<std::string>& array) {
	std::string values(config[name]);
	std::string value;
	for (size_t offsetFirst = 0, offsetLast = values.find("<NEXT>"); offsetLast <= values.size() - 6; offsetLast = values.find("<NEXT>", offsetFirst)) {
		value = values.substr(offsetFirst, offsetLast - offsetFirst);
		array.push_back(value);
		offsetFirst = offsetLast + 6;
	}
}

template <>
void ConfigManager::ReadArray<bool>(std::string name, std::vector<bool>& array) {
	std::string values(config[name]);
	std::string value;
	for (size_t offsetFirst = 0, offsetLast = values.find("<NEXT>"); offsetLast <= values.size() - 6; offsetLast = values.find("<NEXT>", offsetFirst)) {
		value = values.substr(offsetFirst, offsetLast - offsetFirst);
		array.push_back(value == "true" ? true : false);
		offsetFirst = offsetLast + 6;
	}
}

template <>
void ConfigManager::ReadArray<int>(std::string name, std::vector<int>& array) {
	std::string values(config[name]);
	std::string value;
	for (size_t offsetFirst = 0, offsetLast = values.find("<NEXT>"); offsetLast <= values.size() - 6; offsetLast = values.find("<NEXT>", offsetFirst)) {
		value = values.substr(offsetFirst, offsetLast - offsetFirst);
		int iValue{};
		auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), iValue);
		if (ec == std::errc()) {
			array.push_back(iValue);
		}
		offsetFirst = offsetLast + 6;
	}
}

template <>
void ConfigManager::ReadArray<float>(std::string name, std::vector<float>& array) {
	std::string values(config[name]);
	std::string value;
	for (size_t offsetFirst = 0, offsetLast = values.find("<NEXT>"); offsetLast <= values.size() - 6; offsetLast = values.find("<NEXT>", offsetFirst)) {
		value = values.substr(offsetFirst, offsetLast - offsetFirst);
		float fValue{};
		auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), fValue);
		if (ec == std::errc()) {
			array.push_back(fValue);
		}
		offsetFirst = offsetLast + 6;
	}
}

template <>
void ConfigManager::ReadArray<std::filesystem::path>(std::string name, std::vector<std::filesystem::path>& array) {
	std::string values(config[name]);
	std::string value;
	for (size_t offsetFirst = 0, offsetLast = values.find("<NEXT>"); offsetLast <= values.size() - 6; offsetLast = values.find("<NEXT>", offsetFirst)) {
		value = values.substr(offsetFirst, offsetLast - offsetFirst);
		array.push_back(value);
		offsetFirst = offsetLast + 6;
	}
}

template <typename T>
void ConfigManager::ReadArray(std::string name, std::vector<T>& array) {
	ReadArray(name, array);
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