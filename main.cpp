#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <libzippp/libzippp.h>
#ifdef _WIN32
#include <codecvt>
#include <windows.h>
#ifdef UNICODE
#define main wmain
#endif
#else
#define TCHAR char
#define TEXT(x) x
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#endif	// _WIN32

using ZipArchive = libzippp::ZipArchive;
using ZipEntry = libzippp::ZipEntry;
using path_string = std::basic_string<TCHAR>;

bool Makedirectory(const TCHAR* path) {
#ifdef _WIN32
	return CreateDirectory(path, NULL) || ERROR_ALREADY_EXISTS == GetLastError();
#else
	return !mkdir(path, 0777) || errno == EEXIST;
#endif
}

bool CreatePath(const path_string& path, const path_string& workingdir = TEXT("./")) {
	std::vector<path_string> folders;
	path_string temp;
	for(int i = 0; i < (int)path.size(); i++) {
		if(path[i] == TEXT('/')) {
			folders.push_back(temp);
			temp.clear();
		} else
			temp += path[i];
	}
	temp.clear();
	for(auto folder : folders) {
		if(temp.empty() && !workingdir.empty())
			temp = workingdir + TEXT("/") + folder;
		else
			temp += TEXT("/") + folder;
		if(!Makedirectory(temp.c_str()))
			return false;
	}
	return true;
}

void ExtractTo(ZipArchive* archive, const path_string& path) {
	archive->open(ZipArchive::READ_ONLY);
	CreatePath(path + TEXT("/"));
	auto total = archive->getEntriesCount();
	for(auto& entry : archive->getEntries()) {
		if(entry.isNull()) continue;
#ifdef UNICODE
		path_string entryname = std::wstring_convert<std::codecvt_utf8_utf16<TCHAR>, TCHAR>{}.from_bytes(entry.getName());
#else
		path_string entryname = entry.getName();
#endif
		if(entry.isDirectory())
			CreatePath(entryname + TEXT("/"), path);
		else
			CreatePath(entryname, path);
		std::cout << "Extracting: " << entry.getName() << std::endl;
		if(entry.isFile()) {
			std::ofstream out(path + TEXT("/") + entryname, std::wifstream::binary);
			entry.readContent(out);
			out.close();
		}
	}
	//archive->close();
}

void LaunchEdopro(const TCHAR* progname) {
#ifdef _WIN32
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	path_string str(progname);
	str.append(TEXT(" show_changelog"));
	CreateProcess(nullptr, (LPWSTR)str.c_str(), NULL, NULL, FALSE, 0, NULL, TEXT("./"), &si, &pi);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#elif defined(__APPLE__)
	char launch_argument[256];
	sprintf(launch_argument, "open -b %s --args show_changelog", progname);
	system(launch_argument);
#else
	pid_t pid = fork();
	if(pid == 0) {
		execl(progname, "show_changelog");
		exit(EXIT_FAILURE);
	}
#endif
}

int main(int argc, TCHAR* argv[]) {
	if(argc < 4)
		return 0;
	std::vector<char> buff;
	for(int i = 3; i < argc; i++) {
		path_string name = argv[i];
		std::ifstream file(name, std::ifstream::in | std::ifstream::binary);
		buff = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		auto zf = ZipArchive::fromBuffer(buff.data(), buff.size());
		ExtractTo(zf, argv[1]);
	}
	LaunchEdopro(argv[2]);
	return 0;
}