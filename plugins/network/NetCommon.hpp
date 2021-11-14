#ifndef __NETCOMMON_HPP__
#define __NETCOMMON_HPP__

#include <windows.h>
#include <lm.h>
#include <plugin.hpp>
#include "NetLng.hpp"

extern struct Options
{
	int AddToDisksMenu;
	int AddToPluginsMenu;
	int HiddenShares;
	int ShowPrinters;
	int LocalNetwork;
	int DisconnectMode;
	int ConfirmRemoveConnection;
	int HiddenSharesAsHidden;
	int FullPathShares;
	int FavoritesFlags;
	int RootDoublePoint;
	int NavigateToDomains;

	void Read();
} Opt;


extern NETRESOURCE CommonCurResource;
extern LPNETRESOURCE PCommonCurResource;
extern BOOL IsFirstRun;

const wchar_t* GetMsg(int MsgId);

BOOL DlgCreateFolder(wchar_t* lpBuffer, int nBufferSize);

#define ShowMessage(x) PsInfo.Message(&MainGuid, nullptr, FMSG_ALLINONE|FMSG_MB_OK, L"", reinterpret_cast<const wchar_t* const*>(x), 0,{})

#endif // __NETCOMMON_HPP__
