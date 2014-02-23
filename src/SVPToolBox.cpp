﻿#include "SVPToolBox.h"
#include "Charconvert.h"
#include "MD5Checksum.h"
#include <stdio.h>
#include <wchar.h> 
#include <algorithm>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#define SVP_MIN(a, b)  (((a) < (b)) ? (a) : (b))

CSVPToolBox::CSVPToolBox(void)
{
}

CSVPToolBox::~CSVPToolBox(void)
{
}


#define SVPATH_BASENAME 0  //Without Dot
#define SVPATH_EXTNAME 1  //With Dot
#define SVPATH_DIRNAME 2 //With Slash
#define SVPATH_FILENAME 3  //Without Dot

std::wstring CSVPToolBox::getVideoFileBasename(std::wstring szVidPath, std::vector<std::wstring>* szaPathInfo = NULL)
{

  //CPath szTPath(szVidPath.c_str());
  int posDot    = szVidPath.find_last_of(L'.');
  int posSlash  = szVidPath.find_last_of(L'\\');
  int posSlash2 = szVidPath.find_last_of(L'/');
  if (posSlash2 > posSlash)
    posSlash = posSlash2;

  if(posDot > posSlash)
  {
    if (szaPathInfo != NULL)
    {
      std::wstring szBaseName = szVidPath.substr(0, posDot);
      std::wstring szExtName  = szVidPath.substr(posDot, szVidPath.size() - posDot);
      std::transform(szExtName.begin(), szExtName.end(), szExtName.begin(),::tolower);
      std::wstring szFileName = szVidPath.substr(posSlash+1, (posDot - posSlash - 1));
      std::wstring szDirName  = szVidPath.substr(0, posSlash + 1);
      szaPathInfo -> clear();
      szaPathInfo -> push_back(szBaseName); // Base Name
        

      szaPathInfo -> push_back(szExtName ); //ExtName
      szaPathInfo -> push_back(szDirName); //Dir Name ()
      szaPathInfo -> push_back(szFileName); // file name only
      
    }
    return szVidPath.substr(posDot);
  }
  return szVidPath;
}





void CSVPToolBox::filePutContent_STL(std::wstring szFilePath, std::wstring szData, bool bAppend)
{
  FILE* f;
  if ( (f=fopen(WideChar2MultiByte(szFilePath.c_str()), "w"))=NULL)
  {
    fwrite(szData.c_str(), sizeof(wchar_t), szData.length(), f);
    fclose(f);
  }
}


int CSVPToolBox::unpackGZfile(char* fnin , char* fnout)
{
	
	FILE* fout;
	int ret = 0;
	if ( (fout=fopen(fnout, "wb"))== NULL){
		return -1; //output file open error
	}

    gzFile gzfIn = gzopen( fnin , "rb");
	if (gzfIn){
	
		char buff[4096];
		int iBuffReadLen ;
		do{
            iBuffReadLen = gzread(gzfIn, buff, 4096 );
			if(iBuffReadLen > 0 ){
				if( fwrite(buff, sizeof( char ),  iBuffReadLen, fout) <= 0 ){
				    //file write error
					ret = 1;
				}
			}else if(iBuffReadLen < 0){
				ret = -3; //decompress error
				break;
			}else{
				break;
			}
		}while(1);

		fclose(fout);
        gzclose(gzfIn);
	}else{
		ret = -2; //gz file open error
    }

	if (ret != 0 ){
		printf("gzip error");
	}
	return ret;
}



std::wstring CSVPToolBox::DetectSubFileLanguage_STL(std::wstring fn)
{
  std::wstring szRet = L".chn";
  FILE *stream ;
  if ( (stream=fopen(WideChar2MultiByte(fn.c_str()), "rb")) ==NULL)
  {
    //detect bom?
    int totalWideChar = 0;
    int totalChar     = 0;
    int ch;

    for(int i=0; (feof( stream ) == 0 ); i++)
    {
      ch = 0xff & fgetc(stream);
      if (ch >= 0x80 )
        totalWideChar++;
      totalChar++;
    }

    fclose( stream );

    if(totalWideChar < (totalChar / 10) && totalWideChar < 1700)
      szRet = L".eng";
      }
  return szRet;
}





