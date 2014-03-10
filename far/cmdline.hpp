#pragma once

/*
cmdline.hpp

��������� ������
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

#include "scrobj.hpp"
#include "editcontrol.hpp"

class CommandLine:public ScreenObject
{
public:
	CommandLine();
	virtual ~CommandLine();

	virtual int ProcessKey(int Key) override;
	virtual int ProcessMouse(const MOUSE_EVENT_RECORD *MouseEvent) override;
	virtual __int64 VMProcess(int OpCode,void *vParam=nullptr,__int64 iParam=0) override;
	virtual void ResizeConsole() override;

	int GetCurDir(string &strCurDir);
	void SetCurDir(const string& CurDir);
	void GetString(string &strStr) const { CmdStr.GetString(strStr); }
	int GetLength() const { return CmdStr.GetLength(); }
	void SetString(const string& Str,bool Redraw=true);
	void InsertString(const string& Str);
	int ExecString(const string& Str, bool AlwaysWaitFinish, bool SeparateWindow = false, bool DirectRun = false, bool WaitForIdle = false, bool RunAs = false, bool RestoreCmd = false);
	void ShowViewEditHistory();
	void SetCurPos(int Pos, int LeftPos=0, bool Redraw=true);
	int GetCurPos() const { return CmdStr.GetCurPos(); }
	int GetLeftPos() const { return CmdStr.GetLeftPos(); }
	void SetPersistentBlocks(bool Mode);
	void SetDelRemovesBlocks(bool Mode);
	void SetAutoComplete(int Mode);
	void GetSelection(intptr_t &Start,intptr_t &End) const { CmdStr.GetSelection(Start,End); }
	void Select(int Start, int End) { CmdStr.Select(Start,End); }
	void SaveBackground(int X1,int Y1,int X2,int Y2);
	void SaveBackground();
	void ShowBackground();
	void CorrectRealScreenCoord();
	void LockUpdatePanel(bool Mode);
	const int GetPromptSize() const {return PromptSize;}
	void SetPromptSize(int NewSize);

private:
	virtual void DisplayObject() override;
	int ProcessOSCommands(const string& CmdLine,bool SeparateWindow, bool &PrintCommand);
	std::list<std::pair<string, FarColor>> GetPrompt();
	static bool IntChDir(const string& CmdLine,int ClosePanel,bool Selent=false);
	static bool CheckCmdLineForHelp(const wchar_t *CmdLine);
	static bool CheckCmdLineForSet(const string& CmdLine);

	friend class SetAutocomplete;

	int PromptSize;
	EditControl CmdStr;
	SaveScreen *BackgroundScreen;
	string strCurDir;
	string strLastCmdStr;
	int LastCmdPartLength;
	std::stack<string> ppstack;
};
