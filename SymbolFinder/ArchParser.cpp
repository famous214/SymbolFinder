#define _LITTLE_ENDIAN
#include "staticlib.h"
#include <malloc.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <memory.h> 
#include "utils.h"
#include <stdlib.h>
#include <string.h>

int strLenWithSpace(char * s, int n)
{
	char * p = s;
	while ((p - s) < n && *p != ' ')
		p++;
	return p - s;
}

void strCpySpaceToNull(char * des, char * src, int n)
{
	int count = 0;
	while (count < n && *src != ' ')
	{
		*des = *src;
		des++;
		src++;
		count++;
	}
	*des = NULL;
}

ArchParser::ArchParser()
{
	m_fileStart = NULL;
	memset(&this->m_fileDes, 0, sizeof(this->m_fileDes));
	m_objName = NULL;
}

ArchParser::~ArchParser()
{
	if (this->m_fileStart != NULL)
		free(this->m_fileStart);
	if (m_objName != NULL)
		free(m_objName);
}

void ArchParser::findSymbol(char * symbol, ArchSymbolFinder * finder)
{
	if (finder == NULL)
		return;
	DWORD count = *this->m_fileDes.linker2->symbolNum;
	char * pCur = this->m_fileDes.linker2->symbolTb;
	for (int i = 0; i < count; i++)
	{
		if (strstr(pCur, symbol) != NULL)
		{
			int index = m_fileDes.linker2->objIndiceTb[i] - 1;
			struct ArchiveHeader * hdr = getObjHdr(index);
			int len;
			this->getObjName(hdr, NULL, &len);
			if (m_objName == NULL)
			{
				this->m_objName = (char *)malloc(len + 1);
			}
			else
			{
				if (strlen(m_objName) < len)
				{
					free(m_objName);
					this->m_objName = (char *)malloc(len + 1);
				}
			}
			this->getObjName(hdr, this->m_objName, NULL);
			bool isContinue = true;
			finder->symbolFound(pCur, m_objName, &isContinue);
			if (!isContinue)
				return;
		}
		pCur = pCur + strlen(pCur) + 1;
	}
}

char * ArchParser::findSymbol(char * symbol)
{
	UINT32 count = *this->m_fileDes.linker2->symbolNum;
	char * pCur = this->m_fileDes.linker2->symbolTb;
	for (int i = 0; i < count; i++)
	{
		if (strcmp(pCur, symbol) == 0)
		{
			int index = m_fileDes.linker2->objIndiceTb[i] - 1;
			struct ArchiveHeader * hdr = getObjHdr(index);
			int len;
			this->getObjName(hdr, NULL, &len);
			if (m_objName == NULL)
			{
				this->m_objName = (char *)malloc(len + 1);
			}
			else
			{
				if (strlen(m_objName) < len)
				{
					free(m_objName);
					this->m_objName = (char *)malloc(len + 1);
				}
			}
			this->getObjName(hdr, this->m_objName, NULL);
			return m_objName;
		}
		pCur = pCur + strlen(pCur) + 1;
	}
	return NULL;
}

bool ArchParser::load(char * archFile)
{
	int h = open(archFile, _O_RDONLY);
	if (h == -1)
		return false;
	long size = filelength(h);
	close(h);
	FILE * fp = fopen(archFile, "rb");
	if (fp == NULL)
		return false;

	m_fileStart = (char *)malloc(size);
	if (m_fileStart == NULL)
		return false;
	char * pCur = m_fileStart;
	int count = size;
	int n;
	while (count > 0 && (n = fread(pCur, 1, count, fp)) > 0)
	{
		pCur += n;
		count -= n;
	}
	fclose(fp);
	if (!parse())
		return false;
	return true;
}

bool ArchParser::parse()
{
	if (memcmp(this->m_fileStart, ARCHIVE_START, ARCHIVE_START_LEN) != 0)
		return false;
	this->m_cur = this->m_fileStart;
	this->m_fileDes.start = this->m_fileStart;
	this->m_cur += ARCHIVE_START_LEN;
	if (!this->tryParseLinker1())
		return false;
	if (!this->tryParseLinker2())
		return false;
	this->tryParseLongNames();
	return true;
}

DWORD ArchParser::getMemberSize(struct ArchiveHeader * hdr)
{
	char size[ARCH_HDR_SIZE_LEN];
	memset(size, 0, ARCH_HDR_SIZE_LEN);
	for (int i = 0; i < ARCH_HDR_SIZE_LEN; i++)
		if (hdr->size[i] == ' ')
		{
			size[i] = NULL;
			break;
		}
		else
			size[i] = hdr->size[i];
	return atoi(size);
}

