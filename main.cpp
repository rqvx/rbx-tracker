#include <fstream>
#include <string>
#include <cstdio>

#include "tracker_src/rbx_tracker.h"

int main()
{
	rbxtracker tracker;

	const std::string file_name = "rbx_update_info.txt";

	std::remove(file_name.c_str());

	std::string last_known_version;
	auto info = tracker.fetch_version(last_known_version);

	std::ofstream out(file_name, std::ios::out | std::ios::trunc);
	if (!out.is_open())
		return 1;

	if (!info.success)
		return 1;

	out << "[+] Updated: " << (info.has_new_version ? "Yes" : "No") << "\n";
	out << "[+] Latest Version: " << info.latest_version << "\n";
	out << "[+] Last Known Version: " << last_known_version << "\n";
	out << "[+] Raw Line: " << info.raw_line << "\n";

	out.close();

	std::string command = "explorer /select,\"" + file_name + "\"";
	system(command.c_str());

	return 0;
}