/*
hotplug.cpp

���������� Hotplug-���������
*/
/*
Copyright � 1996 Eugene Roshal
Copyright � 2000 Far Group
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

#include "headers.hpp"
#pragma hdrstop

#include "hotplug.hpp"
#include "language.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "vmenu2.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "notification.hpp"
#include "lasterror.hpp"

/*
A device is considered a HotPlug device if the following are TRUE:
- does NOT have problem CM_PROB_DEVICE_NOT_THERE
- does NOT have problem CM_PROB_HELD_FOR_EJECT
- does NOT have problem CM_PROB_DISABLED
- has Capability CM_DEVCAP_REMOVABLE
- does NOT have Capability CM_DEVCAP_SURPRISEREMOVALOK
- does NOT have Capability CM_DEVCAP_DOCKDEVICE
*/
static bool IsChildDeviceHotplug(DEVINST hDevInst)
{
	bool Result=false;
	DEVINST hDevChild;
	if (CM_Get_Child(&hDevChild,hDevInst,0)==CR_SUCCESS)
	{
		wchar_t szDeviceID[MAX_DEVICE_ID_LEN];
		if (CM_Get_Device_ID(hDevChild, szDeviceID, ARRAYSIZE(szDeviceID), 0) == CR_SUCCESS)
		{
			HDEVINFO Info = SetupDiGetClassDevs(&GUID_DEVINTERFACE_VOLUME, szDeviceID, nullptr, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
			if(Info != INVALID_HANDLE_VALUE)
			{
				SCOPE_EXIT { SetupDiDestroyDeviceInfoList(Info); };

				SP_DEVINFO_DATA DeviceInfoData = {sizeof(DeviceInfoData)};
				if(SetupDiOpenDeviceInfo(Info, szDeviceID, nullptr, 0, &DeviceInfoData))
				{
					DWORD Capabilities = 0;
					if(SetupDiGetDeviceRegistryProperty(Info, &DeviceInfoData, SPDRP_CAPABILITIES, nullptr, reinterpret_cast<PBYTE>(&Capabilities), sizeof(Capabilities), nullptr))
					{
						Result=!(Capabilities&CM_DEVCAP_SURPRISEREMOVALOK)&&(Capabilities&CM_DEVCAP_UNIQUEID);
					}
				}
			}
		}
	}
	return Result;
}

static bool IsDeviceHotplug(DEVINST hDevInst)
{
	bool Result = false;
	wchar_t szDeviceID[MAX_DEVICE_ID_LEN];
	if (CM_Get_Device_ID(hDevInst, szDeviceID, ARRAYSIZE(szDeviceID), 0) == CR_SUCCESS)
	{
		HDEVINFO Info = SetupDiGetClassDevs(&GUID_DEVINTERFACE_VOLUME, szDeviceID, nullptr, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
		if(Info != INVALID_HANDLE_VALUE)
		{
			SCOPE_EXIT { SetupDiDestroyDeviceInfoList(Info); };

			SP_DEVINFO_DATA DeviceInfoData = {sizeof(DeviceInfoData)};
			if(SetupDiOpenDeviceInfo(Info, szDeviceID, nullptr, 0, &DeviceInfoData))
			{
				DWORD Capabilities = 0;
				if(SetupDiGetDeviceRegistryProperty(Info, &DeviceInfoData, SPDRP_CAPABILITIES, nullptr, reinterpret_cast<PBYTE>(&Capabilities), sizeof(Capabilities), nullptr))
				{
					DWORD Status = 0, Problem = 0;
					if (CM_Get_DevNode_Status(&Status, &Problem, hDevInst, 0) == CR_SUCCESS)
					{
						if ((Problem != CM_PROB_DEVICE_NOT_THERE) &&
								(Problem != CM_PROB_HELD_FOR_EJECT) && //��������, ���� ��������� �� ������� ������� ������
								(Problem != CM_PROB_DISABLED) &&
								(Capabilities & CM_DEVCAP_REMOVABLE) &&
								(!(Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) || IsChildDeviceHotplug(hDevInst)) &&
								!(Capabilities & CM_DEVCAP_DOCKDEVICE))
							Result = true;
					}
				}
			}
		}
	}
	return Result;
}

static DWORD DriveMaskFromVolumeName(const string& VolumeName)
{
	DWORD Result = 0;
	string strCurrentVolumeName;
	auto Strings = api::GetLogicalDriveStrings();
	std::any_of(CONST_RANGE(Strings, i) -> bool
	{
		if (api::GetVolumeNameForVolumeMountPoint(i, strCurrentVolumeName) && strCurrentVolumeName.compare(0, VolumeName.size(), VolumeName) == 0)
		{
			Result = 1 << (i.front() - L'A');
			return true;
		}
		return false;
	});
	return Result;
}

static DWORD GetDriveMaskFromMountPoints(DEVINST hDevInst)
{
	DWORD dwMask = 0;
	wchar_t szDeviceID[MAX_DEVICE_ID_LEN];
	if (CM_Get_Device_ID(hDevInst, szDeviceID, ARRAYSIZE(szDeviceID), 0) == CR_SUCCESS)
	{
		HDEVINFO Info = SetupDiGetClassDevs(&GUID_DEVINTERFACE_VOLUME, szDeviceID, nullptr, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
		if(Info != INVALID_HANDLE_VALUE)
		{
			SCOPE_EXIT { SetupDiDestroyDeviceInfoList(Info); };

			SP_DEVICE_INTERFACE_DATA sdid = {sizeof(sdid)};
			DWORD MemberIndex = 0;
			while(SetupDiEnumDeviceInterfaces(Info, nullptr, &GUID_DEVINTERFACE_VOLUME, MemberIndex, &sdid))
			{
				MemberIndex++;
				DWORD RequiredSize = 0;
				SetupDiGetDeviceInterfaceDetail(Info, &sdid, nullptr, 0, &RequiredSize, nullptr);
				if(RequiredSize)
				{
					block_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> DData(RequiredSize);
					DData->cbSize = sizeof(*DData);
					if(SetupDiGetDeviceInterfaceDetail(Info, &sdid, DData.get(), RequiredSize, nullptr, nullptr))
					{
						string strMountPoint(DData->DevicePath);
						AddEndSlash(strMountPoint);
						string strVolumeName;
						if (api::GetVolumeNameForVolumeMountPoint(strMountPoint,strVolumeName))
						{
							dwMask |= DriveMaskFromVolumeName(strVolumeName);
						}
					}
				}
			}
		}
	}
	return dwMask;
}

static DWORD GetRelationDrivesMask(DEVINST hDevInst)
{
	DWORD dwMask = 0;
	wchar_t szDeviceID[MAX_DEVICE_ID_LEN];
	if (CM_Get_Device_ID(hDevInst, szDeviceID, ARRAYSIZE(szDeviceID), 0) == CR_SUCCESS)
	{
		DWORD dwSize = 0;
		if (CM_Get_Device_ID_List_Size(&dwSize, szDeviceID, CM_GETIDLIST_FILTER_REMOVALRELATIONS) == CR_SUCCESS && dwSize)
		{
			wchar_t_ptr DeviceIdList(dwSize);
			if (CM_Get_Device_ID_List(szDeviceID, DeviceIdList.get(), dwSize, CM_GETIDLIST_FILTER_REMOVALRELATIONS) == CR_SUCCESS)
			{
				wchar_t *p = DeviceIdList.get();

				while (*p)
				{
					DEVINST hRelationDevInst;
					if (CM_Locate_DevNode(&hRelationDevInst, p, 0) == CR_SUCCESS)
						dwMask |= GetDriveMaskFromMountPoints(hRelationDevInst);

					p += wcslen(p)+1;
				}
			}
		}
	}

	return dwMask;
}

static DWORD GetDriveMaskForDeviceInternal(DEVINST hDevInst)
{
	DWORD dwMask = 0;
	do
	{
		if (!IsDeviceHotplug(hDevInst))
		{
			dwMask |= GetDriveMaskFromMountPoints(hDevInst);
			dwMask |= GetRelationDrivesMask(hDevInst);

			DEVINST hDevChild;
			if (CM_Get_Child(&hDevChild, hDevInst, 0) == CR_SUCCESS)
				dwMask |= GetDriveMaskForDeviceInternal(hDevChild);
		}
	}
	while (CM_Get_Sibling(&hDevInst, hDevInst, 0) == CR_SUCCESS);

	return dwMask;
}


static std::bitset<32> GetDisksForDevice(DEVINST hDevInst)
{
	int DisksMask = 0;
	DisksMask |= GetDriveMaskFromMountPoints(hDevInst);
	DisksMask |= GetRelationDrivesMask(hDevInst);

	DEVINST hDevChild;
	if (CM_Get_Child(&hDevChild, hDevInst, 0) == CR_SUCCESS)
		DisksMask |= GetDriveMaskForDeviceInternal(hDevChild);

	return std::bitset<32>(DisksMask);
}

static bool GetDeviceProperty(DEVINST hDevInst, DWORD Property, string& strValue, bool bSearchChild)
{
	bool Result = false;
	wchar_t szDeviceID[MAX_DEVICE_ID_LEN];
	if (CM_Get_Device_ID(hDevInst, szDeviceID, ARRAYSIZE(szDeviceID), 0) == CR_SUCCESS)
	{
		HDEVINFO Info = SetupDiGetClassDevs(&GUID_DEVINTERFACE_VOLUME, szDeviceID, nullptr, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
		if(Info != INVALID_HANDLE_VALUE)
		{
			SCOPE_EXIT { SetupDiDestroyDeviceInfoList(Info); };

			SP_DEVINFO_DATA DeviceInfoData = {sizeof(DeviceInfoData)};
			if(SetupDiOpenDeviceInfo(Info, szDeviceID, nullptr, 0, &DeviceInfoData))
			{
				DWORD RequiredSize = 0;
				SetupDiGetDeviceRegistryProperty(Info, &DeviceInfoData, Property, nullptr, nullptr, 0, &RequiredSize);
				if(RequiredSize)
				{
					wchar_t_ptr Buffer(RequiredSize);
					if(SetupDiGetDeviceRegistryProperty(Info, &DeviceInfoData, Property, nullptr, reinterpret_cast<BYTE*>(Buffer.get()), RequiredSize, nullptr))
					{
						Result = true;
					}
					strValue.assign(Buffer.get());
				}
				else
				{
					if(bSearchChild)
					{
						DEVINST hDevChild;
						if (CM_Get_Child(&hDevChild, hDevInst, 0) == CR_SUCCESS)
						{
							Result = GetDeviceProperty(hDevChild, Property, strValue, bSearchChild);
						}
					}
				}
			}
		}
	}
	return Result;
}

struct DeviceInfo
{
	DEVINST DevInst;
	std::bitset<32> Disks;
};

static void GetChildHotplugDevicesInfo(DEVINST hDevInst, std::vector<DeviceInfo>& Info)
{
	do
	{
		if (IsDeviceHotplug(hDevInst))
		{
			DeviceInfo Item = {hDevInst, GetDisksForDevice(hDevInst)};
			Info.emplace_back(Item);
		}

		DEVINST hDevChild;
		if (CM_Get_Child(&hDevChild, hDevInst, 0) == CR_SUCCESS)
			GetChildHotplugDevicesInfo(hDevChild, Info);
	}
	while (CM_Get_Sibling(&hDevInst, hDevInst, 0) == CR_SUCCESS);
}

static std::vector<DeviceInfo> GetHotplugDevicesInfo()
{
	std::vector<DeviceInfo> Result;

	DEVNODE hDevRoot;
	if (CM_Locate_DevNodeW(&hDevRoot, nullptr, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS)
	{
		DEVINST hDevChild;
		if (CM_Get_Child(&hDevChild, hDevRoot, 0) == CR_SUCCESS)
		{
			GetChildHotplugDevicesInfo(hDevChild, Result);
		}
	}
	return Result;
}

static int RemoveHotplugDevice(const DeviceInfo& Info, DWORD Flags)
{
	string strFriendlyName;
	GetDeviceProperty(Info.DevInst, SPDRP_FRIENDLYNAME, strFriendlyName, true);
	RemoveExternalSpaces(strFriendlyName);

	string strDescription;
	GetDeviceProperty(Info.DevInst, SPDRP_DEVICEDESC, strDescription, true);
	RemoveExternalSpaces(strDescription);

	int MessageResult = 0;

	if (!(Flags&EJECT_NO_MESSAGE) && Global->Opt->Confirm.RemoveHotPlug)
	{
		string DisksStr;
		for (size_t i = 0; i < Info.Disks.size(); ++i)
		{
			if (Info.Disks[i])
				DisksStr.append(1, static_cast<wchar_t>(L'A' + i)).append(L":, ", 3);
		}

		// remove trailing ", "
		if (!DisksStr.empty())
			DisksStr.resize(DisksStr.size() - 2);

		std::vector<string> MessageItems;
		MessageItems.reserve(6);

		MessageItems.emplace_back(MSG(MChangeHotPlugDisconnectDriveQuestion));
		MessageItems.emplace_back(strDescription);

		if (!strFriendlyName.empty() && StrCmpI(strDescription.data(), strFriendlyName.data()))
			MessageItems.emplace_back(strFriendlyName);

		if (!DisksStr.empty())
			MessageItems.emplace_back(LangString(MHotPlugDisks) << DisksStr);

		MessageItems.emplace_back(MSG(MHRemove));
		MessageItems.emplace_back(MSG(MHCancel));

		MessageResult = Message(MSG_WARNING, 2, MSG(MChangeHotPlugDisconnectDriveTitle), MessageItems);
	}

	int bResult = -1;

	if (MessageResult == 0)
	{
		PNP_VETO_TYPE pvtVeto = PNP_VetoTypeUnknown;
		wchar_t VetoName[MAX_PATH];
		CONFIGRET crResult = CM_Request_Device_Eject(Info.DevInst, &pvtVeto, VetoName, ARRAYSIZE(VetoName), 0);
		if ((crResult != CR_SUCCESS) || (pvtVeto != PNP_VetoTypeUnknown))   //M$ ���, ���� ���� VetoName, �� ���� ��� ������ ������������ CR_SUCCESS
		{
			SetLastError((pvtVeto != PNP_VetoTypeUnknown)?ERROR_DRIVE_LOCKED:ERROR_UNABLE_TO_UNLOAD_MEDIA); // "The disk is in use or locked by another process."
			bResult = 0;
		}
		else
		{
			SetLastError(ERROR_SUCCESS);
			bResult = 1;
		}
	}

	if (bResult == 1 && (Flags&EJECT_NOTIFY_AFTERREMOVE))
	{
		Message(0,1,MSG(MChangeHotPlugDisconnectDriveTitle),MSG(MChangeHotPlugNotify1),strDescription.data(),strFriendlyName.data(),MSG(MChangeHotPlugNotify2),MSG(MOk));
	}

	return bResult;
}

int RemoveHotplugDisk(wchar_t Disk, DWORD Flags)
{
	GuardLastError gle;
	const auto Info = GetHotplugDevicesInfo();
	const size_t DiskNumber = Upper(Disk) - L'A';
	const auto ItemIterator = std::find_if(CONST_RANGE(Info, i) {return i.Disks[DiskNumber];});
	return ItemIterator != Info.cend()? RemoveHotplugDevice(*ItemIterator, Flags) : -1;
}

void ShowHotplugDevices()
{
	VMenu2 HotPlugList(MSG(MHotPlugListTitle), nullptr, 0, 0);
	static std::vector<DeviceInfo> Info;

	auto FillMenu = [&]()
	{
		HotPlugList.DeleteItems();
		Info = GetHotplugDevicesInfo();

		if (!Info.empty())
		{
			MenuItemEx ListItem;

			std::for_each(CONST_RANGE(Info, i)
			{
				ListItem.Clear();
				string strFriendlyName, strDescription;
				if (GetDeviceProperty(i.DevInst, SPDRP_DEVICEDESC, strDescription, true) && !strDescription.empty())
				{
					RemoveExternalSpaces(strDescription);
					ListItem.strName = strDescription;
				}

				if (GetDeviceProperty(i.DevInst, SPDRP_FRIENDLYNAME, strFriendlyName, true))
				{
					RemoveExternalSpaces(strFriendlyName);

					if (!strDescription.empty())
					{
						if (!strFriendlyName.empty() && StrCmpI(strDescription.data(),strFriendlyName.data()))
						{
							ListItem.strName += L" \"" + strFriendlyName + L"\"";
						}
					}
					else if (!strFriendlyName.empty())
					{
						ListItem.strName = strFriendlyName;
					}
				}

				if (ListItem.strName.empty())
				{
					ListItem.strName = L"UNKNOWN";
				}
				HotPlugList.AddItem(&ListItem);
			});
		}
	};

	FillMenu();
	HotPlugList.SetFlags(VMENU_WRAPMODE|VMENU_AUTOHIGHLIGHT);
	HotPlugList.SetPosition(-1,-1,0,0);
	HotPlugList.AssignHighlights(TRUE);
	HotPlugList.SetBottomTitle(MSG(MHotPlugListBottom));

	bool NeedRefresh = false;

	listener DeviceListener(L"devices", [&NeedRefresh]()
	{
		NeedRefresh = true;
	});

	HotPlugList.Run([&](int Key)->int
	{
		if(Key==KEY_NONE && NeedRefresh)
		{
			Key=KEY_CTRLR;
			NeedRefresh = false;
		}

		int KeyProcessed = 1;

		switch (Key)
		{
			case KEY_F1:
			{
				Help Hlp(L"HotPlugList");
				break;
			}
			case KEY_CTRLR:
			{
				FillMenu();
				break;
			}
			case KEY_NUMDEL:
			case KEY_DEL:
			{
				if (HotPlugList.GetItemCount() > 0)
				{
					int bResult;
					int I = HotPlugList.GetSelectPos();

					if ((bResult=RemoveHotplugDevice(Info[I], EJECT_NOTIFY_AFTERREMOVE)) == 1)
					{
						FillMenu();
					}
					else if (bResult != -1)
					{
						SetLastError(ERROR_DRIVE_LOCKED); // ...� "The disk is in use or locked by another process."
						Global->CatchError();
						Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),
						        MSG(MChangeCouldNotEjectHotPlugMedia2),HotPlugList.GetItemPtr(I)->strName.data(),MSG(MOk));
					}
				}

				break;
			}

			default:
				KeyProcessed = 0;
		}
		return KeyProcessed;
	});
}
