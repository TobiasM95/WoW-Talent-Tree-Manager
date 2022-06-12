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
//#define DEBUGREMOTE
#pragma warning(disable : 4996)

#include "Updater.h"

#include <filesystem>
#include <fstream>

#include "curl.h"

#include "TTMEnginePresets.h"
#include "TTMGUIPresetsInternal.h"

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
        uiData.outOfDateResources.push_back(ResourceType::ICONS);
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
#ifdef DEBUGREMOTE
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/resource_versions.txt");
#else
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/resource_versions.txt");
#endif
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            //TTMTODO: this seems very very very dangerous
            if (CURLE_OK != res || result.find("404:") != std::string::npos) {
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
            std::string remotePresetVersion = Engine::splitString(remoteVersions[static_cast<int>(ResourceType::PRESET)], ";")[1];
            if (compareVersions(Presets::TTM_VERSION, remotePresetVersion) < 0) {
                additionalUpdateMessage += "Preset version is higher than TTM version. If you update, presets might stop working. Presets in current workspace will be transformed to custom trees. Please update TTM before updating presets.\n";
                uiData.presetToCustomOverride = true;
            }
            flagAllResources(uiData);
            return additionalUpdateMessage + "Couldn't read Resource file. Please update resources after updating TTM!";
        }

        //compare version files and append out of date resources
        if (localVersions.size() != remoteVersions.size()) {
            std::string remotePresetVersion = Engine::splitString(remoteVersions[static_cast<int>(ResourceType::PRESET)], ";")[1];
            if (compareVersions(Presets::TTM_VERSION, remotePresetVersion) < 0) {
                additionalUpdateMessage += "Preset version is higher than TTM version. If you update, presets might stop working. Presets in current workspace will be transformed to custom trees. Please update TTM before updating presets.\n";
                uiData.presetToCustomOverride = true;
            }
            flagAllResources(uiData);
            return additionalUpdateMessage + "New resources added. Please update resources after updating TTM!";
        }
        
        for (int i = 0; i < TTM_RESOURCE_TYPE_COUNT; i++) {
            if (localVersions[i] != remoteVersions[i]) {
                if (static_cast<ResourceType>(i) == ResourceType::PRESET) {
                    std::string remotePresetVersion = Engine::splitString(remoteVersions[i], ";")[1];
                    if (compareVersions(Presets::TTM_VERSION, remotePresetVersion) < 0) {
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
            case ResourceType::ICONS: {
                updateIcons(uiData);
            }break;
            }
        }

        //after updating resources, update the versions file

        std::string result;
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
#ifdef DEBUGREMOTE
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/resource_versions.txt");
#else
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/resource_versions.txt");
#endif
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            //TTMTODO: this seems very very very dangerous
            if (CURLE_OK != res || result.find("404:") != std::string::npos) {
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
#ifdef DEBUGREMOTE
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/Engine/resources/presets.txt");
#else
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/presets.txt");
#endif
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            //TTMTODO: this seems very very very dangerous
            if (CURLE_OK != res || result.find("404:") != std::string::npos) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                return;
            }
        }
        std::filesystem::path localVersionFilePath = std::filesystem::path::path();
        localVersionFilePath = localVersionFilePath / "resources" / "presets.txt";
        std::ofstream localVersionFile(localVersionFilePath);
        localVersionFile << result;
    }

    void updateIcons(UIData& uiData) {
        std::string metaDataRaw;
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
#ifdef DEBUGREMOTE
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/icons/icons_packed_meta.txt");
#else
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/icons_packed_meta.txt");
#endif
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &metaDataRaw);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            //TTMTODO: this seems very very very dangerous
            if (CURLE_OK != res || metaDataRaw.find("404:") != std::string::npos) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                return;
            }
        }
        std::vector<std::string> metaData = Engine::splitString(metaDataRaw, "\n");

        std::string pathToPackedIcons("./resources/icons/icons_packed.png");
        if (!std::filesystem::is_directory(std::filesystem::path(pathToPackedIcons).parent_path())) {
            std::filesystem::create_directory(std::filesystem::path(pathToPackedIcons).parent_path());
        }
        FILE* fp = nullptr;
        curl = curl_easy_init();
        if (curl) {
            fp = fopen(pathToPackedIcons.c_str(), "wb");
            if (fp == NULL) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                return;
            }
