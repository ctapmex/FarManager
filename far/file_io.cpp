﻿/*
file_io.cpp

*/
/*
Copyright © 2019 Far Group
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

// Self:
#include "file_io.hpp"

// Internal:
#include "config.hpp"
#include "exception.hpp"
#include "flink.hpp"
#include "global.hpp"
#include "mix.hpp"
#include "pathmix.hpp"

// Platform:
#include "platform.fs.hpp"

// Common:
#include "common/string_utils.hpp"

// External:

//----------------------------------------------------------------------------

void save_file_with_replace(string const& FileName, DWORD const FileAttributes, DWORD const ExtraAttributes, bool const CreateBackup, function_ref<void(std::ostream& Sream)> const Handler)
{
	const auto IsFileExists = FileAttributes != INVALID_FILE_ATTRIBUTES;

	const auto UseTemporaryFile = IsFileExists && [&]
	{
		if (!Global->Opt->EdOpt.SaveSafely)
			return false;

		// ReplaceFileW doesn't work with streams
		if (contains(PointToName(FileName), L':'))
			return false;

		// ReplaceFileW doesn't work with symlinks
		if (FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			return false;

		// We don't want to break hardlinks
		if (const auto Hardlinks = GetNumberOfLinks(FileName); Hardlinks && *Hardlinks > 1)
			return false;

		return true;
	}();

	const auto OutFileName = UseTemporaryFile? MakeTempInSameDir(FileName) : FileName;
	const auto NewAttributes = (IsFileExists? FileAttributes : 0) | FILE_ATTRIBUTE_ARCHIVE | ExtraAttributes;

	os::security::descriptor SecurityDescriptor;
	SECURITY_ATTRIBUTES SecurityAttributes{};

	if (UseTemporaryFile)
	{
		// ReplaceFileW handles DAC, but just in case if it can't for whatever reason:
		SecurityDescriptor = os::fs::get_file_security(FileName, DACL_SECURITY_INFORMATION);
		if (SecurityDescriptor)
			SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor.get();
	}

	{
		os::fs::file OutFile(
			OutFileName,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			SecurityDescriptor? &SecurityAttributes : nullptr,
			IsFileExists && !UseTemporaryFile? TRUNCATE_EXISTING : CREATE_NEW,
			// No FILE_ATTRIBUTE_SYSTEM at this point to avoid potential conflicts with FILE_ATTRIBUTE_ENCRYPTED. We will set it later.
			NewAttributes & ~FILE_ATTRIBUTE_SYSTEM);

		if (!OutFile)
			throw MAKE_FAR_EXCEPTION(L"Can't create a temporary file"sv);

		os::fs::filebuf StreamBuffer(OutFile, std::ios::out);
		std::ostream Stream(&StreamBuffer);
		Stream.exceptions(Stream.badbit | Stream.failbit);

		Handler(Stream);

		Stream.flush();
		OutFile.SetEnd();
	}

	if (UseTemporaryFile)
	{
		if (!os::fs::replace_file(FileName, OutFileName, CreateBackup? FileName + L".bak"sv : L""sv, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS))
			throw MAKE_FAR_EXCEPTION(L"Can't replace the file"sv);
	}

	// No error checking - non-critical (TODO: log)
	os::fs::set_file_attributes(FileName, NewAttributes);
}