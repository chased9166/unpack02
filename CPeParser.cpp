#include "CPeParser.h"

#define PEHEADER_SIZE 0x400

CPeParser::CPeParser(const char* pFilePath)
{
	strcpy_s(m_pFilePath, pFilePath);
}

CPeParser::~CPeParser()
{
}


BOOL CPeParser::loadFileToMem() {
	// 0. ��Ҫ���
	if (m_pFilePath[0] == 0) {
		return FALSE;
	}

	if (m_pMemBuffer) {
		delete m_pMemBuffer;
		m_pMemBuffer = NULL;
	}

	FILE* fp = fopen(m_pFilePath, "rb");

	if (fp == NULL) {
		printf("fopen err");
		return FALSE;
	}
	// 1. ��ȡPEͷ��ȡ��Ҫ��Ϣ
	byte* pPEHeader = new byte[PEHEADER_SIZE];
	memset(pPEHeader,0, PEHEADER_SIZE);
	int nRet = fread(pPEHeader, PEHEADER_SIZE, 1, fp);
	if(nRet == 0){
		printf("fopen err");
		return FALSE;
	}
	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pPEHeader;
	IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS *)(pPEHeader + pDosHeader->e_lfanew);
	m_pOptionalHead = (IMAGE_OPTIONAL_HEADER32 *)(&pNtHeader->OptionalHeader);
	IMAGE_SECTION_HEADER *pSectonInfo = (IMAGE_SECTION_HEADER *)((LPBYTE)m_pOptionalHead + pNtHeader->FileHeader.SizeOfOptionalHeader);
	int i = 0;
	while (pSectonInfo[i].Name[0] != 0)
	{
		memcpy(&m_SectionInfo[i], &pSectonInfo[i], sizeof(IMAGE_SECTION_HEADER));
		i++;
	}
	m_SectionCount = i;
	m_dwEP = m_pOptionalHead->AddressOfEntryPoint;
	m_dwMemAlign = m_pOptionalHead->SectionAlignment;
	m_dwFileAlign = m_pOptionalHead->FileAlignment;
	m_dwImageSize = m_pOptionalHead->SizeOfImage;
	m_dwImageBase = m_pOptionalHead->ImageBase;
	// 2. ��ȡӳ���ܴ�С�����������ռ�
	m_pMemBuffer = new byte[m_dwImageSize];
	memset(m_pMemBuffer, 0, m_dwImageSize);
	// 3. ����PEͷ��m_pMemBuffer��
	memcpy(m_pMemBuffer, pPEHeader, PEHEADER_SIZE);
	delete pPEHeader; pPEHeader = NULL;
	// 4. ��ȡÿһ�����ε����ݵ�m_pMemBuffer��
	for (size_t i = 0; i < m_SectionInfo[i].Name[0] != 0; i++)
	{
		DWORD dwOffet = m_SectionInfo[i].VirtualAddress;
		fseek(fp, m_SectionInfo[i].PointerToRawData, SEEK_SET);
		int nReadSize = m_SectionInfo[i].SizeOfRawData;
		
		if (nReadSize == 0)
			continue;
		int nRet = fread(m_pMemBuffer + dwOffet, nReadSize, 1, fp);
		if (nRet == 0) {
			printf("fread err");
			return FALSE;
		}
	}

	return TRUE;
}

DWORD CPeParser::FindMem(BYTE* pbyCode, DWORD codeLen) {

	DWORD bufLen = m_dwImageSize;
	byte* pBase = m_pMemBuffer;
	DWORD dwSize = bufLen;
	DWORD dwOffset = 0;
	for (size_t i = 0; i < dwSize; i++)
	{
		BOOL bFind = TRUE;
		DWORD j = 0;
		for (; j < codeLen; j++)
		{
			if (pBase[i + j] != pbyCode[j]) {
				bFind = FALSE;
			}
		}
		if (bFind) {
			dwOffset = i + j;
			break;
		}
	}
	if (dwOffset)
	{
		DWORD dwData = *(DWORD*)(pBase + dwOffset);
		return dwData;
	}

	return 0;
}


DWORD CPeParser::FindMem2(byte* pBase, DWORD size, BYTE* pbyCode, DWORD codeLen) {

	DWORD dwSize = size;
	DWORD dwOffset = 0;
	for (size_t i = 0; i < dwSize; i++)
	{
		BOOL bFind = TRUE;
		DWORD j = 0;
		for (; j < codeLen; j++)
		{
			if (pBase[i + j] != pbyCode[j]) {
				bFind = FALSE;
			}
		}
		if (bFind) {
			dwOffset = i + j;
			break;
		}
	}
	return dwOffset;
}


