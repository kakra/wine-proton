/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <malloc.h>
#include "windows.h"
#include "winerror.h"
#include "winnls.h"
#include "heap.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           GetACP               (KERNEL32.148)
 */
UINT32 GetACP(void)
{
    return 1252;    /* Windows 3.1 ISO Latin */
}

/***********************************************************************
 *           GetCPInfo            (KERNEL32.154)
 */
BOOL32 GetCPInfo( UINT32 codepage, LPCPINFO cpinfo )
{
    cpinfo->DefaultChar[0] = '?';
    switch (codepage)
    {
    case 932 : /* Shift JIS (japan) */
        cpinfo->MaxCharSize = 2;
        cpinfo->LeadByte[0]= 0x81; cpinfo->LeadByte[1] = 0x9F;
        cpinfo->LeadByte[2]= 0xE0; cpinfo->LeadByte[3] = 0xFC;
        cpinfo->LeadByte[4]= 0x00; cpinfo->LeadByte[5] = 0x00;
        break;
    case 936 : /* GB2312 (Chinese) */
    case 949 : /* KSC5601-1987 (Korean) */
    case 950 : /* BIG5 (Chinese) */
        cpinfo->MaxCharSize = 2;
        cpinfo->LeadByte[0]= 0x81; cpinfo->LeadByte[1] = 0xFE;
        cpinfo->LeadByte[2]= 0x00; cpinfo->LeadByte[3] = 0x00;
        break;
    case 1361 : /* Johab (Korean) */
        cpinfo->MaxCharSize = 2;
        cpinfo->LeadByte[0]= 0x84; cpinfo->LeadByte[1] = 0xD3;
        cpinfo->LeadByte[2]= 0xD8; cpinfo->LeadByte[3] = 0xDE;
        cpinfo->LeadByte[4]= 0xE0; cpinfo->LeadByte[5] = 0xF9;
        cpinfo->LeadByte[6]= 0x00; cpinfo->LeadByte[7] = 0x00;
        break;
    default :
    	cpinfo->MaxCharSize = 1;
        cpinfo->LeadByte[0]= 0x00; cpinfo->LeadByte[1] = 0x00;
    	break;
    }
    return 1;
}

/***********************************************************************
 *              GetOEMCP                (KERNEL32.248)
 */
UINT32 GetOEMCP(void)
{
    return 437;    /* MS-DOS United States */
}

/***********************************************************************
 *              MultiByteToWideChar                (KERNEL32.392)
 */
int MultiByteToWideChar(UINT page, DWORD flags, char *src, int srclen,
                        WCHAR *dst, int dstlen)
{
    return (srclen==-1) ? strlen(src) * 2: srclen*2; 
}

int WideCharToMultiByte(UINT page, DWORD flags, WCHAR *src, int srclen,
						char *dst, int dstlen, char* defchar, BOOL *used)
{
	int count = 0;
	int dont_copy= (dstlen==0);
	if(page!=GetACP() && page!=CP_OEMCP && page!=CP_ACP)
		fprintf(stdnimp,"Conversion in CP %d not supported\n",page);
	if(flags)
		fprintf(stdnimp,"WideCharToMultiByte flags %lx not supported\n",flags);
	if(used)
		*used=0;
	while(srclen && (dont_copy || dstlen))
	{
		if(!dont_copy){
			if(*src<256)
				*dst = *src;
			else
			{
				/* FIXME: Is this correct ?*/
				if(flags & WC_DEFAULTCHAR){
					*dst = defchar ? *defchar : '?';
					if(used)*used=1;
				}
			}
			dstlen--;
			dst++;
		}
		count++;
		if(!*src)
			break;
		if(srclen!=-1)srclen--;
		src++;
	}
	return count;
}


BOOL32 IsDBCSLeadByteEx(UINT32 codepage, BYTE testchar)
{
    CPINFO cpinfo;
    int i;

    GetCPInfo(codepage, &cpinfo);
    for (i = 0 ; i < sizeof(cpinfo.LeadByte)/sizeof(cpinfo.LeadByte[0]); i+=2)
    {
	if (cpinfo.LeadByte[i] == 0)
            return FALSE;
	if (cpinfo.LeadByte[i] <= testchar && testchar <= cpinfo.LeadByte[i+1])
            return TRUE;
    }
    return FALSE;
}

BOOL32 IsDBCSLeadByte(BYTE testchar)
{
    return IsDBCSLeadByteEx(GetACP(), testchar);
}


/***********************************************************************
 *              EnumSystemCodePages32A                (KERNEL32.92)
 */
BOOL32
EnumSystemCodePages32A(CODEPAGE_ENUMPROC32A lpfnCodePageEnum,DWORD flags) {
	dprintf_win32(stddeb,"EnumSystemCodePages32A(%p,%08lx)\n",
		lpfnCodePageEnum,flags
	);
	lpfnCodePageEnum("437");
	return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePages32W                (KERNEL32.93)
 */
BOOL32
EnumSystemCodePages32W( CODEPAGE_ENUMPROC32W lpfnCodePageEnum, DWORD flags)
{
    WCHAR	*cp;
    dprintf_win32(stddeb,"EnumSystemCodePages32W(%p,%08lx)\n",
                  lpfnCodePageEnum,flags );

    cp = HEAP_strdupAtoW( GetProcessHeap(), 0, "437" );
    lpfnCodePageEnum(cp);
    HeapFree( GetProcessHeap(), 0, cp );
    return TRUE;
}
