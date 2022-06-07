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

#include <filesystem>
#include <fstream>

#include "Updater.h"
#include "curl.h"

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

    void checkForUpdate(UIData& uiData) {
        std::vector<ResourceType> outOfDate;

        std::string result;

        //check if local version file exists, if not, all resources should be updated
        std::filesystem::path localVersionFilePath = std::filesystem::path::path();
        localVersionFilePath = localVersionFilePath / "resources" / "resource_versions.txt";
        if (!std::filesystem::is_regular_file(localVersionFilePath)) {
            flagAllResources(uiData);
            return;
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
            flagAllResources(uiData);
            return;
        }

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
                return;
            }
        }
        std::vector<std::string> remoteVersions = Engine::splitString(result, "\n");

        //compare version files and append out of date resources
        if (localVersions.size() != remoteVersions.size()) {
            flagAllResources(uiData);
            return;
        }
        
        for (int i = 0; i < TTM_RESOURCE_TYPE_COUNT; i++) {
            if (localVersions[i] != remoteVersions[i]) {
                outOfDate.push_back(static_cast<ResourceType>(i));
            }
        }

        uiData.outOfDateResources = outOfDate;
        if (outOfDate.size() > 0) {
            uiData.updateStatus = UpdateStatus::OUTDATED;
        }
        else {
            uiData.updateStatus = UpdateStatus::UPTODATE;
        }
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
        if (uiData.updateCurrentWorkspace) {
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