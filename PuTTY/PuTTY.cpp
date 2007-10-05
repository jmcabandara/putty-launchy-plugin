// BlankPlugin.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "LaunchyPlugin.h"
#include "PluginOptionsForm.h"
#include "PuTTY.h"
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <vcclr.h>

using namespace System::Windows::Forms;

wstring PathToPutty;

void unmungestr(const wstring &in_s, wstring &out_s)
{
	wstring::const_iterator in;

	out_s.clear();
	back_insert_iterator<wstring> out(out_s);

	for (in = in_s.begin(); in != in_s.end(); ++in) {
		if (*in == '%' && in[1] && in[2]) {
			int i, j;

			i = in[1] - '0';
			i -= (i > 9 ? 7 : 0);
			j = in[2] - '0';
			j -= (j > 9 ? 7 : 0);

			*out++ = (i << 4) + j;

			in += 2;
		}else{
			*out++ = *in;
		}
	}
}

void EscapeQuotes(const wstring &in_s, wstring &out_s)
{
	wstring::const_iterator in;

	out_s.clear();
	back_insert_iterator<wstring> out(out_s);

	for (in = in_s.begin(); in != in_s.end(); ++in) {
		if (*in == '"') {
			*out++ = '\\';
			*out++ = '"';
		}else{
			*out++ = *in;
		}
	}
}

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

bool PluginOwnsSearch (TCHAR* txt) 
{
	return false;
}



SearchResult* PluginGetIdentifiers (int* iNumResults)
{
	vector<SearchResult> results;
	results.push_back(makeResult(L"PuTTY", L"", L"", NULL));
	results.push_back(makeResult(L"ssh", L"", L"", NULL));

	*iNumResults = (int) results.size();
	return ResultVectorToArray(results);
}

TCHAR* PluginGetRegexs(int* iNumResults)
{
	vector<wstring> vect;
	vect.push_back(L"^[Pp][uU][Tt][Tt][Yy] .*");
	vect.push_back(L"^[sS][sS][hH] .*");
	*iNumResults = (int) vect.size();
	return StringVectorToTCHAR(vect);
	//*iNumResults = 0;
	//return NULL;
}

bool FoundAllKeywords(const wstring &haystack, const wstring &keywords) {
	wstring needle;
	wstringstream tokenizer(keywords);

	while (tokenizer >> needle) {
		if (wstring::npos == haystack.find(needle)) {
			return false;
		}
	}

	return true;
}

SearchResult* PluginUpdateSearch (int NumStrings, const TCHAR* Strings, const TCHAR* FinalString, int* NumResults)
{
	vector<SearchResult> results;
	HKEY key;
	int i;
	wchar_t keyname[255];
	wstring keyname_unmunged;
	wstring keyname_lower;
	wstring searchString = FinalString;
	back_insert_iterator<wstring> keyname_lower_output(keyname_lower);

	if (NumStrings == 0) {
		//we are in a regex
		wstring::size_type start;
		start = searchString.find_first_of(L" ");
		if (wstring::npos != start)
			searchString = searchString.substr(start);
	}else if (NumStrings >= 2) {
		//person has hit tab twice, so ignore it
		*NumResults = 0;
		return NULL;
	}

	transform(searchString.begin(), searchString.end(), searchString.begin(), tolower);

	//add sessions here
	if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_SESSION_REG_POS, &key) == ERROR_SUCCESS) {
		for (i=0; RegEnumKey(key, i, keyname, sizeof(keyname)) == ERROR_SUCCESS; ++i) {
			//remove %xx's
			unmungestr(wstring(keyname), keyname_unmunged);

			//make the keyname lowercase for searching
			keyname_lower.clear();
			transform(keyname_unmunged.begin(), keyname_unmunged.end(), keyname_lower_output, tolower);

			//only add if the search string is found
			if (FoundAllKeywords(keyname_lower, searchString)) {
				results.push_back(makeResult(keyname_unmunged, L"", PLUGIN_NAME L" " + keyname_unmunged, NULL));
			}
		}

		RegCloseKey(key);
	}

	*NumResults = (int) results.size();
	return ResultVectorToArray(results);
}

SearchResult* PluginFileOptions (const TCHAR* FullPath, int NumStrings, const TCHAR* Strings, const TCHAR* FinalString, int* NumResults) 
{
	*NumResults = 0;
	return NULL;
}


void PluginDoAction (int NumStrings, const TCHAR* Strings, const TCHAR* FinalString, const TCHAR* FullPath) {
	wstring cmd, params, profile, safe_profile;

	cmd = PathToPutty.empty() ? L"putty" : PathToPutty;

	if (NumStrings >= 2) {
		//try to remove header if person hit tab too many times
		vector<wstring> stringVec = TCHARListToVector(NumStrings, Strings);

		profile = stringVec[1];
	}else{
		profile = FullPath;
	}

	EscapeQuotes(profile, safe_profile);

	params = L"-load \"" + profile + L"\"";

	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = cmd.c_str();
	ShExecInfo.lpParameters = params.c_str();
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_NORMAL;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
}

TCHAR* PluginGetSeparator() {
	wstring tmp = L" ";
	return string2TCHAR(tmp);
}

TCHAR* PluginGetName() {
	wstring tmp = PLUGIN_NAME;
	return string2TCHAR(tmp);
}

TCHAR* PluginGetDescription() {
	wstring tmp = PLUGIN_DESCRIPTION;
	return string2TCHAR(tmp);
}

void PluginClose() {
	return;
}

void PluginInitialize() {
	PathToPutty = RetrieveString(L"PUTTY_PLUGIN_PATH_TO_PUTTY");
}

void PluginSaveOptions() {
	StoreString(L"PUTTY_PLUGIN_PATH_TO_PUTTY", PathToPutty);
}

//yucky interop between clr and native code here
void PluginCallOptionsDlg(HWND parent) {
	String^ tempString = gcnew String(PathToPutty.c_str());

	PuTTY::PluginOptionsForm^ options = gcnew PuTTY::PluginOptionsForm(tempString);

	if (::DialogResult::OK == options->ShowDialog()) {
		pin_ptr<const wchar_t> ptr = PtrToStringChars(options->PathToPutty());
		PathToPutty = ptr;
	}
}

bool PluginHasOptionsDlg() {
	return true;
}

