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
	fseek(fp, 0, SEEK_SET);
	char* pTemp = new char[uTxtSize + 1];
	fread(pTemp, 1, uTxtSize, fp);
	fclose(fp);
	pTemp[uTxtSize] = 0;
	wstring sTxt = AToW(pTemp + 1);
	delete[] pTemp;
	vector<wstring> vTxt = SplitOf(sTxt, L"\r\n");
	for (vector<wstring>::iterator it = vTxt.begin(); it != vTxt.end(); ++it)
	{
		if (StartWith(*it, L"No."))
		{
			return 1;
		}
	}
	return 0;
}
