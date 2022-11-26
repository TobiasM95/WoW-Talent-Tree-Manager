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
#pragma warning(disable : 4996)

#include "Updater.h"

#include <chrono>
#include <thread>
#include <string>
#include <filesystem>

#include "imgui.h"
#include "miniz.h"

namespace Updater {
    typedef int(__stdcall* f_curl_global_init)(int);
    typedef void(__stdcall* f_curl_global_cleanup)();
    typedef void* (__stdcall* f_curl_easy_init)();
    typedef int(__stdcall* f_curl_easy_setopt)(void*, int, ...);
    typedef int(__stdcall* f_curl_easy_perform)(void*);
    typedef int(__stdcall* f_curl_easy_cleanup)(void*);
    constexpr int CURL_GLOBAL_DEFAULT = 3;
    constexpr int CURLOPT_URL = 10002;
    constexpr int CURLOPT_WRITEFUNCTION = 20011;
    constexpr int CURLOPT_WRITEDATA = 10001;
    constexpr int CURLOPT_VERBOSE = 41;
    constexpr int CURLOPT_NOPROGRESS = 43;
    constexpr int CURLOPT_FOLLOWLOCATION = 52;
    constexpr int CURLOPT_USERAGENT = 10018;
    constexpr int CURLOPT_XFERINFODATA = 10057;
    constexpr int CURLOPT_XFERINFOFUNCTION = 20219;
    constexpr int CURLE_OK = 0;

    void updateApplication(ThreadedUpdateStatus& updateStatus, bool& done) {

        /*
        //rename temp updater
        //extract filelist from zip and extract all files individually, overwriting original stuff
        //delete zip file
        //open gui and close itself
        //in gui wait for kill and then rename/delete temp updater (see onenote)
        */

        if (!closeAllTTMInstances(updateStatus)) {
            return;
        }

        if (!downloadTTMZip(updateStatus)) {
            return;
        }

#ifndef _DEBUG
        updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES_ERROR);
        updateStatus.setStatusString("Don't extract files while running from debug mode to not corrupt the debug bin directory!");
        updateStatus.setUpdatedFlag(true);
        return;
#endif

        if (!extractTTMZip(updateStatus)) {
            int success = rename(tempAppUpdaterName, "AppUpdater.exe");
            if (success == EACCES) {
                remove(tempAppUpdaterName);
            }
            return;
        }

