#include <sdw.h>

int UMain(int argc, UChar* argv[])
{
	if (argc != 2)
	{
		return 1;
	}
	FILE* fp = UFopen(argv[1], USTR("rb"));
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
	wstring sKana;
	for (wchar_t i = 0x3041; i < 0x3094; i++)
	{
		sKana.append(1, i);
	}
	for (wchar_t i = 0x30A1; i < 0x30F4; i++)
	{
		sKana.append(1, i);
	}
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
		n32 nNum = SToN32(sNum.substr(wcslen(L"No.")));
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
		if (!sTxtNew.empty())
		{
			sTxtNew += L"\r\n\r\n";
		}
		if (nNum % 2 != 0)
		{
			wstring::size_type uPos = sStmtNew.find_first_of(sKana);
			if (uPos != wstring::npos)
			{
				wstring sWord = sStmtNew.substr(uPos, 1);
				sStmtNew = Replace(sStmtNew, L"\r\n", L"\n");
				UPrintf(USTR("%") PRIUS USTR(" |%") PRIUS USTR("|\n%") PRIUS USTR("\n"), WToU(sNum).c_str(), WToU(sWord).c_str(), WToU(sStmtNew).c_str());
			}
		}
	}
	return 0;
}
