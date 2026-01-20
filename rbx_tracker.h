// written by rqv.x
// this is meant to be lightweight so no fancy things

// thanks to: https://github.com/YuB-W/YuB-X-Hyperion_Updateds

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <string>
#include <string_view>
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

		parse_latest_version(response, last_known_version, info);
		
		info.success = true;

		return info;
	}

private:
	static bool http_get(const wchar_t* host, const wchar_t* path, std::string& out)
	{
		HINTERNET hSession = WinHttpOpen(
			L"rbx-tracker/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0
		);

		if (!hSession) return false;

		HINTERNET hConnect = WinHttpConnect(
			hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0
		);

		if (!hConnect)
		{
			WinHttpCloseHandle(hSession);
			return false;
		}

		HINTERNET hRequest = WinHttpOpenRequest(
			hConnect,
			L"GET",
			path,
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE
		);
		if (!hRequest) {
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return false;
		}

		BOOL ok = WinHttpSendRequest(
			hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0
		);

		if (ok)
			ok = WinHttpReceiveResponse(hRequest, nullptr);

		if (!ok) {
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return false;
		}

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

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		return !out.empty();
	}

	static void parse_latest_version(const std::string& text, const std::string& last_known, rbxUpdateInfo& out)
	{
		size_t pos = text.size();

		while (pos > 0) {
			size_t start = text.rfind('\n', pos - 1);
			start = (start == std::string::npos) ? 0 : start + 1;

			std::string_view line(text.data() + start, pos - start);

			if (line.starts_with("New WindowsPlayer version-")) {
				size_t vpos = line.find("version-");

				if (vpos != std::string_view::npos) {
					size_t vend = line.find_first_not_of(
						"abcdefghijklmnopqrstuvwxyz0123456789-",
						vpos
					);

					out.latest_version = std::string(line.substr(vpos, vend - vpos));
					out.raw_line = std::string(line);
					out.has_new_version = !last_known.empty() && out.latest_version != last_known;
				}
				return;
			}

			if (start == 0)
				break;

			pos = start - 1;
		}
	}
};