        updateStatus.setUpdateStep(Updater::UpdateStep::DONE);
        updateStatus.setStatusString("Update process complete. Press the close button and restart TTM.");
        updateStatus.setUpdatedFlag(true);
    }

    bool closeAllTTMInstances(ThreadedUpdateStatus& updateStatus) {

        updateStatus.setUpdateStep(Updater::UpdateStep::CLOSE_GUI);
        updateStatus.setStatusString("Closing all WoW Talent Tree Manager Windows..");
        updateStatus.setUpdatedFlag(true);

        const auto start_time = std::chrono::steady_clock::now();
        std::vector<HWND> handles;
        EnumWindows(GetAllTTMWindowHandles, reinterpret_cast<LPARAM>(&handles));

        while (handles.size() > 0) {
            for (auto& hwnd : handles) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            const auto current = std::chrono::steady_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(current - start_time);
            if (milliseconds.count() > 5000) {
                updateStatus.setUpdateStep(Updater::UpdateStep::CLOSE_GUI_ERROR);
                updateStatus.setStatusString("Could not close all instances of opened WoW Talent Tree Manager. Manually close all windows, open a single instance and try updating again.");
                updateStatus.setUpdatedFlag(true);
                return false;
            }

            handles.clear();
            EnumWindows(GetAllTTMWindowHandles, reinterpret_cast<LPARAM>(&handles));
        }
        return true;
    }
    /*
    bool checkProcessId(char* processIDcptr, unsigned int& processID) {
        std::string pidstr{ processIDcptr };
        bool valid =  !pidstr.empty() && pidstr.find_first_not_of("0123456789") == std::string::npos;
        if (valid) {
            processID = std::stoi(pidstr);
        }
        return valid;
    }
    */

    /*
    bool pidIsRunning(DWORD pid)
    {
        HANDLE hProcess;

        //Special case for PID 0 System Idle Process
        if (pid == 0) {
            return true;
        }

        //skip testing bogus PIDs
        if (pid < 0) {
            return false;
        }

        hProcess = OpenProcess(SYNCHRONIZE, false, pid);
        DWORD ret = WaitForSingleObject(hProcess, 0);
        CloseHandle(hProcess);
        return (ret == WAIT_TIMEOUT);
    }
    */

    /*
    void getAllWindowsFromProcessID(DWORD dwProcessID, std::vector<HWND>& vhWnds)
    {
        // find all hWnds (vhWnds) associated with a process id (dwProcessID)
        HWND hCurWnd = nullptr;
        do
        {
            hCurWnd = FindWindowExW(nullptr, hCurWnd, nullptr, nullptr);
            DWORD checkProcessID = 0;
            GetWindowThreadProcessId(hCurWnd, &checkProcessID);
            if (checkProcessID == dwProcessID)
            {
                vhWnds.push_back(hCurWnd);  // add the found hCurWnd to the vector
                //wprintf(L"Found hWnd %d\n", hCurWnd);
            }
        } while (hCurWnd != nullptr);
    }
    */

    BOOL CALLBACK GetAllTTMWindowHandles(HWND hwnd, LPARAM lparam) {
        WCHAR windowTitle[30];

        GetWindowTextW(hwnd, windowTitle, 30);
        std::wstring title{ windowTitle };

        if (title == L"Talent Tree Manager") {
            std::vector<HWND>& handles = *reinterpret_cast<std::vector<HWND>*>(lparam);
            handles.push_back(hwnd);
        }
        return true;
    }


    bool downloadTTMZip(ThreadedUpdateStatus& updateStatus) 
    {
        updateStatus.setUpdateStep(Updater::UpdateStep::DOWNLOAD_TTM_ZIP);
        updateStatus.setStatusString("Fetch up to date WoW Talent Tree Manager version..");
        updateStatus.setUpdatedFlag(true);

#ifdef _DEBUG
        HMODULE libcurlDLL = LoadLibrary(L"libcurl-d.dll");
#else
        HMODULE libcurlDLL = LoadLibrary(L"libcurl.dll");
#endif
        if (!libcurlDLL) {
            updateStatus.setUpdateStep(Updater::UpdateStep::DOWNLOAD_TTM_ZIP_ERROR);
            updateStatus.setStatusString("Couldn't load libcurl.dll and zlibd1.dll. These need to be present in the directory.");
            updateStatus.setUpdatedFlag(true);
            return false;
        }
        bool dllFail = false;
        f_curl_global_init curl_global_init = (f_curl_global_init)GetProcAddress(libcurlDLL, "curl_global_init");
        if (!curl_global_init) {
            dllFail = true;
        }
        f_curl_global_cleanup curl_global_cleanup = (f_curl_global_cleanup)GetProcAddress(libcurlDLL, "curl_global_cleanup");
        if (!curl_global_cleanup) {
            dllFail = true;
        }
        f_curl_easy_init curl_easy_init = (f_curl_easy_init)GetProcAddress(libcurlDLL, "curl_easy_init");
        if (!curl_easy_init) {
            dllFail = true;
        }
        f_curl_easy_setopt curl_easy_setopt = (f_curl_easy_setopt)GetProcAddress(libcurlDLL, "curl_easy_setopt");
        if (!curl_easy_setopt) {
            dllFail = true;
        }
        f_curl_easy_perform curl_easy_perform = (f_curl_easy_perform)GetProcAddress(libcurlDLL, "curl_easy_perform");
        if (!curl_easy_perform) {
            dllFail = true;
        }
        f_curl_easy_cleanup curl_easy_cleanup = (f_curl_easy_cleanup)GetProcAddress(libcurlDLL, "curl_easy_cleanup");
        if (!curl_easy_cleanup) {
            dllFail = true;
        }
        if (dllFail) {
            updateStatus.setUpdateStep(Updater::UpdateStep::DOWNLOAD_TTM_ZIP_ERROR);
            updateStatus.setStatusString("Couldn't load libcurl.dll and zlib1.dll. These need to be present in the directory.");
            updateStatus.setUpdatedFlag(true);
            return false;
        }
        curl_global_init(CURL_GLOBAL_DEFAULT);
        void* curl;
        int res;
        std::string ttmZipString;

        curl = curl_easy_init();
        if (curl) {
            //grab download link
            std::string versionString = "";
            const char* versionUrl = "https://raw.githubusercontent.com/TobiasM95/WoW-Talent-Tree-Manager/master/GUI/resources/updatertarget/resource_versions.txt";
            curl_easy_setopt(curl, CURLOPT_URL, versionUrl);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &versionString);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &updateStatus);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

            res = curl_easy_perform(curl);
            if (CURLE_OK != res) {
                updateStatus.setUpdateStep(Updater::UpdateStep::DOWNLOAD_TTM_ZIP_ERROR);
                updateStatus.setStatusString("Couldn't fetch up to date WoW Talent Tree Manager version. Update process aborted!");
                updateStatus.setUpdatedFlag(true);
                return false;
            }
            size_t first = versionString.find(";");
            size_t second = versionString.find(";", first + 1);
            std::string zipUrl = "https://github.com/TobiasM95/WoW-Talent-Tree-Manager/releases/download/v"
                + versionString.substr(first + 1, second - first - 1) + "/TalentTreeManager.zip";
            std::string currStatus = updateStatus.getStatusString();
            updateStatus.setStatusString("Found:\n" + zipUrl + "\nDownload..");
            updateStatus.setUpdatedFlag(true);

            //grab zip
            //char zipUrl[] = "https://github.com/TobiasM95/WoW-Talent-Tree-Manager/releases/download/v1.3.7/TalentTreeManager.zip";
            curl_easy_setopt(curl, CURLOPT_URL, zipUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ttmZipString);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            res = curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            if (CURLE_OK != res) {
                updateStatus.setUpdateStep(Updater::UpdateStep::DOWNLOAD_TTM_ZIP_ERROR);
                updateStatus.setStatusString("Couldn't fetch up to date WoW Talent Tree Manager version. Update process aborted!");
                updateStatus.setUpdatedFlag(true);
                return false;
            }
        }
        curl_global_cleanup();
        FreeLibrary(libcurlDLL);
        updateStatus.setTTMZipString(ttmZipString);
        return true;
    }

    static size_t progress_callback(void* clientp,
        long long dltotal,
        long long dlnow,
        long long ultotal,
        long long ulnow)
    {
        ThreadedUpdateStatus* updateStatus = (ThreadedUpdateStatus*)clientp;

        if (dltotal <= 0.0) {
            return 0;
        }

        updateStatus->setDownloadProgress(std::pair<float, float>(dlnow / 1000000.0f, dltotal / 1000000.0f));
        updateStatus->setUpdatedFlag(true);


        return 0; /* all is good */
    }

    static size_t write_memory(void* buffer, size_t size, size_t nmemb, void* param)
    {
        std::string& text = *static_cast<std::string*>(param);
        size_t totalsize = size * nmemb;
        text.append(static_cast<char*>(buffer), totalsize);
        return totalsize;
    }

    bool extractTTMZip(ThreadedUpdateStatus& updateStatus)
    {
        updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES);
        updateStatus.setStatusString("Extract files..");
        updateStatus.setUpdatedFlag(true);

        //try to remove an temp version of the app updater, this can fail without consequences yet
        remove(tempAppUpdaterName);

        //rename temp updater
        int tries = 10;
        int success = rename("AppUpdater.exe", tempAppUpdaterName);
        while (success != 0 && tries > 0) {
            tries--;
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            success = rename("AppUpdater.exe", tempAppUpdaterName);
        }
        if (success != 0) {
            updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES_ERROR);
            updateStatus.setStatusString("Couldn't rename AppUpdater.exe and therefore update the updater. Update process aborted! (Restarting the update process is advised)");
            updateStatus.setUpdatedFlag(true);
            //remove(tempZipName);
            return false;
        }
        //extract filelist from zip and extract all files individually, overwriting original stuff
        mz_zip_archive zip_archive;
        mz_bool status;
        memset(&zip_archive, 0, sizeof(zip_archive));
        std::string zipString = updateStatus.getTTMZipString();
        size_t zipStringSize = zipString.size();
        char* zipStringBuffer = new char[zipStringSize];
        memcpy(zipStringBuffer, zipString.c_str(), zipStringSize);

        status = mz_zip_reader_init_mem(&zip_archive, zipStringBuffer, zipStringSize, 0);
        if (!status)
        {
            updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES_ERROR);
            updateStatus.setStatusString("Couldn't extract WoW Talent Tree Manager data. Update process aborted!");
            updateStatus.setUpdatedFlag(true);
            return false;
        }

        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, 0, &file_stat))
        {
            updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES_ERROR);
            updateStatus.setStatusString("Couldn't extract WoW Talent Tree Manager data. Update process aborted!");
            updateStatus.setUpdatedFlag(true);

            mz_zip_reader_end(&zip_archive);
            return false;
        }
        mz_uint numFiles = mz_zip_reader_get_num_files(&zip_archive);
        updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES_COUNT);
        updateStatus.setExtractCount({0, numFiles});
        updateStatus.setUpdatedFlag(true);
        char buffer[1024];
        std::vector<std::string> failedFiles;
        bool hasFailed = false;
        for (mz_uint i = 0; i < numFiles; i++) {
            if (numFiles > 50 && (i % (numFiles / 50) == 0)) {
                updateStatus.setExtractCount({ i, numFiles });
                updateStatus.setUpdatedFlag(true);
            }

            mz_zip_reader_get_filename(&zip_archive, i, buffer, 512);
            std::string filename{ buffer };
            filename = "autoupdatertemp/" + filename.substr(filename.find("/") + 1);
            bool isDir = mz_zip_reader_is_file_a_directory(&zip_archive, i);

            if (isDir) {
                continue;
            }
            std::filesystem::path filepath{ filename };
            //grab directory name of file 
            std::filesystem::path dirpath = filepath.parent_path();
            //create directories recursively
            if (dirpath != L"" && !std::filesystem::is_regular_file(dirpath)) {
                try {
                    std::filesystem::create_directories(dirpath);
                }
                catch (const std::filesystem::filesystem_error &e) {
                    failedFiles.push_back(filename);
                    continue;
                }
            }
            else if (std::filesystem::is_regular_file(dirpath)) {
                failedFiles.push_back(filename);
                continue;
            }
            int success = mz_zip_reader_extract_to_file(&zip_archive, i, filename.c_str(), 0);
            if (!success) {
                hasFailed = true;
                failedFiles.push_back(filename);
            }
        }
        if (hasFailed) {
            mz_zip_reader_end(&zip_archive);

            updateStatus.setUpdateStep(Updater::UpdateStep::EXTRACT_FILES_ERROR);
            updateStatus.setStatusString("Could not extract the following files:");
            updateStatus.setFailedFiles(failedFiles);
            updateStatus.setUpdatedFlag(true);

            return false;
        }
        

        mz_zip_reader_end(&zip_archive);
        delete[] zipStringBuffer;

        return true;
    }

    //status display functions
    void displayStatus(UpdateStatusCache& usc) {
        switch (usc.updateStep) {
        case UpdateStep::CLOSE_GUI: {
            displayGUICloseStatus(usc);
        }break;
        case UpdateStep::DOWNLOAD_TTM_ZIP: {
            displayTTMDownloadStatus(usc);
        }break;
        case UpdateStep::EXTRACT_FILES: {
            displayFileExtractionStatus(usc);
        }break;
        case UpdateStep::EXTRACT_FILES_COUNT: {
            displayFileExtractionCountStatus(usc);
        }break;
        case UpdateStep::OPEN_GUI: {
            displayOpenGUIStatus(usc);
        }break;
        case UpdateStep::DONE: {
            displayDoneStatus(usc);
        }break;
        case UpdateStep::EXTRACT_FILES_ERROR: {
            displayExtractFilesErrorStatus(usc);
        }break;
        case UpdateStep::CLOSE_GUI_ERROR:
        case UpdateStep::DOWNLOAD_TTM_ZIP_ERROR:
        case UpdateStep::OPEN_GUI_ERROR: {
            displayErrorStatus(usc);
        }break;
        default: break;
        }
    }

    void displayGUICloseStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::Text("%s", usc.statusString.c_str());
        ImGui::PopTextWrapPos();
    }

    void displayTTMDownloadStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::Text("%s", usc.statusString.c_str());
        ImGui::Text("%.4f MB / %.4f MB", usc.downloadProgress.first, usc.downloadProgress.second);
        ImGui::PopTextWrapPos();
    }

    void displayFileExtractionStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::Text("%s", usc.statusString.c_str());
        ImGui::PopTextWrapPos();
    }

    void displayFileExtractionCountStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::Text("%s", usc.statusString.c_str());
        ImGui::Text("Extract file %d/%d..", usc.extractCount.first, usc.extractCount.second);
        ImGui::PopTextWrapPos();
    }

    void displayOpenGUIStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::Text("%s", usc.statusString.c_str());
        ImGui::PopTextWrapPos();
    }
    
    void displayDoneStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::Text("%s", usc.statusString.c_str());
        ImGui::PopTextWrapPos();

        constexpr ImVec2 buttonSize = ImVec2(100.0f, 30.0f);
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - buttonSize.x * 0.5f, windowSize.y - buttonSize.y - 10.0f));
        if (ImGui::Button("Quit", buttonSize)) {
            usc.done = true;
        }
    }

    void displayExtractFilesErrorStatus(UpdateStatusCache& usc) {
        if (usc.failedFiles.size() == 0) {
            displayErrorStatus(usc);
            return;
        }
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "%s", usc.statusString.c_str());

        //display list
        if (ImGui::BeginListBox("##fileExtractionFailedListBox", ImVec2(ImGui::GetContentRegionAvail().x, 150)))
        {
            for (int n = 0; n < usc.failedFiles.size(); n++)
            {
                ImGui::Selectable(usc.failedFiles[n].c_str(), false);

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                //if (n == 0)
                //    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }


        ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "You can manually provide these files or try to run the updater again.");
        ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "If nothing else works you can download the complete .zip archive from github as usual.");
        ImGui::PopTextWrapPos();

        constexpr ImVec2 buttonSize = ImVec2(100.0f, 30.0f);
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - buttonSize.x * 0.5f, windowSize.y - buttonSize.y - 10.0f));
        if (ImGui::Button("Quit", buttonSize)) {
            usc.done = true;
        }
    }

    void displayErrorStatus(UpdateStatusCache& usc) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "%s", usc.statusString.c_str());
        ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "If auto updating doesn't work you can download the complete .zip archive from github as usual.");
        ImGui::PopTextWrapPos();

        constexpr ImVec2 buttonSize = ImVec2(100.0f, 30.0f);
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - buttonSize.x * 0.5f, windowSize.y - buttonSize.y - 10.0f));
        if (ImGui::Button("Quit", buttonSize)) {
            usc.done = true;
        }
    }
}