DWORD CPeParser::FindMem3(byte* pBase, DWORD size, BYTE* pbyCode, DWORD codeLen) {

	DWORD dwSize = size;
	DWORD dwOffset = 0;
	for (size_t i = 0; i < dwSize; i++)
	{
		BOOL bFind = TRUE;
		DWORD j = 0;
		for (; j < codeLen; j++)
		{
			if (pBase[i + j] != pbyCode[j]) {
				bFind = FALSE;
			}
		}
		if (bFind) {
			dwOffset = i + j;
			break;
		}
	}

	if (dwOffset)
	{
		DWORD dwEIP = (DWORD)pBase + dwOffset;
		BYTE byCode = *(BYTE*)(pBase + dwOffset);
		// upx ��ȡ OEP��ַ  jmp xxxx
		DWORD dwOEP;
		if (byCode == 0xE9) {
			DWORD dwData = *(DWORD*)(pBase + dwOffset + 1);
			dwOEP = dwEIP + dwData + 5;
		}
		return dwOEP;
	}
	return dwOffset;
}

DWORD CPeParser::parseIATAndFixImport(DWORD dwImportOffset, DWORD dwDllNameOffset) {

	// ��ȡ�Ǳ���ĵ�����Ϣ��ַ
	DWORD dwBase = (DWORD)m_pMemBuffer + m_SectionInfo[0].VirtualAddress;
	DWORD dwAddr = dwBase + dwImportOffset;

	// �ṹ
	// dwDllNameOffset 
	// dwIatRVA
	// 01 funname,0,01 funame,0...,0,0
	// FF,���,0,.......,0,0
	// ���Զ�ȡ��dll���ƺ�funname,д�뵽ԭPE�ļ����ڵ�λ��
	DWORD dwOffset = *(DWORD*)dwAddr;
	DWORD dwDllNameAddr = dwBase + dwOffset + dwDllNameOffset;
	dwOffset = *(DWORD*)(dwAddr+4);
	DWORD dwIATAddr = dwBase + dwOffset;
	DWORD dwFunNameAddr = dwAddr + 8;

	// Ѱ��PEͷ
	DWORD dwSize = m_dwImageSize- (dwAddr- (DWORD)m_pMemBuffer);
	DWORD dwPE = FindMem2((byte*)dwAddr, dwSize,m_pbyCode3, sizeof(m_pbyCode3));
	IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS *)(dwAddr+dwPE-4);
	IMAGE_OPTIONAL_HEADER32 *pOptionalHead = (IMAGE_OPTIONAL_HEADER32 *)(&pNtHeader->OptionalHeader);
	IMAGE_DATA_DIRECTORY* pDataDir = (IMAGE_DATA_DIRECTORY*)pOptionalHead->DataDirectory;
	IMAGE_SECTION_HEADER *pSectonInfo = (IMAGE_SECTION_HEADER *)((LPBYTE)pOptionalHead + pNtHeader->FileHeader.SizeOfOptionalHeader);

	IMAGE_IMPORT_DESCRIPTOR* pImport = (IMAGE_IMPORT_DESCRIPTOR*)((DWORD)m_pMemBuffer + pDataDir[1].VirtualAddress);
	
	// ѭ����dll����д�뵼���ṹ�ж�ӦRVAָ���λ��
	// ѭ������������д�뵼���ṹ��IAT��ӦRVAָ���λ�ò��������
	byte byCode = 0;
	do
	{
		// ����dll����
		char* pDllName = (char*)(m_pMemBuffer + pImport->Name);
		strcpy(pDllName, (char*)dwDllNameAddr);
		// ����IAT��RVA
		pImport->FirstThunk = dwIATAddr - (DWORD)m_pMemBuffer;

		int index = 0;
		IMAGE_THUNK_DATA* pIATData = (IMAGE_THUNK_DATA*)((DWORD)m_pMemBuffer + pImport->FirstThunk);
		do
		{
			// ����������ַ
			// ��ȡiatData
			DWORD dwIatRVA = *(DWORD*)((DWORD)pIATData + index * 4);
			DWORD dwFlag = dwIatRVA & 0x80000000;
			if (dwFlag != 0) {
				dwFunNameAddr += 3;

				index++;
				byCode = *(byte*)dwFunNameAddr;
				if (byCode == 0) {
					dwFunNameAddr++;
					break;
				}
				continue;
			}

			_IMAGE_IMPORT_BY_NAME* pName = (_IMAGE_IMPORT_BY_NAME*)((DWORD)m_pMemBuffer + dwIatRVA);

			byCode = *(byte*)dwFunNameAddr;
			if (byCode == 1) {
				dwFunNameAddr++;
				pName->Hint = index;
				int nLen = strlen((char*)dwFunNameAddr);
				strcpy(pName->Name, (char*)dwFunNameAddr);
				dwFunNameAddr = dwFunNameAddr + nLen + 1;
			}

			index++;
			byCode = *(byte*)dwFunNameAddr;
			if (byCode == 0) {
				dwFunNameAddr++;
				break;
			}

		} while (true);
		
		
		byCode = *(byte*)dwFunNameAddr;
		if (byCode == 0) {
			break;
		}
		pImport++;

		dwDllNameAddr = dwBase + *(DWORD*)dwFunNameAddr + dwDllNameOffset;
		dwIATAddr = dwBase + *(DWORD*)(dwFunNameAddr+4);
		dwFunNameAddr+=8;
	} while (true);

	pImport++;
	memset(pImport,0,sizeof(IMAGE_IMPORT_DESCRIPTOR));

	IMAGE_DATA_DIRECTORY* pDataDir1 = GetDataDir();
	memcpy(&pDataDir1[1], &pDataDir[1], 0x8);

	// ��ȡ��ǰPEͷ����Դ��ƫ��
	// ��ԭPEͷ����������PEͷ���ָ�PEͷ��Ϣ
	DumpMemory();
	return 0;
}