int CSVPToolBox::HandleSubPackage(FILE* fp){
	//Extract Package
    printf( "Extracting package\n" );


	char szSBuff[8];

	if ( fread(szSBuff , sizeof(char), 4, fp) < 4){
		
        printf("Fail to retrive Package Data Length\n" );
		return -1;
	}
    Char4ToInt(szSBuff);
	
	if ( fread(szSBuff , sizeof(char), 4, fp) < 4){
        printf("Fail to retrive Desc Data Length\n");
		return -2;
	}

	
	size_t iDescLength = this->Char4ToInt(szSBuff);

	if(iDescLength > 0){

		char * szDescData = this->ReadToPTCharByLength(fp, iDescLength);
		if(!szDescData){

			return -4;
		}
        // convert szDescData to MutliByte and save to WideChar

		szaSubDescs.push_back(MutliByte2WideChar(szDescData));
		free(szDescData);
	}else{
		//szaSubDescs.Add(_T(""));
		szaSubDescs.push_back(L"");
	}
    this->ExtractSubFiles(fp);
	

	return 0;

}
int CSVPToolBox::ExtractSubFiles(FILE* fp){
    printf( "Extracting subFiles\n" );
	char szSBuff[8];
	if ( fread(szSBuff , sizeof(char), 4, fp) < 4){

		return -1;
	}

	size_t iFileDataLength = this->Char4ToInt(szSBuff); //确认是否确实有文件下载
	if(!iFileDataLength){
        printf("No real file released\n");
		return 0;
	}

	if ( fread(szSBuff , sizeof(char), 1, fp) < 1){
        printf("Fail to retrive how many Files are there\n");
		return -2;
	}
	int iFileCount = szSBuff[0];
	if( iFileCount <= 0 ){
        printf("No file in File Data\n");
		return -3;
	}

    int iCurrentId = this->szaSubTmpFileList.size();
	this->szaSubTmpFileList.push_back(L"");
	for(int k = 0; k < iFileCount; k++){
		if(this->ExtractEachSubFile(fp, iCurrentId) ){
			//SVP_LogMsg(_T("File Extract Error ") );
			return -4;
		}
	}

	return 0;
}
char* CSVPToolBox::ReadToPTCharByLength(FILE* fp, size_t length){
	char * szBuffData =(char *)calloc( length + 1, sizeof(char));

	if(!szBuffData){

		return 0;
	}

	if ( fread(szBuffData , sizeof(char), length, fp) < length){

		return 0;
	}

	return szBuffData;
}
int CSVPToolBox::ExtractEachSubFile(FILE* fp, int iSubPosId){
	// get file ext name
    printf( "Extracting eachfile\n" );
	char szSBuff[4096];
	if ( fread(szSBuff , sizeof(char), 4, fp) < 4){
		printf("Fail to retrive Single File Pack Length");
		return -1;
	}

	if ( fread(szSBuff , sizeof(char), 4, fp) < 4){

		return -1;
	}

	size_t iExtLength = this->Char4ToInt(szSBuff);
	char* szExtName = this->ReadToPTCharByLength(fp, iExtLength);
	if(!szExtName){

		return -2;
	}
	
	
	//get filedata length
	if ( fread(szSBuff , sizeof(char), 4, fp) < 4){

		return -1;
	}

	size_t iFileLength = this->Char4ToInt(szSBuff);
	
	// gen tmp name and tmp file point
	char otmpfilename[L_tmpnam];
    tmpnam(otmpfilename);

	FILE* fpt=fopen(otmpfilename,"wb+");
    //unlink(otmpfilename);can not unlink here,the file will be opened again.
    
	if(fpt==NULL){

		return -4;
	}

    // copy data to tmp file
	size_t leftoread = iFileLength;
	do{
		size_t needtoread = SVP_MIN( 4096, leftoread );
		size_t accturead = fread(szSBuff , sizeof(char), needtoread, fp);
		if(accturead == 0){
			//wtf
			break;
		}
		leftoread -= accturead;
    fwrite(szSBuff,  sizeof(char), accturead , fpt);//把szSBuff里的东西写到otmpfilename里

		
	}while(leftoread > 0);
	fclose( fpt );

	char otmpfilenameraw[L_tmpnam];
    tmpnam(otmpfilenameraw);

    printf("\nDecompressing file\n");
    unpackGZfile( otmpfilename , otmpfilenameraw );
    //unlink(otmpfilenameraw);can not unlink here,the file will be opened again.
    
	// add filename and tmp name to szaTmpFileNames
	//this->szaSubTmpFileList[iSubPosId].Append( this->UTF8ToCString(szExtName, iExtLength)); //why cant use + ???
	this->szaSubTmpFileList[iSubPosId].append(MutliByte2WideChar(szExtName));
	this->szaSubTmpFileList[iSubPosId].append(L"|" );
	this->szaSubTmpFileList[iSubPosId].append(MutliByte2WideChar(otmpfilenameraw));
	this->szaSubTmpFileList[iSubPosId].append( L";");

    unlink(otmpfilename);
	return 0;
}


