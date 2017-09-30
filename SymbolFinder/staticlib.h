#pragma once
#ifndef STATICLIB_XXXX

#define STATICLIB_XXXX

#include <stdio.h>
#include <windows.h>

#define ARCHIVE_START "!<arch>\n"
#define ARCHIVE_START_LEN 8
#define LINKER_NAME "/ "
#define LINKER_NAME_LEN 2
#define LONGNANME_NAME "// "
#define LONGNANME_NAME_LEN 3
#define ARCH_HDR_SIZE_LEN 10
#define ARCH_HDR_NAME_LEN 16
#define ARCH_HDR_GID_LEN 6
#define ARCH_HDR_UID_LEN 6
#define ARCH_HDR_MODE_LEN 8
#define LONG_NAME_FLAG '/'

#pragma pack(push, 1)

struct ArchiveHeader {
	char name[ARCH_HDR_NAME_LEN];
	char timedate[12];
	char userId[ARCH_HDR_UID_LEN];
	char groupId[ARCH_HDR_GID_LEN];
	char mode[ARCH_HDR_MODE_LEN];
	char size[ARCH_HDR_SIZE_LEN];
	char end[2];
};

#pragma pack(pop)


struct Linker1
{
	struct ArchiveHeader* linkerHdr;
	PDWORD symbolNum;//big_endian
	PDWORD offsetTb;//big_endian
	char * sybmbolTb;
};

struct Linker2
{
	struct ArchiveHeader* linkerHdr;
	PDWORD objNum;//没有特殊说明，可能是本地顺序
	PDWORD objOffsetTb;//没有特殊说明，可能是本地顺序
	PDWORD symbolNum;//没有特殊说明，可能是本地顺序
	PWORD objIndiceTb;
	char * symbolTb;
};

struct LongNames
{
	struct ArchiveHeader * linkerHdr;
	char * longNameTb;
};

struct ArchFile
{
	char * start;
	struct Linker1* linker1;
	struct Linker2* linker2;
	struct LongNames* longNames;
};

class ArchObjFinder;

class ArchSymbolFinder;

class ArchParser
{
public:
	ArchParser();
	~ArchParser();
	bool load(char * archFile);
	//struct ArchFile * getFileDes(){ return &m_fileDes; }
	int getPublicSymbolCount();
	static DWORD getMemberSize(struct ArchiveHeader * hdr);
	int getObjCount();
	//int getSymbolLen(int index);
	//void getSymbolName(int index, char * name);
	int getObjNameLen(int index);
	void getObjName(int index, char * name);
	void getObjName(struct ArchiveHeader * hdr, char * name, int * nameLen);
	void findObj(char * objName, ArchObjFinder * finder);
	char * findSymbol(char * symbol);//返回为模块名字
	void findSymbol(char * symbol, ArchSymbolFinder * finder);
private:
	struct ArchFile m_fileDes;
	char * m_fileStart;
	bool parse();
	bool tryParseLinker1();
	bool tryParseLinker2();
	void tryParseLongNames();
	char * findNextMemberStart(struct ArchiveHeader * hdr, char * memberStart);
	bool ArchParser::objNameIsLong(struct ArchiveHeader * hdr, int * offsetInLongName);
	struct ArchiveHeader* getObjHdr(int index);
	char * m_cur;
	char *m_objMemberStart;
	struct Linker1 m_linker1;
	struct Linker2 m_linker2;
	struct LongNames m_longNames;
	char * m_objName;
};

class ArchHdr
{
public:
	ArchHdr() { m_name = NULL; };
	ArchHdr(ArchParser* parser, char * objStart);
	~ArchHdr();
	void assign(ArchParser* parser, char * objStart);
	char * getName() { return m_name; }
	char * getObjStart() { return m_objStart; }
	//char timedate[12];
	char userId[ARCH_HDR_UID_LEN + 1];
	char groupId[ARCH_HDR_GID_LEN + 1];
	char mode[ARCH_HDR_MODE_LEN + 1];
	DWORD size;
private:
	char* m_name;
	char * m_objStart;
};


class ArchSymbolFinder
{
public:
	virtual void symbolFound(char * symbolFound, char * objName, bool * isContinue) = 0;
};

class SymbolPrinter : public ArchSymbolFinder
{
public:
	virtual void symbolFound(char * symbolFound, char * objName, bool * isContinue);
};

class ArchObjFinder
{
public:
	virtual void objFound(ArchHdr * hdr, bool * isContinue) = 0;
};

class ObjExport : public ArchObjFinder
{
public:
	ObjExport() { isfound = false; stopIfFound = true; }
	~ObjExport();
	//void setExportFileName(char * file);
	virtual void objFound(ArchHdr * hdr, bool * isContinue);
	bool stopIfFound;
	bool isfound;
private:
	//char * m_file;	
};

#endif