IMAGE_DATA_DIRECTORY* CPeParser::GetDataDir() {

	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)m_pMemBuffer;
	IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS *)(m_pMemBuffer + pDosHeader->e_lfanew);
	IMAGE_OPTIONAL_HEADER32 *pOptionalHead = (IMAGE_OPTIONAL_HEADER32 *)(&pNtHeader->OptionalHeader);


	return pOptionalHead->DataDirectory;
}

void CPeParser::DumpMemory() {

	DWORD dwOEP = FindMem3((byte*)m_pMemBuffer+m_dwEP, m_SectionInfo[1].SizeOfRawData, m_byCode,sizeof(m_byCode));

	// �����ṹ��
	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)m_pMemBuffer;      //DOSͷ
	IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS *)(m_pMemBuffer + pDosHeader->e_lfanew);      //NTͷ
	IMAGE_OPTIONAL_HEADER32 *pOptionalHead = (IMAGE_OPTIONAL_HEADER32 *)(&pNtHeader->OptionalHeader);    //��ѡͷ
	IMAGE_SECTION_HEADER *pSectonInfo = (IMAGE_SECTION_HEADER *)((LPBYTE)pOptionalHead + pNtHeader->FileHeader.SizeOfOptionalHeader);

	// 1. �޸��ļ���С���ڴ��Сһ��(dump֮���ļ�����Ҳ��0x1000)
	int i = 0;
	while (pSectonInfo[i].Name[0] != 0)
	{
		pSectonInfo[i].SizeOfRawData = pSectonInfo[i].Misc.VirtualSize;
		pSectonInfo[i].PointerToRawData = pSectonInfo[i].VirtualAddress;
		i++;
	}

	// 2. �޸�OEP
	pNtHeader->OptionalHeader.AddressOfEntryPoint = dwOEP - (DWORD)m_pMemBuffer;
	// 3. ȥ�������ַ
	pOptionalHead->DllCharacteristics &= 0x81;

	// 3. д�ļ������ڴ�����
	char szDumpPath[MAX_PATH];
	memset(szDumpPath, 0, MAX_PATH);
	strcpy(szDumpPath, m_pFilePath);

	const char * extension = 0;
	char* dot = strchr(szDumpPath, '.');
	if (dot)
	{
		*dot = L'\0';
		extension = m_pFilePath + (dot - szDumpPath); //wcsrchr(selectedFilePath, L'.');
	}

	strcat(szDumpPath, "_pb.exe");

	SaveFile(m_pMemBuffer, m_dwImageSize, szDumpPath);

}

bool CPeParser::SaveFile(byte * buf, int len, const char * filename)
{
	//�������
	HANDLE hfile = CreateFileA(filename,
		GENERIC_READ | GENERIC_WRITE, //�����д����
		FILE_SHARE_WRITE | FILE_SHARE_READ, //�������д����
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("Can't open\r\n");
		return false;
	}
	//�����ָ���ļ���
	SetFilePointer(hfile, 0, 0, FILE_BEGIN);
	DWORD dwWritten;     //����д�˶����ֽڵ��ļ���
	WriteFile(hfile, buf, len, &dwWritten, 0);
	//������д���ļ� 
	CloseHandle(hfile);
	return   true;
}