char * ArchParser::findNextMemberStart(struct ArchiveHeader * hdr, char * memberStart)
{
	char * p = memberStart + sizeof(struct ArchiveHeader) + getMemberSize(hdr);
	//也许这是微软编译器的奇怪行为，因为linker1, 2和longnames以及每个obj的最后都是串表，所以如果串表的最后一个字节后是‘0a’，则size不计算该字节，但下一个头要从0a后开始。我甚至怀疑连续的0a都不算。
	while (*p == '\n')
		p++;
	return p;
}

bool ArchParser::tryParseLinker1()
{
	if (memcmp(m_cur, LINKER_NAME, LINKER_NAME_LEN) != 0)
	{
		this->m_fileDes.linker1 = NULL;
		return false;
	}
	this->m_fileDes.linker1 = &(this->m_linker1);
	this->m_linker1.linkerHdr = (struct ArchiveHeader *)m_cur;
	m_cur += sizeof(ArchiveHeader);
	this->m_linker1.symbolNum = (PDWORD)m_cur;
	m_cur += sizeof(DWORD);
	this->m_linker1.offsetTb = (PDWORD)m_cur;
	int count = NTOHL(*this->m_linker1.symbolNum);
	m_cur += sizeof(DWORD) * count;
	this->m_linker1.sybmbolTb = (char *)m_cur;
	m_cur = findNextMemberStart(this->m_linker1.linkerHdr, (char *)(this->m_linker1.linkerHdr));
	return true;
}

int ArchParser::getPublicSymbolCount()
{
	if (this->m_fileDes.linker1 == NULL)
		return -1;
	return NTOHL(this->m_linker1.symbolNum);
}

int ArchParser::getObjCount()
{
	if (this->m_fileDes.linker2 == NULL)//只支持新格式,老格式可以从第一个obj开始遍历所有obj，算出长度
		return -1;
	return *this->m_fileDes.linker2->objNum;
}

struct ArchiveHeader* ArchParser::getObjHdr(int index)
{
	if (this->m_fileDes.linker2 == NULL)
		return NULL;
	int count = *this->m_fileDes.linker2->objNum;
	if (index >= count)
		return NULL;
	int offset = this->m_fileDes.linker2->objOffsetTb[index];
	struct ArchiveHeader* hdr = (struct ArchiveHeader*)(this->m_fileDes.start + offset);
	return hdr;
}

int ArchParser::getObjNameLen(int index)
{
	struct ArchiveHeader* hdr = getObjHdr(index);
	if (hdr == NULL)
		return -1;
	int ret;
	getObjName(hdr, NULL, &ret);
	return ret;
}

void ArchParser::getObjName(int index, char * name)
{
	struct ArchiveHeader* hdr = getObjHdr(index);
	if (hdr == NULL)
		*name = NULL;
	getObjName(hdr, name, NULL);
}


bool ArchParser::objNameIsLong(struct ArchiveHeader * hdr, int * offsetInLongName)
{
	char name[ARCH_HDR_NAME_LEN + 1];
	char *p = hdr->name;
	bool ret = false;
	if (*p == LONG_NAME_FLAG)
	{
		strCpySpaceToNull(name, hdr->name, ARCH_HDR_NAME_LEN);
		p++;
		*offsetInLongName = atoi(name + 1);//atoi对非法字串，也返回0所以要判断一下。
		if (*offsetInLongName == 0)
		{
			if (*p == '0' && (*(p + 1) == ' '))
				return true;
			else
				return false;
		}
		else
			return true;
	}
	return false;
}

void ArchParser::getObjName(struct ArchiveHeader * hdr, char * name, int * nameLen)
{
	char * p = hdr->name;
	int offsetInLongNames;
	if (!objNameIsLong(hdr, &offsetInLongNames))
	{
		if (nameLen != NULL)
		{
			*nameLen = strLenWithSpace(hdr->name, ARCH_HDR_NAME_LEN);
			return;
		}
		strCpySpaceToNull(name, hdr->name, ARCH_HDR_NAME_LEN);
	}
	else
	{
		if (nameLen != NULL)
		{
			*nameLen = strlen(this->m_fileDes.longNames->longNameTb + offsetInLongNames);
			return;
		}
		strcpy(name, this->m_fileDes.longNames->longNameTb + offsetInLongNames);
	}
}

/*
int ArchParser::getSymbolLen(int index)
{
if (this->m_fileDes.linker2 == NULL)
return -1;
this->m_fileDes.linker2->
}


void ArchParser::getSymbolName(int index, char * name)
{
}

*/