int CSVPToolBox::Explode(std::wstring szIn,
                         std::wstring szTok,
                         std::vector<std::wstring>* szaOut)
{
  szaOut->clear();

  std::wstring resToken;

  int size=sizeof(wchar_t);
  wchar_t* str = new wchar_t[szIn.length() + 1];
  wcscpy(str, szIn.c_str());
  wchar_t* sep = new wchar_t[szTok.length() + 1];
  wcscpy(sep, szTok.c_str());
  wchar_t* token = NULL;
  wchar_t* next_token = NULL;
  token = wcstok(str, sep, &next_token);

  while (token != NULL)
  {
    szaOut->push_back((std::wstring)token);
    token = wcstok( NULL, sep, &next_token);
  }

  delete[] str;
  delete[] sep;

  return 0;
}



bool CSVPToolBox::ifFileExist_STL(std::wstring szPathname, bool evenSlowDriver)
{
  std::wstring szPathExt = szPathname.substr(0, 6);
  std::transform(szPathExt.begin(), szPathExt.end(),
                 szPathExt.begin(), ::tolower);
 
   if (szPathExt ==  L"rar://")
    {
      /*
        //RARTODO: 检测rar内的文件是否存在 //Done
              CSVPRarLib svpRar;
              if(svpRar.SplitPath_STL(szPathname))
                szPathname = (LPCTSTR)svpRar.m_fnRAR;
              else
                return false;*/

        printf("rar file!\n");
    }
  

  
  struct stat sbuf;

  return (!stat(WideChar2MultiByte(szPathname.c_str()), &sbuf) && S_IFREG & sbuf.st_mode);//Get status information on a file.


}





