// written by rqv.x
// this is meant to be lightweight so no fancy things

// thanks to: https://github.com/YuB-W/YuB-X-Hyperion_Updateds

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <string>
#include <string_view>
#include <vector>
#include <Windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

class rbxtracker
{
public:
	struct rbxUpdateInfo
	{
		bool success = false;
		bool has_new_version = false;
		std::string latest_version;
		std::string previous_version;
		std::string raw_line;
	};

	rbxtracker() = default;
	~rbxtracker() = default;

	rbxUpdateInfo fetch_version(const std::string& last_known_version = {})
	{
		rbxUpdateInfo info;
		std::string response;

		if (!http_get(L"setup.rbxcdn.com", L"/DeployHistory.txt", response))
			return info;

		parse_history(response, last_known_version, info);
		
		info.success = true;

		return info;
	}

private:
	struct httpHandle
	{
		HINTERNET ptr;
		httpHandle(HINTERNET h) : ptr(h) {}
		~httpHandle() { if (ptr) WinHttpCloseHandle(ptr); }
		operator HINTERNET() const { return ptr; }
	};

	static bool http_get(const wchar_t* host, const wchar_t* path, std::string& out)
	{
		httpHandle hSession = WinHttpOpen(
			L"rbx-tracker/1.1",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			nullptr, nullptr, 0
		);
		
		if (!hSession) return false;

		httpHandle hConnect = WinHttpConnect(
			hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0
		);

		if (!hConnect) return false;

		httpHandle hRequest = WinHttpOpenRequest(
			hConnect,
			L"GET",
			path,
			nullptr, nullptr, nullptr,
			WINHTTP_FLAG_SECURE
		);

		if (!hRequest) return false;

		if (!WinHttpSendRequest(
			hRequest, nullptr, 0, nullptr, 0, 0, 0
		) || !WinHttpReceiveResponse(hRequest, nullptr))
			return false;

		DWORD bytes_available = 0;
		while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available) {
			std::string buffer(bytes_available, '\0');
			DWORD bytes_read = 0;

			if (!WinHttpReadData(
				hRequest,
				buffer.data(),
				bytes_available,
				&bytes_read
			) || bytes_read == 0)
				break;

			out.append(buffer.data(), bytes_read);
		}

		return !out.empty();
	}

	static void parse_history(const std::string& text, const std::string& current_local, rbxUpdateInfo& out) {
		size_t pos = text.size();
		std::vector<std::string> found_versions;

		while (pos > 0 && found_versions.size() < 2) {
			size_t start = text.rfind('\n', pos - 1);
			size_t line_start = (start == std::string::npos) ? 0 : start + 1;

			std::string_view line(text.data() + line_start, pos - line_start);

			if (line.starts_with("New WindowsPlayer version-")) {
				size_t vpos = line.find("version-");

				if (vpos != std::string_view::npos) {
					size_t vend = line.find_first_of(" ,", vpos);
					std::string v = std::string(line.substr(vpos, vend - vpos));

					if (found_versions.empty() || v != found_versions.back()) {
						found_versions.push_back(v);
					}

					if (found_versions.size() == 1) {
						out.latest_version = v;
						out.raw_line = std::string(line);
					}
					else if (found_versions.size() == 2) {
						out.previous_version = v;
					}
				}
			}

			if (line_start == 0) break;
			pos = line_start - 1;
		}

		if (!current_local.empty()) {
			out.has_new_version = (out.latest_version != current_local);
		}
	}
};