#define _FILE_OFFSET_BITS 64

#include "SubTransFormat.h"
#include "MD5Checksum.h"
#include "Charconvert.h"
#include "shooterclient.key"

#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>


#define SVP_REV_NUMBER	0
#define UNIQU_HASH_SIZE 512

SubTransFormat::SubTransFormat(void)
{
}

SubTransFormat::~SubTransFormat(void)
{
}

std::wstring SubTransFormat::GetShortFileNameForSearch2(std::wstring szFn)
{
	std::wstring szFileName = szFn;
	int posDot = szFileName.find_last_of('.');
	szFileName = szFileName.substr(0, posDot);

	std::vector<std::wstring> szaStopWords;
	szaStopWords.push_back(L"blueray");
	szaStopWords.push_back(L"bluray");
	szaStopWords.push_back(L"dvdrip");
	szaStopWords.push_back(L"xvid");
	szaStopWords.push_back(L"cd1");
	szaStopWords.push_back(L"cd2");
	szaStopWords.push_back(L"cd3");
	szaStopWords.push_back(L"cd4");
	szaStopWords.push_back(L"cd5");
	szaStopWords.push_back(L"cd6");
	szaStopWords.push_back(L"vc1");
	szaStopWords.push_back(L"vc-1");
	szaStopWords.push_back(L"hdtv");
	szaStopWords.push_back(L"1080p");
	szaStopWords.push_back(L"720p");
	szaStopWords.push_back(L"1080i");
	szaStopWords.push_back(L"x264");
	szaStopWords.push_back(L"stv");
	szaStopWords.push_back(L"limited");
	szaStopWords.push_back(L"ac3");
	szaStopWords.push_back(L"xxx");
	szaStopWords.push_back(L"hddvd");

	std::transform(szFileName.begin(), szFileName.end(),
		szFileName.begin(), tolower);

	for (size_t i = 0 ; i < szaStopWords.size(); i++)
	{
		int pos = szFileName.find(szaStopWords[i]);
		if(pos != szFileName.npos)
			szFileName = szFileName.substr(0, pos - 1);
	}


	std::wstring szReplace = L"[].-#_=+<>,";
	for (size_t i = 0; i < szReplace.length(); i++)
		for (size_t j = 0; j < szFileName.length(); j++)
			if (szFileName[j] == szReplace[i])
				szFileName[j] = ' ';

	szFileName.erase(szFileName.find_last_not_of(L' ') + 1);
	szFileName.erase(0, szFileName.find_first_not_of(L' '));

	if (szFileName.length() > 1)
		return szFileName;

	return L"";
}

std::wstring SubTransFormat::GetShortFileNameForSearch(std::wstring szFnPath)
{
	std::wstring szFileName;


	std::wstring::size_type pos;
	pos=szFnPath.find_last_of(L'/');
	if(pos!=szFnPath.npos)
		szFileName = szFnPath.substr(szFnPath.find_last_of(L'/'));
	
	pos=szFnPath.find_last_of(L'\\');
	if(pos!=szFnPath.npos)
		szFileName = szFnPath.substr(szFnPath.find_last_of(L'\\'));
	pos = szFileName.find_last_of(L'.');
	std::wstring szFileName2 = szFileName.substr(0, pos);

	szFileName = GetShortFileNameForSearch2(szFileName2);
	if(!szFileName.empty())
		return szFileName;

	return szFnPath;
}



std::wstring SubTransFormat::ComputerFileHash_STL(char* szFilePath)
{

	std::wstring szRet = L"";
	int64_t offset[4];


 
	if (szRet.empty())
	{
		int stream;
		//errno_t err;
		//unsigned long timecost = GetTickCount();
		//err =	 _wsopen_s(&stream, szFilePath.c_str(),
		// _O_BINARY|_O_RDONLY , _SH_DENYNO , _S_IREAD);

		FILE *fp=fopen(szFilePath,"rb");
		if(fp==NULL){printf("open video file error!\n!");exit(1);}
		stream=fileno(fp);
		//if (!err)
		if(fp!=NULL)
		{	struct stat buf;
			fstat(stream,&buf);
			int64_t ftotallen  = buf.st_size;
			if (ftotallen < 8192)
			{
				//a video file less then 8k? impossible!
			}
			else
			{
				offset[3] = ftotallen - 8192;
				offset[2] = ftotallen / 3;
				offset[1] = ftotallen / 3 * 2;
				offset[0] = 4096;
				CMD5Checksum mMd5;
				unsigned char bBuf[4096];
				for(int i = 0; i < 4;i++)
				{
					lseek(stream, offset[i], 0);
					//hash 4k block
					int readlen = read( stream, bBuf, 4096);
					std::wstring szMD5 = mMd5.GetMD5(bBuf, readlen); 
					if(!szRet.empty())
						szRet.append(L";");
					szRet.append(szMD5);
				}
			}
			//_close(stream);
		}
		//timecost =  GetTickCount() - timecost;
	}



	//szFilePath.Format(_T("Vid Hash Cost %d milliseconds "), timecost);
	//SVP_LogMsg(szFilePath);
	return szRet;
}

std::wstring SubTransFormat::genVHash(const char* szTerm2, const char* szTerm3)
{
	
	char buffx[4096],uniqueIDHash[UNIQU_HASH_SIZE];
	
	memset(buffx, 0, 4096);
	
	memset(uniqueIDHash,0,UNIQU_HASH_SIZE);
#ifdef CLIENTKEY	
	snprintf( buffx, 4096, CLIENTKEY , SVP_REV_NUMBER, szTerm2, szTerm3, uniqueIDHash);
#else
	snprintf( buffx, 4096, "un authiority client %d %s %s %s", SVP_REV_NUMBER, szTerm2, szTerm3, uniqueIDHash);
#endif
	CMD5Checksum cmd5;
	return cmd5.GetMD5((unsigned char*)buffx, strlen(buffx));
	
}

/*std::wstring SubTransFormat::genVHash(const wchar_t* szTerm2, const wchar_t* szTerm3)//�����������
  {

  char buffx[4096],uniqueIDHash[UNIQU_HASH_SIZE];

  memset(buffx, 0, 4096);

  memset(uniqueIDHash,0,UNIQU_HASH_SIZE);
  #ifdef CLIENTKEY	
  sprintf_s( buffx, 4096, CLIENTKEY , SVP_REV_NUMBER, szTerm2, szTerm3, uniqueIDHash);
  #else
  //���������������������Ϊwchar*,��swprintf_s����������������char*
  snprintf( buffx, 4096, "un authiority client %d %s %s %s", SVP_REV_NUMBER, szTerm2, szTerm3, uniqueIDHash);
  #endif
  CMD5Checksum cmd5;
  return cmd5.GetMD5((unsigned char*)buffx, strlen(buffx));

  }*/
