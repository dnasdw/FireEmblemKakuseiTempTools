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
	// £¤£¤£¤£¤£¤£¤£¤£¤ÖØ¸´ÎÄ±¾£¤£¤£¤£¤£¤£¤£¤
	wstring sReplacement = L"\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5\u91CD\u590D\u6587\u672C\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5\uFFE5";
	wstring::size_type uPos0 = 0;
	while ((uPos0 = sTxt.find(L"No.", uPos0)) != wstring::npos)
	{
		uPos0 += wcslen(L"No.");
		wstring::size_type uPos1 = sTxt.find(L"\r\n--------------------------------------\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		UString sNum = WToU(sTxt.substr(uPos0, uPos1 - uPos0));
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
		uPos0 = uPos1 += wcslen(L"\r\n--------------------------------------");
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
		if (sStmtNew != sReplacement && sStmtNew != sStmtOld)
		{
			map<wstring, u32> mControlOld;
			map<wstring, u32> mControlNew;
			wstring::size_type uPosOld0 = 0;
			wstring::size_type uPosNew0 = 0;
			do
			{
				uPosOld0 = sStmtOld.find_first_of(L"<{", uPosOld0);
				uPosNew0 = sStmtNew.find_first_of(L"<{", uPosNew0);
				if (uPosOld0 == wstring::npos && uPosNew0 == wstring::npos)
				{
					break;
				}
				if (uPosOld0 == wstring::npos || uPosNew0 == wstring::npos)
				{
					UPrintf(USTR("No.%") PRIUS USTR(" error\n"), sNum.c_str());
					break;
				}
				wstring::size_type uPosOld1 = uPosOld0;
				if (sStmtOld[uPosOld0] == L'<')
				{
					uPosOld1 = sStmtOld.find(L'>', uPosOld0);
					if (uPosOld1 == wstring::npos)
					{
						UPrintf(USTR("No.%") PRIUS USTR(" error, old not find >\n"), sNum.c_str());
						return 1;
					}
					wstring sControlOld = sStmtOld.substr(uPosOld0, uPosOld1 + 1 - uPosOld0);
					mControlOld[sControlOld]++;
				}
				else if (sStmtOld[uPosOld0] == L'{')
				{
					uPosOld1 = sStmtOld.find(L'}', uPosOld0);
					if (uPosOld1 == wstring::npos)
					{
						UPrintf(USTR("No.%") PRIUS USTR(" error, old not find }\n"), sNum.c_str());
						return 1;
					}
					wstring sControlOld = sStmtOld.substr(uPosOld0, uPosOld1 + 1 - uPosOld0);
					mControlOld[sControlOld]++;
				}
				wstring::size_type uPosNew1 = uPosNew0;
				if (sStmtNew[uPosNew0] == L'<')
				{
					uPosNew1 = sStmtNew.find(L'>', uPosNew0);
					if (uPosNew1 == wstring::npos)
					{
						UPrintf(USTR("No.%") PRIUS USTR(" error, new not find >\n"), sNum.c_str());
						return 1;
					}
					wstring sControlNew = sStmtNew.substr(uPosNew0, uPosNew1 + 1 - uPosNew0);
					mControlNew[sControlNew]++;
				}
				else if (sStmtNew[uPosNew0] == L'{')
				{
					uPosNew1 = sStmtNew.find(L'}', uPosNew0);
					if (uPosNew1 == wstring::npos)
					{
						UPrintf(USTR("No.%") PRIUS USTR(" error, new not find }\n"), sNum.c_str());
						return 1;
					}
					wstring sControlNew = sStmtNew.substr(uPosNew0, uPosNew1 + 1 - uPosNew0);
					mControlNew[sControlNew]++;
				}
				uPosOld0 = uPosOld1 + 1;
				uPosNew0 = uPosNew1 + 1;
			} while (true);
			if (!equal(mControlOld.begin(), mControlOld.end(), mControlNew.begin()))
			{
				UPrintf(USTR("No.%") PRIUS USTR(" error, control not equal\n"), sNum.c_str());
				UPrintf(USTR("old:\n"));
				for (map<wstring, u32>::iterator it = mControlOld.begin(); it != mControlOld.end(); ++it)
				{
					UPrintf(USTR("%") PRIUS USTR(" %d\n"), WToU(it->first).c_str(), it->second);
				}
				UPrintf(USTR("new:\n"));
				for (map<wstring, u32>::iterator it = mControlNew.begin(); it != mControlNew.end(); ++it)
				{
					UPrintf(USTR("%") PRIUS USTR(" %d\n"), WToU(it->first).c_str(), it->second);
				}
			}
		}
	}
	return 0;
}
