#include <sdw.h>

struct SText
{
	wstring Number;
	n32 FirstNumber;
	n32 SecondNumber;
	wstring Label;
	wstring TextOld;
	wstring TextNew;
};

bool NumberCompare(const SText& lhs, const SText& rhs)
{
	if (lhs.FirstNumber < rhs.FirstNumber)
	{
		return true;
	}
	else if (lhs.FirstNumber > rhs.FirstNumber)
	{
		return false;
	}
	return lhs.SecondNumber < rhs.SecondNumber;
}

bool LabelCompare(const SText& lhs, const SText& rhs)
{
	if (lhs.Label < rhs.Label)
	{
		return true;
	}
	else if (lhs.Label > rhs.Label)
	{
		return false;
	}
	return NumberCompare(lhs, rhs);
}

bool ReverseLabelCompare(const SText& lhs, const SText& rhs)
{
	wstring sLabelL = lhs.Label;
	vector<wstring> vLabelL = Split(sLabelL, L"_");
	sLabelL.clear();
	for (vector<wstring>::reverse_iterator rit = vLabelL.rbegin(); rit != vLabelL.rend(); ++rit)
	{
		if (rit != vLabelL.rbegin())
		{
			sLabelL += L"_";
		}
		sLabelL += *rit;
	}
	wstring sLabelR = rhs.Label;
	vector<wstring> vLabelR = Split(sLabelR, L"_");
	sLabelR.clear();
	for (vector<wstring>::reverse_iterator rit = vLabelR.rbegin(); rit != vLabelR.rend(); ++rit)
	{
		if (rit != vLabelR.rbegin())
		{
			sLabelR += L"_";
		}
		sLabelR += *rit;
	}
	if (sLabelL < sLabelR)
	{
		return true;
	}
	else if (sLabelL > sLabelR)
	{
		return false;
	}
	return NumberCompare(lhs, rhs);
}

int UMain(int argc, UChar* argv[])
{
	if (argc != 3)
	{
		return 1;
	}
	n32 nSortType = SToN32(argv[2]);
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
	vector<SText> vText;
	wstring::size_type uPos0 = 0;
	while ((uPos0 = sTxt.find(L"No.", uPos0)) != wstring::npos)
	{
		SText text;
		wstring::size_type uPos1 = sTxt.find(L"\r\n--------------------------------------\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		text.Number = sTxt.substr(uPos0, uPos1 - uPos0);
		text.FirstNumber = SToN32(text.Number.c_str() + wcslen(L"No."));
		wstring sFirstNumber = Format(L"No.%d", text.FirstNumber);
		wstring::size_type uSubPos0 = sFirstNumber.size();
		if (uSubPos0 < text.Number.size() && text.Number[uSubPos0] == L',')
		{
			text.SecondNumber = SToN32(text.Number.c_str() + uSubPos0 + 1);
		}
		else
		{
			text.SecondNumber = 0;
		}
		uSubPos0 = text.Number.find(L' ');
		if (uSubPos0 != wstring::npos)
		{
			text.Label = text.Number.substr(uSubPos0 + 1);
		}
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------\r\n");
		uPos1 = sTxt.find(L"\r\n======================================\r\n", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		text.TextOld = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n======================================\r\n");
		uPos1 = sTxt.find(L"\r\n--------------------------------------", uPos0);
		if (uPos1 == wstring::npos)
		{
			return 1;
		}
		text.TextNew = sTxt.substr(uPos0, uPos1 - uPos0);
		uPos0 = uPos1 + wcslen(L"\r\n--------------------------------------");
		vText.push_back(text);
	}
	switch (nSortType)
	{
	case 0:
		stable_sort(vText.begin(), vText.end(), NumberCompare);
		break;
	case 1:
		stable_sort(vText.begin(), vText.end(), LabelCompare);
		break;
	case 2:
		stable_sort(vText.begin(), vText.end(), ReverseLabelCompare);
		break;
	}
	wstring sTxtNew;
	for (vector<SText>::iterator it = vText.begin(); it != vText.end(); ++it)
	{
		SText& text = *it;
		if (!sTxtNew.empty())
		{
			sTxtNew += L"\r\n\r\n";
		}
		sTxtNew += text.Number;
		sTxtNew += L"\r\n--------------------------------------\r\n";
		sTxtNew += text.TextOld;
		sTxtNew += L"\r\n======================================\r\n";
		sTxtNew += text.TextNew;
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
