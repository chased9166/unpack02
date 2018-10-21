//#include "stdafx.h"
#include <windows.h>
#include "CPeParser.h"

void decompress(void* src, void* dest)
{
	_asm {
pushad
mov     esi, src
mov     edi, dest
push    edi
or ebp, 0FFFFFFFFh
jmp     short loc_43ACF2
; -------------------------------------------------------------------------- -
; align 8

loc_43ACE8:; CODE XREF : UPX1:loc_43ACF9j
mov     al, [esi]
inc     esi
mov[edi], al
inc     edi

loc_43ACEE : ; CODE XREF : UPX1:0043AD86j
; UPX1:0043AD9Dj
add     ebx, ebx
jnz     short loc_43ACF9

loc_43ACF2 : ; CODE XREF : UPX1:0043ACE0j
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx

loc_43ACF9 : ; CODE XREF : UPX1:0043ACF0j
jb      short loc_43ACE8
mov     eax, 1

loc_43AD00:; CODE XREF : UPX1:0043AD0Fj
; UPX1:0043AD1Aj
add     ebx, ebx
jnz     short loc_43AD0B
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx

loc_43AD0B : ; CODE XREF : UPX1:0043AD02j
adc     eax, eax
add     ebx, ebx
jnb     short loc_43AD00
jnz     short loc_43AD1C
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx
jnb     short loc_43AD00

loc_43AD1C : ; CODE XREF : UPX1:0043AD11j
xor     ecx, ecx
sub     eax, 3
jb      short loc_43AD30
shl     eax, 8
mov     al, [esi]
inc     esi
xor     eax, 0FFFFFFFFh
jz      short loc_43ADA2
mov     ebp, eax

loc_43AD30 : ; CODE XREF : UPX1:0043AD21j
add     ebx, ebx
jnz     short loc_43AD3B
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx

loc_43AD3B : ; CODE XREF : UPX1:0043AD32j
adc     ecx, ecx
add     ebx, ebx
jnz     short loc_43AD48
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx

loc_43AD48 : ; CODE XREF : UPX1:0043AD3Fj
adc     ecx, ecx
jnz     short loc_43AD6C
inc     ecx

loc_43AD4D : ; CODE XREF : UPX1:0043AD5Cj
; UPX1:0043AD67j
add     ebx, ebx
jnz     short loc_43AD58
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx

loc_43AD58 : ; CODE XREF : UPX1:0043AD4Fj
adc     ecx, ecx
add     ebx, ebx
jnb     short loc_43AD4D
jnz     short loc_43AD69
mov     ebx, [esi]
sub     esi, 0FFFFFFFCh
adc     ebx, ebx
jnb     short loc_43AD4D

loc_43AD69 : ; CODE XREF : UPX1:0043AD5Ej
add     ecx, 2

loc_43AD6C:; CODE XREF : UPX1:0043AD4Aj
cmp     ebp, 0FFFFF300h
adc     ecx, 1
lea     edx, [edi + ebp]
cmp     ebp, 0FFFFFFFCh
jbe     short loc_43AD8C

loc_43AD7D : ; CODE XREF : UPX1:0043AD84j
mov     al, [edx]
inc     edx
mov[edi], al
inc     edi
dec     ecx
jnz     short loc_43AD7D
jmp     loc_43ACEE
; -------------------------------------------------------------------------- -
align 4

loc_43AD8C:; CODE XREF : UPX1:0043AD7Bj
; UPX1:0043AD99j
mov     eax, [edx]
add     edx, 4
mov[edi], eax
add     edi, 4
sub     ecx, 4
ja      short loc_43AD8C
add     edi, ecx
jmp     loc_43ACEE
; -------------------------------------------------------------------------- -

loc_43ADA2:; CODE XREF : UPX1:0043AD2Cj
pop     esi
mov     edi, esi
mov     ecx, 0C0Fh

loc_43ADAA : ; CODE XREF : UPX1:0043ADB1j
; UPX1:0043ADB6j
mov     al, [edi]
inc     edi
sub     al, 0E8h

loc_43ADAF : ; CODE XREF : UPX1:0043ADD4j
cmp     al, 1
ja      short loc_43ADAA
cmp     byte ptr[edi], 11h
jnz     short loc_43ADAA
mov     eax, [edi]
mov     bl, [edi + 4]
shr     ax, 8
rol     eax, 10h
xchg    al, ah
sub     eax, edi
sub     bl, 0E8h
add     eax, esi
mov[edi], eax
add     edi, 5
mov     al, bl
loop    loc_43ADAF
popad
	}
}

//int main()
//{
//	byte* pSrc=NULL;
//	int nSize=sizeof(bySrcBuffer);
//	byte* pDest=new byte[nSize*3];
//	memset(pDest,0, nSize * 0x3);
//
//	decompress(bySrcBuffer,pDest);
//
//
//    return 0;
//}

void unpack(const char* pszPath) {
	// 1. 读取文件到内存中,在内存中解析PE文件结构
	CPeParser obj(pszPath);
	obj.loadFileToMem();

	// 2. 在内存中解压缩代码和数据
	byte* pSrc = (byte*)obj.m_pMemBuffer + obj.m_SectionInfo[1].VirtualAddress;
	byte* pDest = (byte*)obj.m_pMemBuffer + obj.m_SectionInfo[0].VirtualAddress;
	decompress(pSrc, pDest);

	// 3. 在内存中获取IAT，获取导入表相关信息,还原导入表
	DWORD dwPackImportOffset = obj.FindMem(obj.m_pbyCode1, sizeof(obj.m_pbyCode1));
	DWORD dwDllNameOffset = obj.FindMem(obj.m_pbyCode2, sizeof(obj.m_pbyCode2));
	obj.parseIATAndFixImport(dwPackImportOffset, dwDllNameOffset);
}

int main()
{
	//char szPath[MAX_PATH] = { 0 };
	//printf("please input file name: ");
	//scanf_s("%s", szPath, MAX_PATH);
	//unpack(szPath);

	unpack("hello15pb.exe");

	return 0;
}