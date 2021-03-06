@echo off

rem Runs HlfChecker.linq script to check specified help files in specified languages.
rem
rem Synopsys:
rem     HlfChecker.cmd [<directory> ...] { [+<lng> ...] | [-<lng> ...] }
rem
rem Examples:
rem   Check help files of Far and all plugins in all languages:
rem     HlfChecker.cmd
rem
rem   Check help files of Far and Temporary panel plugin in all languages:
rem     HlfChecker.cmd ..\..\far ..\..\plugins\tmppanel
rem
rem   Check help files of Far and all plugins in Polish language:
rem     HlfChecker.cmd +pol
rem
rem   Check help files of FAR Commands plugins in all languages except Russian:
rem     HlfChecker.cmd ..\..\plugins\farcmds -rus
rem
rem If a help file was parsed successfully, the script will overwrite it with the parsed contents.
rem If there were errors, the file will not be overwritten. All errors and warnings will be printed.
rem If the file was clean, its contents will not change (but timestamp will be updated).
rem The script will correct empty lines and remove trailing spaces (while printing corresponding warnings).
rem Use "git diff" command after running the script.

LPRun.exe HlfChecker.linq %*
