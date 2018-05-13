#include <sdw.h>
#include <png.h>

struct SPngRecord
{
	vector<UString> OldFileName;
	UString NewFileName;
	u32 CRC32;
	u32 Width;
	u32 Height;
};

int getPngFileName(const UString& a_sDirName, vector<UString>& a_vPngFileName)
{
	queue<UString> qDir;
	qDir.push(a_sDirName);
	while (!qDir.empty())
	{
		UString& sParent = qDir.front();
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
		WIN32_FIND_DATAW ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		wstring sPattern = sParent + L"/*";
		hFind = FindFirstFileW(sPattern.c_str(), &ffd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && EndWith<wstring>(ffd.cFileName, L".png"))
				{
					wstring sFileName = sParent + L"/" + ffd.cFileName;
					a_vPngFileName.push_back(sFileName);
				}
				else if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
				{
					wstring sDir = sParent + L"/" + ffd.cFileName;
					qDir.push(sDir);
				}
			} while (FindNextFileW(hFind, &ffd) != 0);
		}
#else
		DIR* pDir = opendir(sParent.c_str());
		if (pDir != nullptr)
		{
			dirent* pDirent = nullptr;
			while ((pDirent = readdir(pDir)) != nullptr)
			{
				if (pDirent->d_type == DT_REG && EndWith<string>(pDirent->d_name, ".png"))
				{
					string sFileName = sParent + "/" + pDirent->d_name;
					a_vPngFileName.push_back(sFileName);
				}
				else if (pDirent->d_type == DT_DIR && strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0)
				{
					string sDir = sParent + "/" + pDirent->d_name;
					qDir.push(sDir);
				}
			}
			closedir(pDir);
		}
#endif
		qDir.pop();
	}
	sort(a_vPngFileName.begin(), a_vPngFileName.end());
	return 0;
}

int resavePng(const UString& a_sDirName)
{
	vector<UString> vPngFileName;
	if (getPngFileName(a_sDirName, vPngFileName) != 0)
	{
		return 1;
	}
	for (vector<UString>::iterator it = vPngFileName.begin(); it != vPngFileName.end(); ++it)
	{
		UString& sPngFileName = *it;
		FILE* fp = UFopen(sPngFileName.c_str(), USTR("rb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fp);
			return 1;
		}
		png_infop pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, nullptr, nullptr);
			fclose(fp);
			return 1;
		}
		png_infop pEndInfo = png_create_info_struct(pPng);
		if (pEndInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, &pInfo, nullptr);
			fclose(fp);
			return 1;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		png_init_io(pPng, fp);
		png_read_info(pPng, pInfo);
		u32 uPngWidth = png_get_image_width(pPng, pInfo);
		u32 uPngHeight = png_get_image_height(pPng, pInfo);
		n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		n32 nColorType = png_get_color_type(pPng, pInfo);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		u8* pData = new u8[uPngWidth * uPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[uPngHeight];
		for (u32 i = 0; i < uPngHeight; i++)
		{
			pRowPointers[i] = pData + i * uPngWidth * 4;
		}
		png_read_image(pPng, pRowPointers);
		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
		delete[] pRowPointers;
		fclose(fp);
		fp = UFopen(sPngFileName.c_str(), USTR("wb"), false);
		if (fp == nullptr)
		{
			delete[] pData;
			return 1;
		}
		pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fp);
			delete[] pData;
			return 1;
		}
		pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_write_struct(&pPng, nullptr);
			fclose(fp);
			delete[] pData;
			return 1;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_write_struct(&pPng, &pInfo);
			fclose(fp);
			delete[] pData;
			return 1;
		}
		png_init_io(pPng, fp);
		png_set_IHDR(pPng, pInfo, uPngWidth, uPngHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		pRowPointers = new png_bytep[uPngHeight];
		for (u32 i = 0; i < uPngHeight; i++)
		{
			pRowPointers[i] = pData + i * uPngWidth * 4;
		}
		png_set_rows(pPng, pInfo, pRowPointers);
		png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
		png_destroy_write_struct(&pPng, &pInfo);
		delete[] pRowPointers;
		fclose(fp);
		delete[] pData;
	}
	return 0;
}