std::wstring CSVPToolBox::getSubFileByTempid_STL(int iTmpID,char* strPath)
{
  //get base path name
  std::wstring szVidPath=MutliByte2WideChar(strPath);
  std::vector<std::wstring> szVidPathInfo;
  std::wstring szTargetBaseName = L"";
  std::wstring szDefaultSubPath = L"";



  std::wstring StoreDir = L"";
  getVideoFileBasename(szVidPath, &szVidPathInfo); 

  


  StoreDir = szVidPathInfo.at(SVPATH_DIRNAME).c_str();


  std::wstring tmBasenamePath=StoreDir;

  getVideoFileBasename(szVidPath, &szVidPathInfo);
  tmBasenamePath+=(szVidPathInfo.at(SVPATH_FILENAME).c_str());

  std::wstring szBasename = tmBasenamePath;



  //set new file name
  std::vector<std::wstring> szSubfiles;
  //std::wstring szXTmpdata = this->szaSubTmpFileList.GetAt(iTmpID);
  std::wstring szXTmpdata = this->szaSubTmpFileList.at(iTmpID);
  Explode(szXTmpdata, L";", &szSubfiles);
  bool bIsIdxSub = false;
  int ialreadyExist = 0;

  if (szSubfiles.size() < 1) printf("Not enough files in tmp array");

  for(int i = 0; i < szSubfiles.size(); i++)
  {
    std::vector<std::wstring> szSubTmpDetail;
    Explode(szSubfiles[i], L"|", &szSubTmpDetail);
    if (szSubTmpDetail.size() < 2)
    {
      printf("Not enough detail in sub tmp string");
      continue;
    }
    std::wstring szSource = szSubTmpDetail[1];
    std::wstring szLangExt  = L".chn"; //TODO: use correct language perm 
    if(bIsIdxSub)
      szLangExt  = L"";
    else
      szLangExt  = DetectSubFileLanguage_STL(szSource);//check if english sub
    if (szSubTmpDetail[0].at(0) != L'.')
      szSubTmpDetail[0] = L"." + szSubTmpDetail[0];
    std::wstring szTarget = szBasename + szLangExt + szSubTmpDetail[0];
    szTargetBaseName = szBasename + szLangExt ;

    CMD5Checksum cm5source;
    std::wstring szSourceMD5 = cm5source.GetMD5(szSource);
    std::wstring szTargetMD5;
    //check if target exist
    wchar_t szTmp[128];
    wcscpy(szTmp, L"");
    int ilan = 1;
    while(ifFileExist_STL(szTarget))
    {
      //TODO: compare if its the same file
      cm5source.Clean();
      szTargetMD5 = cm5source.GetMD5(szTarget);
      if(szTargetMD5 == szSourceMD5)
      {
        // TODO: if there is a diffrence in delay
        //printf("同样的字幕文件已经存在了\n");
        ialreadyExist++; //TODO: 如果idx+sub里面只有一个文件相同怎么办 ？？~~ 
        break;
      }

      swprintf(szTmp,128, L"%d", ilan);
      szTarget = szBasename + szLangExt + (std::wstring)szTmp + szSubTmpDetail[0];
      szTargetBaseName = szBasename + szLangExt + (std::wstring)szTmp;
      ilan++;
    }

    printf("Copying file\n");
    CopyFile(szSource.c_str(), szTarget.c_str());
    if (((bIsIdxSub && szSubTmpDetail[0].compare(L"idx") == 0)
      || !bIsIdxSub) && szDefaultSubPath.empty())
        szDefaultSubPath = szTarget;

    std::vector<std::wstring> szaDesclines;
    if (szaSubDescs.size() > iTmpID)
    {

		Explode(szaSubDescs.at(iTmpID), L"\x0b\x0b", &szaDesclines);
      if(szaDesclines.size() > 0)
      {
        int iDelay = 0;
        swscanf(szaDesclines.at(0).c_str(), L"delay=%d", &iDelay);
        if (iDelay)
        {
          wchar_t szBuf[128];
          swprintf(szBuf, 128, L"%d", iDelay);
          filePutContent_STL(szTarget + L".delay", (std::wstring)szBuf);

        }
        else
            remove(WideChar2MultiByte((szTarget + L".delay").c_str()));

      }
    }
    else
      printf("Count of szaSubDescs not match with count of subs ");

    unlink(WideChar2MultiByte(szSource.c_str()));
  }
  
  if(ialreadyExist)
    return L"EXIST";
  else
    return szDefaultSubPath;
}


int CSVPToolBox::Char4ToInt(char* szBuf){

	int iData =   ( ((int)szBuf[0] & 0xff) << 24) |  ( ((int)szBuf[1] & 0xff) << 16) | ( ((int)szBuf[2] & 0xff) << 8) |  szBuf[3] & 0xff;;
	
	return iData;
}


int CSVPToolBox::CopyFile(std::wstring sourcePath, std::wstring destPath)
{
    char* strsource=WideChar2MultiByte(sourcePath.c_str());
    char* strdest=WideChar2MultiByte(destPath.c_str());
    char s[512];
    sprintf(s,"cp %s \"%s\"",strsource,strdest);
    system(s);
    return 0;
}
