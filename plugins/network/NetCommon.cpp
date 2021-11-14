#include "NetCommon.hpp"
#include "Network.hpp"
#include "guid.hpp"
#include "NetFavorites.hpp"

Options Opt;


NETRESOURCE CommonCurResource;
NETRESOURCE* PCommonCurResource{};

BOOL IsFirstRun = TRUE;

BOOL DlgCreateFolder(wchar_t* lpBuffer, int nBufferSize)
{
	BOOL res = (BOOL)PsInfo.InputBox(
		&MainGuid,
		nullptr,
		L"Make Folder",
		L"Folder name:",
		L"NewFolder",
		{},
		lpBuffer,
		nBufferSize,
		{},
		FIB_BUTTONS
	);
	return res;
}

const wchar_t* GetMsg(int MsgId)
{
	return PsInfo.GetMsg(&MainGuid, MsgId);
}

