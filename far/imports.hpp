﻿#ifndef IMPORTS_HPP_0589C56B_4071_48EE_B07F_312C2E392280
#define IMPORTS_HPP_0589C56B_4071_48EE_B07F_312C2E392280
#pragma once

/*
imports.hpp

импортируемые функции
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Internal:
#include "platform.hpp"

// Platform:

// Common:
#include "common/nifty_counter.hpp"

// External:

//----------------------------------------------------------------------------

namespace imports_detail
{

class imports
{
public:
	imports() = default;

private:
#define DECLARE_MODULE(MODULE) const os::rtdl::module m_##MODULE{WIDE_SV(#MODULE)}

	DECLARE_MODULE(ntdll);
	DECLARE_MODULE(kernel32);
	DECLARE_MODULE(shell32);
	DECLARE_MODULE(user32);
	DECLARE_MODULE(virtdisk);
	DECLARE_MODULE(rstrtmgr);
	DECLARE_MODULE(netapi32);
	DECLARE_MODULE(dbgeng);
	DECLARE_MODULE(dbghelp);
	DECLARE_MODULE(dwmapi);

#undef DECLARE_MODULE

	static void log_missing_import(const os::rtdl::module& Module, std::string_view Name);

	template<typename text_type, auto StubFunction>
	class unique_function_pointer
	{
		// The indirection is a workaround for MSVC
		using function_type = std::enable_if_t<true, decltype(StubFunction)>;

	public:
		NONCOPYABLE(unique_function_pointer);

		explicit unique_function_pointer(const os::rtdl::module& Module): m_module(&Module) {}
		operator function_type() const { return get_pointer(); }
		explicit operator bool() const noexcept { return get_pointer() != StubFunction; }

	private:
		auto get_pointer() const
		{
			static const auto Pointer = [&]
			{
				if (const auto DynamicPointer = m_module->GetProcAddress<function_type>(text_type::name))
					return DynamicPointer;

				return StubFunction;
			}();

			// Logged separately to avoid a deadlock in recursive static initialisation
			if (static auto Logged = false; !Logged)
			{
				if (Pointer == StubFunction)
					log_missing_import(*m_module, text_type::name);

				Logged = true;
			}


			return Pointer;
		}

		const os::rtdl::module* m_module;
	};

#define DECLARE_IMPORT_FUNCTION(MODULE, CALLTYPE, RETTYPE, NAME, ...)\
private: static RETTYPE CALLTYPE stub_##NAME(__VA_ARGS__);\
private: struct name_##NAME { static constexpr auto name = #NAME; };\
public: const unique_function_pointer<name_##NAME, stub_##NAME> NAME{m_##MODULE}

	// NT4 < 2k < XP < 2k3 < Vista < 7 < 8 < 10
	// Rock bottom is 2k

	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtQueryDirectoryFile, HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtQueryInformationFile, HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtSetInformationFile, HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtQueryObject, HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtOpenSymbolicLinkObject, PHANDLE LinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtQuerySymbolicLinkObject, HANDLE LinkHandle, PUNICODE_STRING LinkTarget, PULONG ReturnedLength); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtClose, HANDLE Handle); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, RtlGetLastNtStatus); // XP
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, ULONG, RtlNtStatusToDosError, NTSTATUS Status); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, RtlGetVersion, PRTL_OSVERSIONINFOW VersionInformation); // 2k
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, BOOLEAN, RtlAcquireResourceExclusive, PRTL_RESOURCE Res, BOOLEAN WaitForAccess); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, BOOLEAN, RtlAcquireResourceShared, PRTL_RESOURCE Res, BOOLEAN WaitForAccess); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, void, RtlInitializeResource, PRTL_RESOURCE Res); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, void, RtlReleaseResource, PRTL_RESOURCE Res); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, void, RtlDeleteResource, PRTL_RESOURCE Res); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtQueryInformationProcess, HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtQuerySystemInformation, SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength); // NT4
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, WORD, RtlCaptureStackBackTrace, DWORD FramesToSkip, DWORD FramesToCapture, PVOID* BackTrace, PDWORD BackTraceHash); // NT4
#ifndef _WIN64
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtWow64QueryInformationProcess64, HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength); // 2k3
	DECLARE_IMPORT_FUNCTION(ntdll, NTAPI, NTSTATUS, NtWow64ReadVirtualMemory64, HANDLE Process, ULONG64 BaseAddress, LPVOID Buffer, ULONG64 Size, PULONG64 NumberOfBytesRead); // 2k3
#endif

	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, GetConsoleKeyboardLayoutNameW, LPWSTR Buffer); // NT4
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOLEAN, CreateSymbolicLinkW, LPCWSTR SymlinkFileName, LPCWSTR TargetFileName, DWORD Flags); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, HANDLE, FindFirstFileNameW, LPCWSTR FileName, DWORD Flags, LPDWORD StringLength, LPWSTR LinkName); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, FindNextFileNameW, HANDLE FindStream, LPDWORD StringLength, PWCHAR LinkName); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, HANDLE, FindFirstStreamW, LPCWSTR FileName, STREAM_INFO_LEVELS InfoLevel, LPVOID FindStreamData, DWORD Flags); // 2k3
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, FindNextStreamW, HANDLE FindStream, LPVOID FindStreamData); // 2k3
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, DWORD, GetFinalPathNameByHandleW, HANDLE File, LPWSTR FilePath, DWORD FilePathSize, DWORD Flags); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, GetVolumePathNamesForVolumeNameW, LPCWSTR VolumeName, LPWSTR VolumePathNames, DWORD BufferLength, PDWORD ReturnLength); // 2k3
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, GetPhysicallyInstalledSystemMemory, PULONGLONG TotalMemoryInKilobytes); // Vista SP1
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, HeapSetInformation, HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength); // 2k + KB816542
#ifndef _WIN64
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, IsWow64Process, HANDLE Process, PBOOL Wow64Process); // 2k3
#endif
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, GetNamedPipeServerProcessId, HANDLE Pipe, PULONG ServerProcessId); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, CancelSynchronousIo, HANDLE Thread); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, SetConsoleKeyShortcuts, BOOL Set, BYTE ReserveKeys, LPVOID AppKeys, DWORD NumAppKeys); // NT4
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, GetConsoleScreenBufferInfoEx, HANDLE ConsoleOutput, PCONSOLE_SCREEN_BUFFER_INFOEX ConsoleScreenBufferInfoEx); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, QueryFullProcessImageNameW, HANDLE Process, DWORD Flags, LPWSTR ExeName, PDWORD Size); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOL, TzSpecificLocalTimeToSystemTime, const TIME_ZONE_INFORMATION* TimeZoneInformation, const SYSTEMTIME* LocalTime, LPSYSTEMTIME UniversalTime); // XP
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, void, InitializeSRWLock, PSRWLOCK SRWLock); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, void, AcquireSRWLockExclusive, PSRWLOCK SRWLock); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, void, AcquireSRWLockShared, PSRWLOCK SRWLock); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, void, ReleaseSRWLockExclusive, PSRWLOCK SRWLock); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, void, ReleaseSRWLockShared, PSRWLOCK SRWLock); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOLEAN, TryAcquireSRWLockExclusive, PSRWLOCK SRWLock); // 7
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, BOOLEAN, TryAcquireSRWLockShared, PSRWLOCK SRWLock); // 7
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, void, GetSystemTimePreciseAsFileTime, LPFILETIME SystemTimeAsFileTime); // 8
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, int, CompareStringOrdinal, LPCWCH String1, int Count1, LPCWCH String2, int Count2, BOOL IgnoreCase); // Vista
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, HRESULT, SetThreadDescription, HANDLE Thread, PCWSTR ThreadDescription); // 10
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, HRESULT, GetThreadDescription, HANDLE Thread, PWSTR* ThreadDescription); // 10
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, PVOID, AddVectoredExceptionHandler, ULONG First, PVECTORED_EXCEPTION_HANDLER Handler); // XP
	DECLARE_IMPORT_FUNCTION(kernel32, WINAPI, ULONG, RemoveVectoredExceptionHandler, PVOID Handle); // XP

	DECLARE_IMPORT_FUNCTION(shell32, STDAPICALLTYPE, HRESULT, SHCreateAssociationRegistration, REFIID riid, void** ppv); // Vista

	DECLARE_IMPORT_FUNCTION(user32, WINAPI, HPOWERNOTIFY, RegisterPowerSettingNotification, HANDLE hRecipient, LPCGUID PowerSettingGuid, DWORD Flags); // Vista
	DECLARE_IMPORT_FUNCTION(user32, WINAPI, BOOL, UnregisterPowerSettingNotification, HPOWERNOTIFY Handle); // Vista

	DECLARE_IMPORT_FUNCTION(virtdisk, WINAPI, DWORD, GetStorageDependencyInformation, HANDLE ObjectHandle, GET_STORAGE_DEPENDENCY_FLAG Flags, ULONG StorageDependencyInfoSize, PSTORAGE_DEPENDENCY_INFO StorageDependencyInfo, PULONG SizeUsed); // 7
	DECLARE_IMPORT_FUNCTION(virtdisk, WINAPI, DWORD, OpenVirtualDisk, PVIRTUAL_STORAGE_TYPE VirtualStorageType, PCWSTR Path, VIRTUAL_DISK_ACCESS_MASK VirtualDiskAccessMask, OPEN_VIRTUAL_DISK_FLAG Flags, POPEN_VIRTUAL_DISK_PARAMETERS Parameters, PHANDLE Handle); // 7
	DECLARE_IMPORT_FUNCTION(virtdisk, WINAPI, DWORD, DetachVirtualDisk, HANDLE VirtualDiskHandle, DETACH_VIRTUAL_DISK_FLAG Flags, ULONG ProviderSpecificFlags); // 7

	DECLARE_IMPORT_FUNCTION(rstrtmgr, WINAPI, DWORD, RmStartSession, DWORD *SessionHandle, DWORD SessionFlags, WCHAR strSessionKey[]); // Vista
	DECLARE_IMPORT_FUNCTION(rstrtmgr, WINAPI, DWORD, RmEndSession, DWORD dwSessionHandle); // Vista
	DECLARE_IMPORT_FUNCTION(rstrtmgr, WINAPI, DWORD, RmRegisterResources, DWORD dwSessionHandle, UINT nFiles, LPCWSTR rgsFilenames[], UINT nApplications, RM_UNIQUE_PROCESS rgApplications[], UINT nServices, LPCWSTR rgsServiceNames[]); // Vista
	DECLARE_IMPORT_FUNCTION(rstrtmgr, WINAPI, DWORD, RmGetList, DWORD dwSessionHandle, UINT *pnProcInfoNeeded, UINT *pnProcInfo, RM_PROCESS_INFO rgAffectedApps[], LPDWORD lpdwRebootReasons); // Vista

	DECLARE_IMPORT_FUNCTION(netapi32, NET_API_FUNCTION, NET_API_STATUS, NetDfsGetInfo, LPWSTR DfsEntryPath, LPWSTR ServerName, LPWSTR ShareName, DWORD Level, LPBYTE* Buffer); // 2k
	DECLARE_IMPORT_FUNCTION(netapi32, NET_API_FUNCTION, NET_API_STATUS, NetDfsGetClientInfo, LPWSTR DfsEntryPath, LPWSTR ServerName, LPWSTR ShareName, DWORD Level, LPBYTE* Buffer); // 2k

	DECLARE_IMPORT_FUNCTION(dbgeng, STDAPICALLTYPE, HRESULT, DebugCreate, REFIID InterfaceId, PVOID* Interface);

	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, MiniDumpWriteDump, HANDLE Process, DWORD ProcessId, HANDLE File, MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam); // XP
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, StackWalk64, DWORD MachineType, HANDLE Process, HANDLE Thread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymInitialize, HANDLE Process, PCSTR UserSearchPath, BOOL InvadeProcess); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymInitializeW, HANDLE Process, PCWSTR UserSearchPath, BOOL InvadeProcess); // Vista
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymCleanup, HANDLE Process); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymFromAddr, HANDLE Process, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymFromAddrW, HANDLE Process, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFOW Symbol); // Vista
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, DWORD, SymSetOptions, DWORD SymOptions); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymGetSymFromAddr64, HANDLE Process, DWORD64 Addr, PDWORD64 Displacement, PIMAGEHLP_SYMBOL64 Symbol); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymGetLineFromAddr64, HANDLE Process, DWORD64 Addr, PDWORD Displacement, PIMAGEHLP_LINE64 Line); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymGetLineFromAddrW64, HANDLE Process, DWORD64 Addr, PDWORD Displacement, PIMAGEHLP_LINEW64 Line); // Vista
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, BOOL, SymGetModuleInfoW64, HANDLE Process, DWORD64 Addr, PIMAGEHLP_MODULEW64 ModuleInfo); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, PVOID, SymFunctionTableAccess64, HANDLE Process, DWORD64 AddrBase); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, DWORD64, SymGetModuleBase64, HANDLE Process, DWORD64 Address); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, DWORD, UnDecorateSymbolName, PCSTR Name, PSTR OutputString, DWORD MaxStringLength, DWORD Flags); // 2k
	DECLARE_IMPORT_FUNCTION(dbghelp, WINAPI, DWORD, UnDecorateSymbolNameW, PCWSTR Name, PWSTR OutputString, DWORD MaxStringLength, DWORD Flags); // Vista

	DECLARE_IMPORT_FUNCTION(dwmapi, WINAPI, HRESULT, DwmGetWindowAttribute, HWND Hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute); // Vista

#undef DECLARE_IMPORT_FUNCTION
};

}

NIFTY_DECLARE(imports_detail::imports, imports);

#endif // IMPORTS_HPP_0589C56B_4071_48EE_B07F_312C2E392280