int exportPng(const UString& a_sOldDirName, const UString& a_sNewDirName, const UString& a_sRecordFileName)
{
	vector<UString> vPngFileName;
	if (getPngFileName(a_sOldDirName, vPngFileName) != 0)
	{
		return 1;
	}
	UMkdir(a_sNewDirName.c_str());
	vector<SPngRecord> vPngRecord;
	for (vector<UString>::iterator it = vPngFileName.begin(); it != vPngFileName.end(); ++it)
	{
		UString sPngFileName = *it;
		FILE* fp = UFopen(sPngFileName.c_str(), USTR("rb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fp);
			return 1;
		}
		png_infop pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, nullptr, nullptr);
			fclose(fp);
			return 1;
		}
		png_infop pEndInfo = png_create_info_struct(pPng);
		if (pEndInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, &pInfo, nullptr);
			fclose(fp);
			return 1;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		png_init_io(pPng, fp);
		png_read_info(pPng, pInfo);
		u32 uPngWidth = png_get_image_width(pPng, pInfo);
		u32 uPngHeight = png_get_image_height(pPng, pInfo);
		n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		n32 nColorType = png_get_color_type(pPng, pInfo);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		u8* pData = new u8[uPngWidth * uPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[uPngHeight];
		for (u32 i = 0; i < uPngHeight; i++)
		{
			pRowPointers[i] = pData + i * uPngWidth * 4;
		}
		png_read_image(pPng, pRowPointers);
		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
		delete[] pRowPointers;
		fclose(fp);
		u32 uCRC32 = UpdateCRC32(pData, uPngWidth * uPngHeight * 4);
		UString sOldFileName = sPngFileName.substr(a_sOldDirName.size() + 1);
		UString sNewFileName = Replace(sOldFileName, USTR("/"), USTR("_"));
		bool bSame = false;
		for (vector<SPngRecord>::iterator itRecord = vPngRecord.begin(); itRecord != vPngRecord.end(); ++itRecord)
		{
			SPngRecord& pngRecord = *itRecord;
			if (pngRecord.CRC32 == uCRC32 && pngRecord.Width == uPngWidth && pngRecord.Height == uPngHeight)
			{
				pngRecord.OldFileName.push_back(sOldFileName);
				bSame = true;
				break;
			}
		}
		if (!bSame)
		{
			vPngRecord.resize(vPngRecord.size() + 1);
			SPngRecord& pngRecord = vPngRecord.back();
			pngRecord.OldFileName.push_back(sOldFileName);
			pngRecord.NewFileName = sNewFileName;
			pngRecord.CRC32 = uCRC32;
			pngRecord.Width = uPngWidth;
			pngRecord.Height = uPngHeight;
			sPngFileName = a_sNewDirName + USTR("/") + sNewFileName;
			fp = UFopen(sPngFileName.c_str(), USTR("wb"), false);
			if (fp == nullptr)
			{
				delete[] pData;
				return 1;
			}
			pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			if (pPng == nullptr)
			{
				fclose(fp);
				delete[] pData;
				return 1;
			}
			pInfo = png_create_info_struct(pPng);
			if (pInfo == nullptr)
			{
				png_destroy_write_struct(&pPng, nullptr);
				fclose(fp);
				delete[] pData;
				return 1;
			}
			if (setjmp(png_jmpbuf(pPng)) != 0)
			{
				png_destroy_write_struct(&pPng, &pInfo);
				fclose(fp);
				delete[] pData;
				return 1;
			}
			png_init_io(pPng, fp);
			png_set_IHDR(pPng, pInfo, uPngWidth, uPngHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			pRowPointers = new png_bytep[uPngHeight];
			for (u32 i = 0; i < uPngHeight; i++)
			{
				pRowPointers[i] = pData + i * uPngWidth * 4;
			}
			png_set_rows(pPng, pInfo, pRowPointers);
			png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&pPng, &pInfo);
			delete[] pRowPointers;
			fclose(fp);
		}
		delete[] pData;
	}
	FILE* fp = UFopen(a_sRecordFileName.c_str(), USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite("\xFF\xFE", 2, 1, fp);
	n32 nIndex = 0;
	for (vector<SPngRecord>::iterator itRecord = vPngRecord.begin(); itRecord != vPngRecord.end(); ++itRecord)
	{
		SPngRecord& pngRecord = *itRecord;
		if (ftell(fp) != 2)
		{
			fu16printf(fp, L"\r\n\r\n");
		}
		fu16printf(fp, L"No.%d\r\n", nIndex++);
		fu16printf(fp, L"--------------------------------------\r\n");
		fu16printf(fp, L"%ls\r\n", pngRecord.NewFileName.c_str());
		fu16printf(fp, L"======================================\r\n");
		for (vector<UString>::iterator it = pngRecord.OldFileName.begin(); it != pngRecord.OldFileName.end(); ++it)
		{
			fu16printf(fp, L"%ls\r\n", it->c_str());
		}
		fu16printf(fp, L"--------------------------------------\r\n");
	}
	fclose(fp);
	return 0;
}

int importPng(const UString& a_sOldDirName, const UString& a_sNewDirName, const UString& a_sRecordFileName)
{
	FILE* fp = UFopen(a_sRecordFileName.c_str(), USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uTxtSize = ftell(fp);
	if (uTxtSize % 2 != 0)
	{
		fclose(fp);
		return 1;
	}
	uTxtSize /= 2;
	fseek(fp, 0, SEEK_SET);
	Char16_t* pTemp = new Char16_t[uTxtSize + 1];
	fread(pTemp, 2, uTxtSize, fp);
	fclose(fp);
	if (pTemp[0] != 0xFEFF)
	{
		delete[] pTemp;
		return 1;
	}
	pTemp[uTxtSize] = 0;
	wstring sTxt = U16ToW(pTemp + 1);
	delete[] pTemp;
	vector<SPngRecord> vPngRecord;
	wstring::size_type uPos0 = 0;
	while ((uPos0 = sTxt.find(L"No.", uPos0)) != wstring::npos)
	{
		wstring::size_type uPos1 = sTxt.find(L"\r\n--------------------------------------\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------\r\n");
		uPos1 = sTxt.find(L"\r\n======================================\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		wstring sStmtOld = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n======================================\r\n");
		uPos1 = sTxt.find(L"\r\n--------------------------------------", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		wstring sStmtNew = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------");
		SPngRecord pngRecord;
		pngRecord.OldFileName = Split(WToU(sStmtNew), USTR("\r\n"));
		pngRecord.NewFileName = WToU(sStmtOld);
		vPngRecord.push_back(pngRecord);
	}
	for (vector<SPngRecord>::iterator itRecord = vPngRecord.begin(); itRecord != vPngRecord.end(); ++itRecord)
	{
		SPngRecord& pngRecord = *itRecord;
		UString sPngFileName = a_sNewDirName + USTR("/") + pngRecord.NewFileName;
		fp = UFopen(sPngFileName.c_str(), USTR("rb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fp);
			return 1;
		}
		png_infop pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, nullptr, nullptr);
			fclose(fp);
			return 1;
		}
		png_infop pEndInfo = png_create_info_struct(pPng);
		if (pEndInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, &pInfo, nullptr);
			fclose(fp);
			return 1;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		png_init_io(pPng, fp);
		png_read_info(pPng, pInfo);
		u32 uPngWidth = png_get_image_width(pPng, pInfo);
		u32 uPngHeight = png_get_image_height(pPng, pInfo);
		n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		n32 nColorType = png_get_color_type(pPng, pInfo);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		u8* pData = new u8[uPngWidth * uPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[uPngHeight];
		for (u32 i = 0; i < uPngHeight; i++)
		{
			pRowPointers[i] = pData + i * uPngWidth * 4;
		}
		png_read_image(pPng, pRowPointers);
		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
		delete[] pRowPointers;
		fclose(fp);
		for (vector<UString>::iterator it = pngRecord.OldFileName.begin(); it != pngRecord.OldFileName.end(); ++it)
		{
			vector<UString> vDirPath = SplitOf(*it, USTR("/\\"));
			UString sDirName = a_sOldDirName;
			for (n32 i = 0; i < static_cast<n32>(vDirPath.size()) - 1; i++)
			{
				sDirName += USTR("/") + vDirPath[i];
				UMkdir(sDirName.c_str());
			}
			sPngFileName = a_sOldDirName + USTR("/") + *it;
			fp = UFopen(sPngFileName.c_str(), USTR("wb"), false);
			if (fp == nullptr)
			{
				delete[] pData;
				return 1;
			}
			pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			if (pPng == nullptr)
			{
				fclose(fp);
				delete[] pData;
				return 1;
			}
			pInfo = png_create_info_struct(pPng);
			if (pInfo == nullptr)
			{
				png_destroy_write_struct(&pPng, nullptr);
				fclose(fp);
				delete[] pData;
				return 1;
			}
			if (setjmp(png_jmpbuf(pPng)) != 0)
			{
				png_destroy_write_struct(&pPng, &pInfo);
				fclose(fp);
				delete[] pData;
				return 1;
			}
			png_init_io(pPng, fp);
			png_set_IHDR(pPng, pInfo, uPngWidth, uPngHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			pRowPointers = new png_bytep[uPngHeight];
			for (u32 i = 0; i < uPngHeight; i++)
			{
				pRowPointers[i] = pData + i * uPngWidth * 4;
			}
			png_set_rows(pPng, pInfo, pRowPointers);
			png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&pPng, &pInfo);
			delete[] pRowPointers;
			fclose(fp);
		}
		delete[] pData;
	}
	return 0;
}

int export2Png(const UString& a_sOldDirName, const UString& a_sNewDirName, const UString& a_sRecordFileName)
{
	FILE* fp = UFopen(a_sRecordFileName.c_str(), USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uTxtSize = ftell(fp);
	if (uTxtSize % 2 != 0)
	{
		fclose(fp);
		return 1;
	}
	uTxtSize /= 2;
	fseek(fp, 0, SEEK_SET);
	Char16_t* pTemp = new Char16_t[uTxtSize + 1];
	fread(pTemp, 2, uTxtSize, fp);
	fclose(fp);
	if (pTemp[0] != 0xFEFF)
	{
		delete[] pTemp;
		return 1;
	}
	pTemp[uTxtSize] = 0;
	wstring sTxt = U16ToW(pTemp + 1);
	delete[] pTemp;
	vector<SPngRecord> vPngRecord;
	wstring::size_type uPos0 = 0;
	while ((uPos0 = sTxt.find(L"No.", uPos0)) != wstring::npos)
	{
		wstring::size_type uPos1 = sTxt.find(L"\r\n--------------------------------------\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------\r\n");
		uPos1 = sTxt.find(L"\r\n======================================\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		wstring sStmtOld = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n======================================\r\n");
		uPos1 = sTxt.find(L"\r\n--------------------------------------", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		wstring sStmtNew = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------");
		SPngRecord pngRecord;
		pngRecord.OldFileName = Split(WToU(sStmtNew), USTR("\r\n"));
		pngRecord.NewFileName = WToU(sStmtOld);
		vPngRecord.push_back(pngRecord);
	}
	UMkdir(a_sNewDirName.c_str());
	for (vector<SPngRecord>::iterator itRecord = vPngRecord.begin(); itRecord != vPngRecord.end(); ++itRecord)
	{
		SPngRecord& pngRecord = *itRecord;
		UString sPngFileName = a_sOldDirName + USTR("/") + pngRecord.OldFileName[0];
		fp = UFopen(sPngFileName.c_str(), USTR("rb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fp);
			return 1;
		}
		png_infop pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, nullptr, nullptr);
			fclose(fp);
			return 1;
		}
		png_infop pEndInfo = png_create_info_struct(pPng);
		if (pEndInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, &pInfo, nullptr);
			fclose(fp);
			return 1;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		png_init_io(pPng, fp);
		png_read_info(pPng, pInfo);
		u32 uPngWidth = png_get_image_width(pPng, pInfo);
		u32 uPngHeight = png_get_image_height(pPng, pInfo);
		n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		n32 nColorType = png_get_color_type(pPng, pInfo);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fp);
			return 1;
		}
		u8* pData = new u8[uPngWidth * uPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[uPngHeight];
		for (u32 i = 0; i < uPngHeight; i++)
		{
			pRowPointers[i] = pData + i * uPngWidth * 4;
		}
		png_read_image(pPng, pRowPointers);
		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
		delete[] pRowPointers;
		fclose(fp);
		sPngFileName = a_sNewDirName + USTR("/") + pngRecord.NewFileName;
		fp = UFopen(sPngFileName.c_str(), USTR("wb"), false);
		if (fp == nullptr)
		{
			delete[] pData;
			return 1;
		}
		pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fp);
			delete[] pData;
			return 1;
		}
		pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_write_struct(&pPng, nullptr);
			fclose(fp);
			delete[] pData;
			return 1;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_write_struct(&pPng, &pInfo);
			fclose(fp);
			delete[] pData;
			return 1;
		}
		png_init_io(pPng, fp);
		png_set_IHDR(pPng, pInfo, uPngWidth, uPngHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		pRowPointers = new png_bytep[uPngHeight];
		for (u32 i = 0; i < uPngHeight; i++)
		{
			pRowPointers[i] = pData + i * uPngWidth * 4;
		}
		png_set_rows(pPng, pInfo, pRowPointers);
		png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
		png_destroy_write_struct(&pPng, &pInfo);
		delete[] pRowPointers;
		fclose(fp);
		delete[] pData;
	}
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	if (argc < 3)
	{
		return 1;
	}
	if (UCslen(argv[1]) == 1)
	{
		switch (*argv[1])
		{
		case USTR('R'):
		case USTR('r'):
			return resavePng(argv[2]);
		case USTR('E'):
		case USTR('e'):
			if (argc != 5)
			{
				return 1;
			}
			return exportPng(argv[2], argv[3], argv[4]);
		case USTR('I'):
		case USTR('i'):
			if (argc != 5)
			{
				return 1;
			}
			return importPng(argv[2], argv[3], argv[4]);
		case USTR('M'):
		case USTR('m'):
			if (argc != 5)
			{
				return 1;
			}
			return export2Png(argv[2], argv[3], argv[4]);
		default:
			break;
		}
	}
	return 1;
}
