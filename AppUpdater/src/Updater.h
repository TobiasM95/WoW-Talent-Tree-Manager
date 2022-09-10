#pragma once

#include <Windows.h>
#include <mutex>
#include <atomic>
#include <vector>

namespace Updater {
	constexpr char tempZipName[] = "ttm_temp.zip";
	//if this gets changed, check Main.cpp of GUI project as well
	constexpr char tempAppUpdaterName[] = "AppUpdaterTemp.exe";

	enum class UpdateStep {
		CLOSE_GUI = 0,
		DOWNLOAD_TTM_ZIP = 1,
		EXTRACT_FILES = 2,
		EXTRACT_FILES_COUNT = 3,
		OPEN_GUI = 4,

		CLOSE_GUI_ERROR = 10,
		DOWNLOAD_TTM_ZIP_ERROR = 11,
		EXTRACT_FILES_ERROR = 12,
		OPEN_GUI_ERROR = 14
	};

	class ThreadedUpdateStatus
	{
	private:
		UpdateStep updateStep = UpdateStep::CLOSE_GUI;
		std::string statusString = "";
		std::pair<unsigned, unsigned> extractCount = { 0,0 };
		std::vector<std::string> failedFiles;

		mutable std::mutex m;
		std::atomic<bool> hasUpdated = false;

		//don't cache this for the GUI, this is internal
		std::string ttmZipString = "";
	public:
		ThreadedUpdateStatus(){};
		ThreadedUpdateStatus(const ThreadedUpdateStatus&) = delete;
		ThreadedUpdateStatus& operator=(const ThreadedUpdateStatus&) = delete;

		void setUpdatedFlag(bool state) {
			hasUpdated.store(state, std::memory_order_release);
		}

		bool getUpdatedFlag() {
			return hasUpdated.load(std::memory_order_acquire);
		}

		void setStatusString(std::string newStatusString) {
			std::lock_guard<std::mutex> lock(m);
			statusString = statusString + newStatusString + "\n";
		}

		std::string getStatusString() const {
			std::lock_guard<std::mutex> lock(m);
			return statusString;
		}

		void setUpdateStep(UpdateStep us) {
			std::lock_guard<std::mutex> lock(m);
			updateStep = us;
		}
		
		UpdateStep getUpdateStep() const {
			std::lock_guard<std::mutex> lock(m);
			return updateStep;
		}

		void setExtractCount(std::pair<unsigned, unsigned> ec) {
			std::lock_guard<std::mutex> lock(m);
			extractCount = ec;
		}

		std::pair<unsigned, unsigned> getExtractCount() const {
			std::lock_guard<std::mutex> lock(m);
			return extractCount;
		}

		void setFailedFiles(std::vector<std::string> ff) {
			std::lock_guard<std::mutex> lock(m);
			failedFiles = ff;
		}

		std::vector<std::string> getFailedFiles() const {
			std::lock_guard<std::mutex> lock(m);
			return std::vector<std::string>(failedFiles);
		}

		void setTTMZipString(std::string s) {
			std::lock_guard<std::mutex> lock(m);
			ttmZipString = s;
		}

		std::string getTTMZipString() const {
			std::lock_guard<std::mutex> lock(m);
			return ttmZipString;
		}
	};

	struct UpdateStatusCache
	{
		bool done = false;

		UpdateStep updateStep = UpdateStep::CLOSE_GUI;
		std::string statusString = "";
		std::pair<unsigned, unsigned> extractCount;
		std::vector<std::string> failedFiles;

		void cacheUpdateStatus(const ThreadedUpdateStatus& tus) {
			statusString = tus.getStatusString();
			updateStep = tus.getUpdateStep();
			extractCount = tus.getExtractCount();
			failedFiles = tus.getFailedFiles();
		}
	};

	void updateApplication(ThreadedUpdateStatus& updateStatus, bool& done);
	//bool checkProcessId(char* processIDcptr, unsigned int& processID);
	//bool pidIsRunning(DWORD pid);
	//void getAllWindowsFromProcessID(DWORD dwProcessID, std::vector<HWND>& vhWnds);
	bool closeAllTTMInstances(ThreadedUpdateStatus& updateStatus);
	BOOL CALLBACK GetAllTTMWindowHandles(HWND hwnd, LPARAM lparam);

	bool downloadTTMZip(ThreadedUpdateStatus& updateStatus);
	static size_t write_memory(void* buffer, size_t size, size_t nmemb, void* param);

	bool extractTTMZip(ThreadedUpdateStatus& updateStatus);
	
	//information display functions
	void displayStatus(UpdateStatusCache& usc);
	void displayGUICloseStatus(UpdateStatusCache& usc);
	void displayTTMDownloadStatus(UpdateStatusCache& usc);
	void displayFileExtractionStatus(UpdateStatusCache& usc);
	void displayFileExtractionCountStatus(UpdateStatusCache& usc);
	void displayOpenGUIStatus(UpdateStatusCache& usc);
	void displayExtractFilesErrorStatus(UpdateStatusCache& usc);
	void displayErrorStatus(UpdateStatusCache& usc);
}