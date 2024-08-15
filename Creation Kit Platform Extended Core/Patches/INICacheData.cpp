﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "Core/Engine.h"
#include "INICacheData.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		struct string_equal_to
		{
			inline bool operator()(const String& lhs, const String& rhs) const
			{
				return !_stricmp(lhs.c_str(), rhs.c_str());
			}
		};

		UnorderedMap<String, mINI::INIStructure*, std::hash<String>, string_equal_to> GlobalINICache;

		INICacheDataPatch::INICacheDataPatch() : Module(GlobalEnginePtr)
		{}

		INICacheDataPatch::~INICacheDataPatch()
		{
			ClearAndFlush();
		}

		bool INICacheDataPatch::HasOption() const
		{
			return true;
		}

		bool INICacheDataPatch::HasCanRuntimeDisabled() const
		{
			return false;
		}

		const char* INICacheDataPatch::GetOptionName() const
		{
			return "CreationKit:bINICache";
		}

		const char* INICacheDataPatch::GetName() const
		{
			return "INI Cache Data";
		}

		bool INICacheDataPatch::HasDependencies() const
		{
			return false;
		}

		Array<String> INICacheDataPatch::GetDependencies() const
		{
			return {};
		}

		void INICacheDataPatch::ClearAndFlush()
		{
			for (auto it = GlobalINICache.begin(); it != GlobalINICache.end(); it++)
			{
				if (it->second)
				{
					mINI::INIFile file(it->first.c_str());
					file.write(*(it->second));
					delete it->second;
					it->second = nullptr;
				}
			}

			GlobalINICache.clear();
		}

		UINT INICacheDataPatch::HKGetPrivateProfileIntA(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName)
		{
			if (!lpKeyName || !lpAppName)
				return (UINT)nDefault;

			auto fileName = GetAbsoluteFileName(lpFileName);
			auto ini_data = (mINI::INIStructure*)GetFileFromCacheOrOpen(fileName);
			if (!ini_data)
				return GetPrivateProfileIntA(lpAppName, lpKeyName, nDefault, lpFileName);

			String s;
			auto ip = ini_data->get(lpAppName);
			if (!ip.has(lpKeyName))
				return (UINT)nDefault;
			else
				s = ip.get(lpKeyName);

			static const char* whitespace_delimiters = " \t\n\r\f\v";
			s.erase(s.find_last_not_of(whitespace_delimiters) + 1);
			s.erase(0, s.find_first_not_of(whitespace_delimiters));

			if (s[0] == '"') s.erase(0, 1);
			if (s[s.length() - 1] == '"') s.resize(s.length() - 1);

			char* end_ptr = nullptr;

			if (s.find_first_of("0x") == 0)
				// hex
				return strtoul(s.c_str() + 2, &end_ptr, 16);
			else
				// dec
				return strtoul(s.c_str(), &end_ptr, 10);
		}

		DWORD INICacheDataPatch::HKGetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault,
			LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
		{
			if (!lpReturnedString || !nSize)
				return 0;

			auto fileName = GetAbsoluteFileName(lpFileName);
			auto ini_data = (mINI::INIStructure*)GetFileFromCacheOrOpen(fileName);
			if (!ini_data)
				return GetPrivateProfileStringA(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);

			String s;
			size_t l = 0;

			if (lpAppName && !lpKeyName)
			{
				auto ip = ini_data->get(lpAppName);

				for (auto i = ip.begin(); i != ip.end(); i++)
				{
					s.append(i->first).append("=").append(i->second).append("\n");
					l = std::min((size_t)nSize, s.length());
					if (l == nSize) break;
				}

				memcpy(lpReturnedString, (const void*)s.c_str(), l);
			}
			else if (!lpAppName)
			{
				for (auto j = ini_data->begin(); j != ini_data->end(); j++)
				{
					s.append("[").append(j->first).append("]\n");

					for (auto i = j->second.begin(); i != j->second.end(); i++)
					{
						s.append(i->first).append("=").append(i->second).append("\n");
						l = std::min((size_t)nSize, s.length());
						if (l == nSize) break;
					}
				}

				memcpy(lpReturnedString, (const void*)s.c_str(), l);
			}
			else
			{
				auto ip = ini_data->get(lpAppName);
				if (!ip.has(lpKeyName))
					s = lpDefault ? lpDefault : "";
				else
					s = ip.get(lpKeyName);

				static const char* whitespace_delimiters = " \t\n\r\f\v";
				s.erase(s.find_last_not_of(whitespace_delimiters) + 1);
				s.erase(0, s.find_first_not_of(whitespace_delimiters));

				if (s[0] == '"') s.erase(0, 1);
				if (s[s.length() - 1] == '"') s.resize(s.length() - 1);

				l = std::min((size_t)nSize, s.length());
				memcpy(lpReturnedString, (const void*)s.c_str(), l);
			}

			lpReturnedString[(l == nSize) ? l - 1 : l] = '\0';
			return (DWORD)l;
		}

		BOOL INICacheDataPatch::HKGetPrivateProfileStructA(LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct,
			UINT uSizeStruct, LPCSTR szFile)
		{
			if (!lpszSection || !szFile || (uSizeStruct >= (UINT)0x7FFFFFFA)) return false;

			auto fileName = GetAbsoluteFileName(szFile);
			auto ini_data = (mINI::INIStructure*)GetFileFromCacheOrOpen(fileName);
			if (!ini_data) 
				return GetPrivateProfileStructA(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile);

			if (!(*ini_data).has(lpszSection))
				return false;
			if (!(*ini_data)[lpszSection].has(lpszKey))
				return false;

			std::string& value_str = (*ini_data)[lpszSection][lpszKey];
			UINT count = (uSizeStruct << 1) + 2;

			if (value_str.length() != count)
				return false;
			else
			{
				DWORD dec_val[2] = { 0 }, temp = 0;
				count = uSizeStruct;
				auto data = value_str.c_str();

				do
				{
					dec_val[0] = (DWORD)data[0];
					temp = dec_val[0] - 0x30;
					if (temp > 9) dec_val[0] -= 7;
					dec_val[1] = (DWORD)data[1];
					temp = dec_val[1] - 0x30;
					if (temp > 9)
					{
						dec_val[1] -= 7;
						dec_val[1] &= 0xF;
					}
					else dec_val[1] -= 0x30;
					((PBYTE)lpStruct)[uSizeStruct - count] = (BYTE)((dec_val[0] << 4) | dec_val[1]);
					data += 2;
					count--;
				} while (count);
			}

			return true;
		}

		BOOL INICacheDataPatch::HKWritePrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName)
		{
			if (!lpAppName || !lpFileName) return false;

			auto fileName = GetAbsoluteFileName(lpFileName);
			auto ini_data = (mINI::INIStructure*)GetFileFromCacheOrOpen(fileName);
			if (!ini_data) return false;

			if (!lpKeyName)
				// The name of the key to be associated with a string.
				// If the key does not exist in the specified section, it is created.
				// If this parameter is NULL, the entire section, including all entries within the section, is deleted.
				ini_data->remove(lpAppName);
			else if (!lpString)
				// A null - terminated string to be written to the file.
				// If this parameter is NULL, the key pointed to by the key_name parameter is deleted.
				(*ini_data)[lpAppName].remove(lpKeyName);
			else
				(*ini_data)[lpAppName][lpKeyName] = lpString;

			return true;
			//mINI::INIFile file(fileName);
			//return file.write(*ini_data);
		}

		BOOL INICacheDataPatch::HKWritePrivateProfileStructA(LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct,
			UINT uSizeStruct, LPCSTR szFile)
		{
			if (!lpStruct)
				return HKWritePrivateProfileStringA(lpszSection, lpszKey, nullptr, szFile);

			if (!lpszSection || !szFile || (uSizeStruct >= (UINT)0x7FFFFFFA)) return false;

			auto fileName = GetAbsoluteFileName(szFile);
			auto ini_data = (mINI::INIStructure*)GetFileFromCacheOrOpen(fileName);
			if (!ini_data) return false;

			static const char* ffmt_value = "0123456789ABCDEF\\";
			
			DWORD dec_val[2] = { 0 }, temp = 0, unk = 0;
			UINT count = uSizeStruct;

			std::string value_str;
			value_str.resize(((size_t)count << 1) + 2);
			auto data = value_str.data();

			if (count)
			{
				do
				{
					temp = ((PBYTE)lpStruct)[uSizeStruct - count];
					unk = (BYTE)(temp + unk);
					dec_val[0] = temp >> 4;
					dec_val[1] = temp & 0xF;

					data[0] = ffmt_value[dec_val[0]];
					data[1] = ffmt_value[dec_val[1]];

					data += 2;
					count--;
				} while (count);
			}

			dec_val[0] = unk >> 4;
			dec_val[1] = unk & 0xF;
			data[0] = ffmt_value[dec_val[0]];
			data[1] = ffmt_value[dec_val[1]];

			(*ini_data)[lpszSection][lpszKey] = value_str;

			return true;

			//mINI::INIFile file(fileName);
			//return file.write(*ini_data);
		}

		std::string INICacheDataPatch::GetAbsoluteFileName(LPCSTR lpFileName)
		{
			if (!lpFileName) return "";
			if (PathIsRelativeA(lpFileName))
			{
				CHAR szBuffer[MAX_PATH];
				std::string sname = lpFileName;
				if (!sname.find_first_of(".\\") || !sname.find_first_of("./"))
				{
					
					GetCurrentDirectoryA(MAX_PATH, szBuffer);
					sname = szBuffer;
					sname.append(++lpFileName);
				}
				else
				{
					if (GetWindowsDirectoryA(szBuffer, MAX_PATH) > 0)
					{
						sname = szBuffer;
						sname.append(++lpFileName);
					}
				}
				return sname;
			}
			return lpFileName;
		}

		HANDLE INICacheDataPatch::GetFileFromCacheOrOpen(const std::string& sFileName)
		{
			if (sFileName.empty()) return nullptr;

			mINI::INIStructure* ini_data = nullptr;

			auto iterator_find = GlobalINICache.find(sFileName.c_str());
			if (iterator_find != GlobalINICache.end())
				ini_data = iterator_find->second;

			if (!ini_data)
			{
				mINI::INIFile* file = new mINI::INIFile(sFileName);
				if (!file) return nullptr;

				ini_data = new mINI::INIStructure();
				if (!ini_data) return nullptr;
				file->read(*ini_data, false);

				GlobalINICache.insert(std::pair<String, mINI::INIStructure*>(sFileName, ini_data));
			}

			return ini_data;
		}

		bool INICacheDataPatch::QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
			const char* lpcstrPlatformRuntimeVersion) const
		{
			return true;
		}

		bool INICacheDataPatch::Activate(const Relocator* lpRelocator,
			const RelocationDatabaseItem* lpRelocationDatabaseItem)
		{
			PatchIAT(HKGetPrivateProfileIntA, "kernel32.dll", "GetPrivateProfileIntA");
			PatchIAT(HKGetPrivateProfileStringA, "kernel32.dll", "GetPrivateProfileStringA");
			PatchIAT(HKGetPrivateProfileStructA, "kernel32.dll", "GetPrivateProfileStructA");
			PatchIAT(HKWritePrivateProfileStringA, "kernel32.dll", "WritePrivateProfileStringA");
			PatchIAT(HKWritePrivateProfileStructA, "kernel32.dll", "WritePrivateProfileStructA");

			return true;
		}

		bool INICacheDataPatch::Shutdown(const Relocator* lpRelocator,
			const RelocationDatabaseItem* lpRelocationDatabaseItem)
		{
			return false;
		}
	}
}