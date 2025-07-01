#include "Version.hpp"

#include <cpr/cpr.h>
#include <json/single_include/nlohmann/json.hpp>
#include <PicoSHA2/picosha2.h>
#include <iostream>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

static std::string hashToStr(std::array<unsigned char, picosha2::k_digest_size>& hash) {
    std::ostringstream os;
    os.setf(std::ios::hex, std::ios::basefield);
    for (auto& it : hash) {
        os.width(2);
        os.fill('0');
        os << static_cast<unsigned int>(it);
    }
    os.setf(std::ios::dec, std::ios::basefield);
    return os.str();
}

Version::Status Version::Check() {
    cpr::Response r = cpr::Get(cpr::Url{ "https://api.github.com/repos/MatVeiQaaa/LR2HackBox/releases" });
    Version::Status status;

    if (r.status_code != 200) return status;
    nlohmann::json json = nlohmann::json::parse(r.text);
    std::string shaRemote;
    for (auto& release : json) {
        if (release["tag_name"] != "latest") continue;
        for (auto& asset : release["assets"]) {
            if (asset["name"] != "azop.dll") continue;
            shaRemote = asset["digest"];
            break;
        }
        break;
    }
    if (shaRemote.empty()) return status;
    shaRemote = shaRemote.substr(7);
    std::ifstream f("azop.dll", std::ios::binary);
    std::array<unsigned char, picosha2::k_digest_size> shaLocalBin;
    picosha2::hash256(f, shaLocalBin.begin(), shaLocalBin.end());
    std::string shaLocal = hashToStr(shaLocalBin);
    status.success = true;
    status.isLast = shaLocal == shaRemote;
    return status;
}