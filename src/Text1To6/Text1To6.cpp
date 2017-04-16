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
	map<n32, map<n32, map<n32, wstring>>> mText;
	wstring::size_type uPos0 = 0;
	while ((uPos0 = sTxt.find(L"No.", uPos0)) != wstring::npos)
	{
		uPos0 += wcslen(L"No.");
		n32 nOuterNo = SToN32(sTxt.c_str() + uPos0);
		n32 nNo = 0;
		n32 nInnerCount = 0;
		wstring sCharName;
		wstring sOldText;
		wstring sNewText;
		if (nOuterNo % 2 == 0)
		{
			wstring sOuterNum = Format(L"%d\r\n", nOuterNo);
			if (!StartWith(sTxt, sOuterNum, uPos0))
			{
				return 1;
			}
		}
		else
		{
			wstring sOuterNum = Format(L"%d,", nOuterNo);
			if (!StartWith(sTxt, sOuterNum, uPos0))
			{
				return 1;
			}
			uPos0 += sOuterNum.size();
			nNo = SToN32(sTxt.c_str() + uPos0);
			wstring sNum = Format(L"%d,", nNo);
			if (!StartWith(sTxt, sNum, uPos0))
			{
				return 1;
			}
			uPos0 += sNum.size();
			nInnerCount = SToN32(sTxt.c_str() + uPos0);
			uPos0 = sTxt.find_first_of(L" \r\n", uPos0);
			if (uPos0 == wstring::npos)
			{
				return 1;
			}
			if (sTxt[uPos0] == L' ')
			{
				wstring::size_type uPos1 = sTxt.find_first_of(L"\r\n", uPos0 + 1);
				if (uPos1 == wstring::npos)
				{
					return 1;
				}
				sCharName = sTxt.substr(uPos0 + 1, uPos1 - (uPos0 + 1));
				uPos0 = uPos1;
			}
		}
		uPos0 = sTxt.find(L"--------------------------------------\r\n", uPos0);
		if (uPos0 == wstring::npos)
		{
			return 1;
		}
		uPos0 += wcslen(L"--------------------------------------\r\n");
		wstring::size_type uPos1 = sTxt.find(L"\r\n======================================\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		sOldText = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n======================================\r\n");
		uPos1 = sTxt.find(L"\r\n--------------------------------------", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		sNewText = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------");
		mText[nOuterNo][nNo][0] = Format(L"%d", nInnerCount);
		mText[nOuterNo][nNo][1] = sCharName;
		mText[nOuterNo][nNo][2] = sOldText;
		mText[nOuterNo][nNo][3] = sNewText;
	}
	// TODO: replace
	fp = UFopen(argv[1], USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite("\xFF\xFE", 2, 1, fp);
	for (map<n32, map<n32, map<n32, wstring>>>::iterator it = mText.begin(); it != mText.end(); ++it)
	{
		n32 nOuterNo = it->first;
		map<n32, map<n32, wstring>>& mOuterStmt = it->second;
		if (nOuterNo % 2 == 0)
		{
			map<n32, wstring>& mStmt = mOuterStmt[0];
			if (ftell(fp) != 2)
			{
				fu16printf(fp, L"\r\n\r\n");
			}
			fu16printf(fp, L"No.%d\r\n", nOuterNo);
			fu16printf(fp, L"--------------------------------------\r\n");
			fu16printf(fp, L"%ls\r\n", mStmt[2].c_str());
			fu16printf(fp, L"======================================\r\n");
			fu16printf(fp, L"%ls\r\n", mStmt[3].c_str());
			fu16printf(fp, L"--------------------------------------\r\n");
		}
		else
		{
			for (map<n32, map<n32, wstring>>::iterator it2 = mOuterStmt.begin(); it2 != mOuterStmt.end(); ++it2)
			{
				n32 nNo = it2->first;
				map<n32, wstring>& mStmt = it2->second;
				wstring& sCharName = mStmt[1];
				if (ftell(fp) != 2)
				{
					fu16printf(fp, L"\r\n\r\n");
				}
				fu16printf(fp, L"No.%d,%d,%ls%ls%ls\r\n", nOuterNo, nNo, mStmt[0].c_str(), sCharName.empty() ? L"" : L" ", sCharName.c_str());
				fu16printf(fp, L"--------------------------------------\r\n");
				fu16printf(fp, L"%ls\r\n", mStmt[2].c_str());
				fu16printf(fp, L"======================================\r\n");
				fu16printf(fp, L"%ls\r\n", mStmt[3].c_str());
				fu16printf(fp, L"--------------------------------------\r\n");
			}
		}
	}
	fclose(fp);
	return 0;
}
