name	shell32
type	win32
base	1

0000 stub CheckEscapesA
0001 stub CheckEscapesW
0002 stdcall CommandLineToArgvW(ptr ptr) CommandLineToArgvW
0003 stub Control_FillCache_RunDLL
0004 stub Control_RunDLL
0005 stub DllGetClassObject
0006 stub DoEnvironmentSubstA
0007 stub DoEnvironmentSubstW
0008 stdcall DragAcceptFiles(long long) DragAcceptFiles
0009 stub DragFinish
0010 stub DragQueryFile
0011 stub DragQueryFileA
0012 stub DragQueryFileAorW
0013 stub DragQueryFileW
0014 stub DragQueryPoint
0015 stub DuplicateIcon
0016 stub ExtractAssociatedIconA
0017 stub ExtractAssociatedIconExA
0018 stub ExtractAssociatedIconExW
0019 stub ExtractAssociatedIconW
0020 stub ExtractIconA
0021 stub ExtractIconEx
0022 stub ExtractIconExA
0023 stub ExtractIconExW
0024 stub ExtractIconResInfoA
0025 stub ExtractIconResInfoW
0026 stub ExtractIconW
0027 stub ExtractVersionResource16W
0028 stub FindExeDlgProc
0029 stub FindExecutableA
0030 stub FindExecutableW
0031 return FreeIconList 4 0
0032 stub InternalExtractIconListA
0033 stub InternalExtractIconListW
0034 stub OpenAs_RunDLL
0035 stub PrintersGetCommand_RunDLL
0036 stub RealShellExecuteA
0037 stub RealShellExecuteExA
0038 stub RealShellExecuteExW
0039 stub RealShellExecuteW
0040 stub RegenerateUserEnvironment
0041 stub RegisterShellHook
0042 stub SHAddToRecentDocs
0043 stub SHAppBarMessage
0044 stub SHBrowseForFolder
0045 stub SHBrowseForFolderA
0046 stub SHBrowseForFolderW
0047 stub SHChangeNotify
0048 stub SHFileOperation
0049 stub SHFileOperationA
0050 stub SHFileOperationW
0051 stub SHFormatDrive
0052 stub SHFreeNameMappings
0053 stub SHGetDesktopFolder
0054 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfo32A
0055 stdcall SHGetFileInfoA(ptr long ptr long long) SHGetFileInfo32A
0056 stub SHGetFileInfoW
0057 stub SHGetInstanceExplorer
0058 stub SHGetMalloc
0059 stub SHGetPathFromIDList
0060 stub SHGetPathFromIDListA
0061 stub SHGetPathFromIDListW
0062 stub SHGetSpecialFolderLocation
0063 stub SHHelpShortcuts_RunDLL
0064 stub SHLoadInProc
0065 stub SheChangeDirA
0066 stub SheChangeDirExA
0067 stub SheChangeDirExW
0068 stub SheChangeDirW
0069 stub SheConvertPathW
0070 stub SheFullPathA
0071 stub SheFullPathW
0072 stub SheGetCurDrive
0073 stub SheGetDirA
0074 stub SheGetDirExW
0075 stub SheGetDirW
0076 stub SheGetPathOffsetW
0077 stub SheRemoveQuotesA
0078 stub SheRemoveQuotesW
0079 stub SheSetCurDrive
0080 stub SheShortenPathA
0081 stub SheShortenPathW
0082 stdcall ShellAboutA(long ptr ptr long) ShellAbout32A
0083 stdcall ShellAboutW(long ptr ptr long) ShellAbout32W
0084 stdcall ShellExecuteA(long ptr ptr ptr ptr long) ShellExecute
0085 stub ShellExecuteEx
0086 stub ShellExecuteExA
0087 stub ShellExecuteExW
0088 stub ShellExecuteW
0089 stub ShellHookProc
0090 stub Shell_NotifyIcon
0091 stub Shell_NotifyIconA
0092 stub Shell_NotifyIconW
0093 stub StrChrA
0094 stub StrChrIA
0095 stub StrChrIW
0096 stub StrChrW
0097 stub StrCmpNA
0098 stub StrCmpNIA
0099 stub StrCmpNIW
0100 stub StrCmpNW
0101 stub StrCpyNA
0102 stub StrCpyNW
0103 stub StrNCmpA
0104 stub StrNCmpIA
0105 stub StrNCmpIW
0106 stub StrNCmpW
0107 stub StrNCpyA
0108 stub StrNCpyW
0109 stub StrRChrA
0110 stub StrRChrIA
0111 stub StrRChrIW
0112 stub StrRChrW
0113 stub StrRStrA
0114 stub StrRStrIA
0115 stub StrRStrIW
0116 stub StrRStrW
0117 stub StrStrA
0118 stub StrStrIA
0119 stub StrStrIW
0120 stub StrStrW
0121 stub WOWShellExecute