#ifdef DEBUGREMOTE
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/icons/icons_packed.png");
#else
            curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/icons_packed.png");
#endif
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            //TTMTODO: this seems very very very dangerous
            if (CURLE_OK != res || metaDataRaw.find("404:") != std::string::npos) {
                uiData.updateStatus = UpdateStatus::UPDATEERROR;
                return;
            }
            fclose(fp);
        }


        if (metaData.size() < 4) {
            uiData.updateStatus = UpdateStatus::UPDATEERROR;
            return;
        }

        if (!unpackIcons(pathToPackedIcons.c_str(), metaData)) {
            std::filesystem::remove(pathToPackedIcons);
            uiData.updateStatus = UpdateStatus::UPDATEERROR;
            return;
        }

        //we should also delete the temporary packed file!
        std::filesystem::remove(pathToPackedIcons);
        refreshIconList(uiData);
    }

    void testUpdateIcons(UIData& uiData) {
        std::vector<std::string> metaData;

        //Not a real thing in the live updater
        //this should read the metaData from remote
        std::filesystem::path pathToUpdaterTarget("../../../GUI/resources/updatertarget");
        std::ifstream metaDataFile;
        metaDataFile.open(pathToUpdaterTarget / "icons_packed_meta.txt");
        if (metaDataFile.is_open()) {
            std::string line;
            while (std::getline(metaDataFile, line)) {
                if (line != "") {
                    metaData.push_back(line);
                }
            }
        }
        metaDataFile.close();
        //this should download a packed icon file to icon directory and return the path
        std::string pathToPackedIcons((pathToUpdaterTarget / "icons_packed.png").string());
        //end
        if (metaData.size() < 4) {
            uiData.updateStatus = UpdateStatus::UPDATEERROR;
            return;
        }

        if (!unpackIcons(pathToPackedIcons.c_str(), metaData)) {
            uiData.updateStatus = UpdateStatus::UPDATEERROR;
            return;
        }

        //we should also delete the temporary packed file!
        //std::filesystem::remove(pathToPackedIcons);
    }

    std::string exportToPastebin(std::string treeRep, std::string treeName) {
        std::string api_dev_key = "BCmGxHU-akWMjGYiXm5yL5An0aclmzOC";
        std::string api_paste_code = treeRep;
        std::string api_paste_name = treeName; // name or title of your paste
        std::string api_user_key = ""; // if an invalid or expired api_user_key is used, an error will spawn. If no api_user_key is used, a guest paste will be created

        CURL* curl;
        CURLcode res;
        std::string resultURL;

        curl = curl_easy_init();
        char* api_paste_name_enc = curl_easy_escape(curl, api_paste_name.c_str(), static_cast<int>(api_paste_name.length()));
        char* api_paste_code_enc = curl_easy_escape(curl, api_paste_code.c_str(), static_cast<int>(api_paste_code.length()));

        if (curl) {
            std::string postfield =
                (std::string)"api_option=paste" +
                "&api_dev_key=" + Presets::PASTEBIN_API_DEV_KEY + 
                "&api_paste_private=1" + // 0=public 1=unlisted 2=private
                "&api_paste_name=" + std::string(api_paste_name_enc) +
                "&api_paste_expire_date=10M" + //10M = 10 minutes
                "&api_paste_code=" + std::string(api_paste_code_enc);
            curl_easy_setopt(curl, CURLOPT_POST, true);
            curl_easy_setopt(curl, CURLOPT_URL, "https://pastebin.com/api/api_post.php");
            curl_easy_setopt(
                curl, 
                CURLOPT_POSTFIELDS, 
                postfield.c_str()
            );
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultURL);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (CURLE_OK != res) {
                return "Error: " + std::string(curl_easy_strerror(res));
            }
        }

        curl_free(api_paste_name_enc);
        curl_free(api_paste_code_enc);

        return resultURL;
    }
}