#include <fstream>
#include <string>
#include <filesystem>
#include <Windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <thread>
#include <chrono>

#include "rbx_tracker.h"

std::string get_appdata()
{
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) == S_OK)
	{
		std::filesystem::path rbxtpath = std::filesystem::path(path) / "rbxtracker";

		if (!std::filesystem::exists(rbxtpath))
		{
			std::filesystem::create_directories(rbxtpath);
		}

		return rbxtpath.string();
	}

	return "";
}

int main()
{
	rbxtracker tracker;

	std::string basepath = get_appdata();
	if (basepath.empty())
	{
		printf("[!]: Couldn't resolve AppData path!\n");
		return 1;
	}

	std::string version_output = basepath + "\\version_report.txt";
	std::string cache_output = basepath + "\\version_cache.txt";

	std::string last_seen;
	std::ifstream cache_in(cache_output);
	std::getline(cache_in, last_seen);
	cache_in.close();

	printf("[+] Fetching DeployHistory from Roblox CDN");
	auto info = tracker.fetch_version(last_seen);

	if (info.success)
	{
		std::ofstream out(version_output);

		out << "rbxtracker - we chillin trust" << "\n";
		out << "-------------------------------------------------------------------------" << "\n";
		out << "[+]	Latest Version: " << info.latest_version << "\n";
		out << "[+]	Previous Version: " << info.previous_version << "\n";
		out << "[+]	Updated?: " << (info.has_new_version ? "YES" : "NO") << "\n";
		out << "-------------------------------------------------------------------------" << "\n";
		out << "Saved at: " << basepath << "\n";
		out << "Raw: " << info.raw_line << "\n";

		out.close();

		if (info.has_new_version || last_seen.empty())
		{
			std::ofstream c_out(cache_output);
			
			c_out << info.latest_version << "\n";
			c_out.close();

			printf("[!] New version cached: %s\n", info.latest_version.c_str());
		}

		ShellExecuteA(NULL, "open", "notepad.exe", version_output.c_str(), NULL, SW_SHOWNORMAL);
	}
	else
	{
		printf("[!] rbxtracker failed to fetch data\n");
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	return 0;
}