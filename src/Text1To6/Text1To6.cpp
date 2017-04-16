#include <sdw.h>

int UMain(int argc, UChar* argv[])
{
	if (argc != 2 && argc != 3)
	{
		return 1;
	}
	bool bText1To4 = argc == 3 && UCscmp(argv[2], USTR("0")) != 0;
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
	map<wstring, n32> mMIDIndex;
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
		if (nOuterNo % 2 == 0 && !mMIDIndex.insert(make_pair(sOldText, nOuterNo)).second)
		{
			return 1;
		}
	}
	wstring sMIDSuffix[6] = { L"_PCM1", L"_PCM2", L"_PCM3", L"_PCF1", L"_PCF2", L"_PCF3" };
	if (bText1To4)
	{
		sMIDSuffix[2].clear();
		sMIDSuffix[5].clear();
	}
	FILE* fpLog = fopen("text1to6.txt", "ab");
	if (fpLog == nullptr)
	{
		return 1;
	}
	fseek(fpLog, 0, SEEK_END);
	if (ftell(fpLog) == 0)
	{
		fwrite("\xFF\xFE", 2, 1, fpLog);
	}
	bool bLogFileName = false;
	for (map<n32, map<n32, map<n32, wstring>>>::iterator it = mText.begin(); it != mText.end(); ++it)
	{
		n32 nOuterNo = it->first;
		map<n32, map<n32, wstring>>& mOuterStmt = it->second;
		vector<n32> vIndex;
		if (nOuterNo % 2 == 0)
		{
			wstring sMID = mOuterStmt[0][2];
			if (!EndWith(sMID, L"_PCM1"))
			{
				continue;
			}
			wstring sMIDBase = sMID.substr(0, sMID.size() - wcslen(L"_PCM1"));
			vIndex.push_back(nOuterNo);
			for (n32 i = 1; i < SDW_ARRAY_COUNT(sMIDSuffix); i++)
			{
				if (!sMIDSuffix[i].empty())
				{
					sMID = sMIDBase + sMIDSuffix[i];
					map<wstring, n32>::iterator it2 = mMIDIndex.find(sMID);
					if (it2 != mMIDIndex.end())
					{
						vIndex.push_back(it2->second);
					}
				}
			}
		}
		if (vIndex.size() > 1)
		{
			map<n32, map<n32, wstring>>& mOuterStmtPCM1 = mText[vIndex[0] + 1];
			for (n32 i = 1; i < static_cast<n32>(vIndex.size()); i++)
			{
				wstring sMID = mText[vIndex[i]][0][2];
				bool bNoText = sMID[sMID.size() - 1] == L'3';
				map<n32, map<n32, wstring>>& mOuterStmtOther = mText[vIndex[i] + 1];
				if (mOuterStmtOther.size() != mOuterStmtPCM1.size())
				{
					if (!bLogFileName)
					{
						fu16printf(fpLog, L"%ls\r\n", UToW(argv[1]).c_str());
					}
					fu16printf(fpLog, L"\tNo.%d, %ls No.%d, %ls stmt count not equal\r\n", vIndex[0] + 1, mText[vIndex[0]][0][2].c_str(), vIndex[i] + 1, sMID.c_str());
					continue;
				}
				bool bContinue = false;
				for (map<n32, map<n32, wstring>>::iterator itPCM1 = mOuterStmtPCM1.begin(); itPCM1 != mOuterStmtPCM1.end(); ++itPCM1)
				{
					n32 nNo = itPCM1->first;
					wstring sCharNamePCM1 = itPCM1->second[1];
					map<n32, map<n32, wstring>>::iterator itOther = mOuterStmtOther.find(nNo);
					if (itOther == mOuterStmtOther.end())
					{
						if (!bLogFileName)
						{
							fu16printf(fpLog, L"%ls\r\n", UToW(argv[1]).c_str());
						}
						fu16printf(fpLog, L"\t%ls stmt not find No.%d,%d,\r\n", sMID.c_str(), vIndex[i] + 1, nNo);
						bContinue = true;
						break;
					}
					wstring sCharNameOther = itOther->second[1];
					if (sCharNameOther != sCharNamePCM1)
					{
						if (!bLogFileName)
						{
							fu16printf(fpLog, L"%ls\r\n", UToW(argv[1]).c_str());
						}
						fu16printf(fpLog, L"\t%ls stmt No.%d,%d, char name not equal %ls %ls\r\n", sMID.c_str(), vIndex[i] + 1, nNo, sCharNamePCM1.c_str(), sCharNameOther.c_str());
						bContinue = true;
						break;
					}
				}
				if (bContinue)
				{
					continue;
				}
				for (map<n32, map<n32, wstring>>::iterator itPCM1 = mOuterStmtPCM1.begin(); itPCM1 != mOuterStmtPCM1.end(); ++itPCM1)
				{
					n32 nNo = itPCM1->first;
					wstring sCharNamePCM1 = itPCM1->second[1];
					if (!(sCharNamePCM1 == L"username" && bNoText))
					{
						mOuterStmtOther[nNo][3] = itPCM1->second[3];
					}
				}
			}
		}
	}
	for (map<n32, map<n32, map<n32, wstring>>>::iterator it = mText.begin(); it != mText.end(); ++it)
	{
		n32 nOuterNo = it->first;
		map<n32, map<n32, wstring>>& mOuterStmt = it->second;
		vector<n32> vIndex;
		if (nOuterNo % 2 == 0)
		{
			wstring sMID = mOuterStmt[0][2];
			if (!EndWith(sMID, L"_PCF1"))
			{
				continue;
			}
			wstring sMIDBase = sMID.substr(0, sMID.size() - wcslen(L"_PCF1"));
			sMID = sMIDBase + L"_PCM1";
			map<wstring, n32>::iterator it2 = mMIDIndex.find(sMID);
			if (it2 != mMIDIndex.end())
			{
				continue;
			}
			vIndex.push_back(nOuterNo);
			for (n32 i = 0; i < SDW_ARRAY_COUNT(sMIDSuffix); i++)
			{
				if (i == 3)
				{
					continue;
				}
				if (!sMIDSuffix[i].empty())
				{
					sMID = sMIDBase + sMIDSuffix[i];
					it2 = mMIDIndex.find(sMID);
					if (it2 != mMIDIndex.end())
					{
						vIndex.push_back(it2->second);
					}
				}
			}
		}
		if (vIndex.size() > 1)
		{
			map<n32, map<n32, wstring>>& mOuterStmtPCF1 = mText[vIndex[0] + 1];
			for (n32 i = 1; i < static_cast<n32>(vIndex.size()); i++)
			{
				wstring sMID = mText[vIndex[i]][0][2];
				bool bNoText = sMID[sMID.size() - 1] == L'3';
				map<n32, map<n32, wstring>>& mOuterStmtOther = mText[vIndex[i] + 1];
				if (mOuterStmtOther.size() != mOuterStmtPCF1.size())
				{
					if (!bLogFileName)
					{
						fu16printf(fpLog, L"%ls\r\n", UToW(argv[1]).c_str());
					}
					fu16printf(fpLog, L"\tNo.%d, %ls No.%d, %ls stmt count not equal\r\n", vIndex[0] + 1, mText[vIndex[0]][0][2].c_str(), vIndex[i] + 1, sMID.c_str());
					continue;
				}
				bool bContinue = false;
				for (map<n32, map<n32, wstring>>::iterator itPCF1 = mOuterStmtPCF1.begin(); itPCF1 != mOuterStmtPCF1.end(); ++itPCF1)
				{
					n32 nNo = itPCF1->first;
					wstring sCharNamePCF1 = itPCF1->second[1];
					map<n32, map<n32, wstring>>::iterator itOther = mOuterStmtOther.find(nNo);
					if (itOther == mOuterStmtOther.end())
					{
						if (!bLogFileName)
						{
							fu16printf(fpLog, L"%ls\r\n", UToW(argv[1]).c_str());
						}
						fu16printf(fpLog, L"\t%ls stmt not find No.%d,%d,\r\n", sMID.c_str(), vIndex[i] + 1, nNo);
						bContinue = true;
						break;
					}
					wstring sCharNameOther = itOther->second[1];
					if (sCharNameOther != sCharNamePCF1)
					{
						if (!bLogFileName)
						{
							fu16printf(fpLog, L"%ls\r\n", UToW(argv[1]).c_str());
						}
						fu16printf(fpLog, L"\t%ls stmt No.%d,%d, char name not equal %ls %ls\r\n", sMID.c_str(), vIndex[i] + 1, nNo, sCharNamePCF1.c_str(), sCharNameOther.c_str());
						bContinue = true;
						break;
					}
				}
				if (bContinue)
				{
					continue;
				}
				for (map<n32, map<n32, wstring>>::iterator itPCF1 = mOuterStmtPCF1.begin(); itPCF1 != mOuterStmtPCF1.end(); ++itPCF1)
				{
					n32 nNo = itPCF1->first;
					wstring sCharNamePCF1 = itPCF1->second[1];
					if (!(sCharNamePCF1 == L"username" && bNoText))
					{
						mOuterStmtOther[nNo][3] = itPCF1->second[3];
					}
				}
			}
		}
	}
	fclose(fpLog);
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