bool ArchParser::tryParseLinker2()
{
	if (memcmp(m_cur, LINKER_NAME, LINKER_NAME_LEN) != 0)
	{
		this->m_fileDes.linker2 = NULL;
		return false;//据微软文档，新格式必须有linker1来兼容老格式，但没有说老格式是否只有Linker1.这里暂时认为如果只有linker1，就是错误的
	}
	this->m_fileDes.linker2 = &(this->m_linker2);
	this->m_linker2.linkerHdr = (struct ArchiveHeader *)m_cur;
	m_cur += sizeof(ArchiveHeader);
	this->m_linker2.objNum = (PDWORD)m_cur;
	m_cur += sizeof(DWORD);
	this->m_linker2.objOffsetTb = (PDWORD)m_cur;
	m_cur += sizeof(DWORD) * (*this->m_linker2.objNum);
	this->m_linker2.symbolNum = (PDWORD)m_cur;
	m_cur += sizeof(DWORD);
	this->m_linker2.objIndiceTb = (PWORD)m_cur;
	m_cur += sizeof(WORD) * (*this->m_linker2.symbolNum);
	this->m_linker2.symbolTb = m_cur;
	m_cur = findNextMemberStart(this->m_linker2.linkerHdr, (char *)(this->m_linker2.linkerHdr));
	return true;
}

void ArchParser::tryParseLongNames()
{
	if (memcmp(m_cur, LONGNANME_NAME, LONGNANME_NAME_LEN) != 0)
	{
		this->m_fileDes.longNames = NULL;
		return;
	}
	this->m_fileDes.longNames = &(this->m_longNames);
	this->m_longNames.linkerHdr = (struct ArchiveHeader *)m_cur;
	m_cur += sizeof(ArchiveHeader);
	this->m_longNames.longNameTb = (char *)m_cur;
	m_cur = findNextMemberStart(this->m_longNames.linkerHdr, (char *)(this->m_longNames.linkerHdr));
	m_objMemberStart = m_cur;
}

void ArchParser::findObj(char * objName, ArchObjFinder * finder)
{
	if (finder == NULL)
		return;
	ArchHdr hdr;
	int objCount = getObjCount();
	for (int i = 0; i < objCount; i++)
	{
		hdr.assign(this, (char *)getObjHdr(i));
		char * name = hdr.getName();
		if (strstr(name, objName) != NULL)
		{
			bool isContinue = false;
			finder->objFound(&hdr, &isContinue);
			if (!isContinue)
				break;
		}
	}
}

void ArchHdr::assign(ArchParser* parser, char * objStart)
{
	struct ArchiveHeader * hdr = (struct ArchiveHeader *)objStart;
	m_objStart = objStart;
	this->size = ArchParser::getMemberSize(hdr);
	int len;
	parser->getObjName(hdr, NULL, &len);
	if (this->m_name != NULL)
		free(m_name);
	this->m_name = (char *)malloc(len + 1);
	parser->getObjName(hdr, this->m_name, NULL);
	strCpySpaceToNull(this->groupId, hdr->groupId, ARCH_HDR_GID_LEN);
	strCpySpaceToNull(this->userId, hdr->groupId, ARCH_HDR_UID_LEN);
	strCpySpaceToNull(this->mode, hdr->mode, ARCH_HDR_MODE_LEN);
}


ArchHdr::~ArchHdr()
{
	if (this->m_name != NULL)
		free(m_name);
}

ObjExport::~ObjExport()
{
	//	if (this->m_file == NULL)
	//		free(this->m_file);
}

/*
void ObjExport::setExportFileName(char * name)
{
if (this->m_file == NULL)
free(this->m_file);
m_file = (char *)malloc(strlen(name) + 1);
strcpy(m_file, name);
}
*/
void ObjExport::objFound(ArchHdr * hdr, bool * isContinue)
{
	this->isfound = true;
	*isContinue = !stopIfFound;
	char exportedFile[512];
	printf("please input exported file name\n");
	while (scanf("%s", exportedFile) != 1)
		printf("please input exported file name\n");
	char * start = hdr->getObjStart();
	FILE * fp = fopen(exportedFile, "wb");
	start += sizeof(struct ArchiveHeader);
	UINT32 count = hdr->size;
	while (count > 0)
	{
		count -= fwrite(start, 1, count, fp);
	}
	fclose(fp);
}

void SymbolPrinter::symbolFound(char * symbolFound, char * objName, bool * isContinue)
{
	*isContinue = true;
	printf("symbol %s obj %s \n", symbolFound, objName);
}