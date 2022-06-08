/*
    WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
    Copyright(C) 2022 Tobias Mielich

    This program is free software : you can redistribute it and /or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see < https://www.gnu.org/licenses/>.

    Contact via https://github.com/TobiasM95/WoW-Talent-Tree-Manager/discussions or BuffMePls#2973 on Discord
*/

#include "Updater.h"

#include <filesystem>
#include <fstream>

#include "curl.h"

#include "TTMEnginePresets.h"

namespace TTM {
    static size_t write_memory(void* buffer, size_t size, size_t nmemb, void* param)
    {
        std::string& text = *static_cast<std::string*>(param);
        size_t totalsize = size * nmemb;
        text.append(static_cast<char*>(buffer), totalsize);
        return totalsize;
    }

    inline void flagAllResources(UIData& uiData) {
        uiData.updateStatus = UpdateStatus::OUTDATED;
        uiData.outOfDateResources.push_back(ResourceType::PRESET);
    }

    std::string checkForUpdate(UIData& uiData) {
        std::string additionalUpdateMessage;

        std::vector<ResourceType> outOfDate;

        std::string result;

        //fetch version file from server
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/resource_versions.txt");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (CURLE_OK != res) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                uiData.outOfDateResources.clear();
                return "Failed to fetch remote resource file.";
            }
        }
        std::vector<std::string> remoteVersions = Engine::splitString(result, "\n");

        //check if local version file exists, if not, all resources should be updated
        std::filesystem::path localVersionFilePath = std::filesystem::path::path();
        localVersionFilePath = localVersionFilePath / "resources" / "resource_versions.txt";
        if (!std::filesystem::is_regular_file(localVersionFilePath)) {
            flagAllResources(uiData);
            return "No resource file found. Please update resources after updating TTM!";
        }

        //read contents from local version file
        std::vector<std::string> localVersions;
        try {
            std::ifstream versionFile(localVersionFilePath);
            std::string line;
            while (std::getline(versionFile, line)) {
                localVersions.push_back(line);
            }
        }
        catch (std::ifstream::failure& e) {
            if (compareVersions(Presets::TTM_VERSION, remoteVersions[static_cast<int>(ResourceType::PRESET)]) < 0) {
                additionalUpdateMessage += "Preset version is higher than TTM version. If you update, presets might stop working. Presets in current workspace will be transformed to custom trees. Please update TTM before updating presets.\n";
                uiData.presetToCustomOverride = true;
            }
            flagAllResources(uiData);
            return additionalUpdateMessage + "Couldn't read Resource file. Please update resources after updating TTM!";
        }

        //compare version files and append out of date resources
        if (localVersions.size() != remoteVersions.size()) {
            if (compareVersions(Presets::TTM_VERSION, remoteVersions[static_cast<int>(ResourceType::PRESET)]) < 0) {
                additionalUpdateMessage += "Preset version is higher than TTM version. If you update, presets might stop working. Presets in current workspace will be transformed to custom trees. Please update TTM before updating presets.\n";
                uiData.presetToCustomOverride = true;
            }
            flagAllResources(uiData);
            return additionalUpdateMessage + "New resources added. Please update resources after updating TTM!";
        }
        
        for (int i = 0; i < TTM_RESOURCE_TYPE_COUNT; i++) {
            if (localVersions[i] != remoteVersions[i]) {
                if (static_cast<ResourceType>(i) == ResourceType::PRESET) {
                    if (compareVersions(Presets::TTM_VERSION, remoteVersions[i]) < 0) {
                        additionalUpdateMessage += "Preset version is higher than TTM version. If you update, presets might stop working. Presets in current workspace will be transformed to custom trees. Please update TTM before updating presets.\n";
                        uiData.presetToCustomOverride = true;
                    }
                }
                outOfDate.push_back(static_cast<ResourceType>(i));
            }
        }

        uiData.outOfDateResources = outOfDate;
        if (outOfDate.size() > 0) {
            uiData.updateStatus = UpdateStatus::OUTDATED;
            return additionalUpdateMessage + "Resources outdated.";
        }
        else {
            uiData.updateStatus = UpdateStatus::UPTODATE;
            return "Resources up to date.";
        }

    }

    int compareVersions(std::string clientVersion, std::string remoteVersion) {
        //versions are always of the format ##..## . ##..## . ##..##
        std::vector<std::string> clientVersionParts = Engine::splitString(clientVersion, ".");
        std::vector<std::string> remoteVersionParts = Engine::splitString(remoteVersion, ".");
        for (int i = 0; i < 3; i++) {
            if (std::stoi(clientVersionParts[i]) < std::stoi(remoteVersionParts[i])) {
                return -1;
            }
            else if (std::stoi(clientVersionParts[i]) > std::stoi(remoteVersionParts[i])) {
                return 1;
            }
        }
        return 0;
    }

    void updateResources(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        if (uiData.outOfDateResources.size() == 0) {
            return;
        }
        for (auto& type : uiData.outOfDateResources) {
            switch (type) {
            case ResourceType::PRESET: {
                updatePresets(uiData);
            }break;
            }
        }

        //after updating resources, update the versions file

        std::string result;
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/resource_versions.txt");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (CURLE_OK != res) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                return;
            }
        }
        std::filesystem::path localVersionFilePath = std::filesystem::path::path();
        localVersionFilePath = localVersionFilePath / "resources" / "resource_versions.txt";
        std::ofstream localVersionFile(localVersionFilePath);
        localVersionFile << result;
        uiData.updateStatus = UpdateStatus::UPTODATE;

        talentTreeCollection.presets = Presets::LOAD_PRESETS();
        if (uiData.updateCurrentWorkspace && !uiData.presetToCustomOverride) {
            for (auto& tree : talentTreeCollection.trees) {
                if (tree.tree.presetName != "custom" && talentTreeCollection.presets.count(tree.tree.presetName)) {
                    tree.tree = Engine::restorePreset(tree.tree, talentTreeCollection.presets[tree.tree.presetName]);
                }
            }
        }
        else {
            for (auto& tree : talentTreeCollection.trees) {
                if (tree.tree.presetName != "custom") {
                    tree.tree.presetName = "custom";
                }
            }
        }
    }

    void updatePresets(UIData& uiData) {
        std::string result;
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/Engine/resources/presets.txt");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (CURLE_OK != res) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                return;
            }
        }
        std::filesystem::path localVersionFilePath = std::filesystem::path::path();
        localVersionFilePath = localVersionFilePath / "resources" / "presets.txt";
        std::ofstream localVersionFile(localVersionFilePath);
        localVersionFile << result;
    }
}