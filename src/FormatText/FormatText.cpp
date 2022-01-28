#include <sdw.h>

int UMain(int argc, UChar* argv[])
{
	if (argc != 2)
	{
		return 1;
	}
	FILE* fp = UFopen(argv[1], USTR("rb"), false);
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
	wstring sTxtNew;
	wstring::size_type uPos0 = 0;
	while ((uPos0 = sTxt.find(L"No.", uPos0)) != wstring::npos)
	{
		wstring::size_type uPos1 = sTxt.find(L"\r\n--------------------------------------\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		wstring sNum = sTxt.substr(uPos0, uPos1 - uPos0);
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
		wstring sTempTxt = sStmtOld;
		sTempTxt = Replace(sTempTxt, L"[--------------------------------------]", L"");
		sTempTxt = Replace(sTempTxt, L"[======================================]", L"");
		if (sTempTxt.find(L"--------------------------------------") != wstring::npos)
		{
			return 1;
		}
		if (sTempTxt.find(L"======================================") != wstring::npos)
		{
			return 1;
		}
		if (sTempTxt.find(L"No.") != wstring::npos)
		{
			return 1;
		}
		sTempTxt = sStmtNew;
		sTempTxt = Replace(sTempTxt, L"[--------------------------------------]", L"");
		sTempTxt = Replace(sTempTxt, L"[======================================]", L"");
		if (sTempTxt.find(L"--------------------------------------") != wstring::npos)
		{
			return 1;
		}
		if (sTempTxt.find(L"======================================") != wstring::npos)
		{
			return 1;
		}
		if (sTempTxt.find(L"No.") != wstring::npos)
		{
			return 1;
		}
		if (!sTxtNew.empty())
		{
			sTxtNew += L"\r\n\r\n";
		}
		sTxtNew += sNum;
		sTxtNew += L"\r\n--------------------------------------\r\n";
		sTxtNew += sStmtOld;
		sTxtNew += L"\r\n======================================\r\n";
		sTxtNew += sStmtNew;
		sTxtNew += L"\r\n--------------------------------------\r\n";
	}
	U16String sTxtNewU16 = WToU16(sTxtNew);
	fp = UFopen(argv[1], USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite("\xFF\xFE", 2, 1, fp);
	fwrite(sTxtNewU16.c_str(), 2, sTxtNewU16.size(), fp);
	fclose(fp);
	return 0;
}
