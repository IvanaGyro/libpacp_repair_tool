#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdio>

#define HeaderBytes 24
#define MaxPkgBytes 65544  //65536+8
#define KeepDays 7
#define KeepSeconds (KeepDays*86400)
#define StartTimeOffset (-1*86400)  // -1 day

using namespace std;

typedef struct{
	int size;
	char data[MaxPkgBytes];
}pkg;

int catoi(const char* ca){
	char tmp[4];
	int* iptr;
	for (int i = 0; i < 4; i++){
		tmp[i] = ca[3 - i];
	}
	iptr = reinterpret_cast<int*>(tmp);
	return *iptr;
}

#ifdef _MSC_VER
#include <windows.h>
#include <iomanip>
wstring str2wstr(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	wstring wstr(buf);
	return wstr;
}
#endif // _MSC_VER


int main(int argc, char** argv){
	string inFileName, outFileName;
	stringstream outBuf;
	fstream fs_in, fs_out;
	char buf_char;
	int buf_int, headercount = 0, curPkgIdx= 0, lastPkgIdx = 1, tmp;
	bool isBroken = false, isValid;
	clock_t mytime;
	unsigned int StartTime = 0, PkgTime;
	pkg buf_pkg[2];

	if (argc != 2){
		return 1;
	}


	inFileName = argv[1];
	fs_in.open(inFileName, ios::binary | ios::in);
	if (!fs_in){
		cout << "Can't open the file: " << inFileName << endl;
		return 1;
	}

	outFileName = inFileName;
	outFileName.insert(outFileName.rfind('.'), "_integrated");
	fs_out.open(outFileName, ios::binary | ios::out);
	if (!fs_out){
		cout << "Can't open the file: " << outFileName << endl;
		return 1;
		
	}


	int invalidPConuter = 0;
	long long outBufMaxPos = 0;

	
	buf_pkg[0].size = 0;
	buf_pkg[1].size = 0;

	mytime = clock();
	fs_in.read(buf_pkg[curPkgIdx].data, HeaderBytes);
	outBuf.write(buf_pkg[curPkgIdx].data, HeaderBytes);
	if (fs_in){
		fs_in.read(buf_pkg[curPkgIdx].data, 4);
		StartTime = catoi(buf_pkg[curPkgIdx].data);
		StartTime += StartTimeOffset;
		fs_in.seekg(-4, ios_base::cur);
	}
	cout << "start" << endl;
	while (fs_in.get(buf_char)){
		fs_in.seekg(-1, ios_base::cur);
		if (buf_char == -95 ){    //0xa1
			fs_in.read(reinterpret_cast<char*>(&buf_int), sizeof(int));
 			if (buf_int == 0xd4c3b2a1){  //a1b2 c3d4
				fs_in.seekg(HeaderBytes-4, ios_base::cur);
				headercount++;
			}
			else fs_in.seekg(-4, ios_base::cur);
		}
		else{
			fs_in.read(buf_pkg[curPkgIdx].data, 16);
			PkgTime = catoi(buf_pkg[curPkgIdx].data);

			/*Set isValid*/
			if (PkgTime - StartTime < KeepSeconds) isValid = true;
			else isValid = false;

			if (isValid){  //last packetage is valid
				/*store size of packetage*/
				buf_pkg[curPkgIdx].size = catoi(buf_pkg[curPkgIdx].data + 8);
				/*store size of packetage*/
				if (buf_pkg[curPkgIdx].size > MaxPkgBytes) isValid = false;
			}
			if (isValid) //Pass packet size check
			{
				/*read packetage data*/
				fs_in.read(buf_pkg[curPkgIdx].data + 16, buf_pkg[curPkgIdx].size);
				buf_pkg[curPkgIdx].size += 16;
				/*read packetage data*/

				/*write last packetage data*/
				outBuf.write(buf_pkg[lastPkgIdx].data, buf_pkg[lastPkgIdx].size);
				if (static_cast<long long>(outBuf.tellp()) > outBufMaxPos)
				{
					outBufMaxPos = static_cast<long long>(outBuf.tellp());
				}
				else if (static_cast<long long>(outBuf.tellp()) == -1)
				{
					cout << "outBufMaxPos: " << outBufMaxPos << endl;
					system("pause");
				}

				if (outBuf.tellp() >= 0x40000000 - MaxPkgBytes) // 1GB
				{
					cout << "write" << endl;
					fs_out << outBuf.rdbuf();
					outBuf.str("");
					outBuf.clear();
				}
				/*write last packetage data*/

				/*swap idx of buffer*/
				tmp = curPkgIdx;
				curPkgIdx = lastPkgIdx;
				lastPkgIdx = tmp;
				/*swap idx of buffer*/
			}
			if (!isValid)
			{
				++invalidPConuter;
				isBroken = true;
				fs_in.seekg(-buf_pkg[lastPkgIdx].size - 15, ios_base::cur);

				/*search correct packetage byte by byte*/
				int tmpflag = 0;

				/*Let PkgTime be invalid.
				If packet is invalid because of its size, original PkgTime was valid*/
				PkgTime = StartTime + KeepSeconds; 

				while (PkgTime - StartTime >= KeepSeconds && fs_in.read(buf_pkg[curPkgIdx].data, 4)){
					PkgTime = catoi(buf_pkg[curPkgIdx].data);
					fs_in.seekg(-3, ios_base::cur);
				}
				fs_in.seekg(-1, ios_base::cur);
				/*search correct packetage byte by byte*/

				buf_pkg[lastPkgIdx].size = 0; //reset the size of the invalid packetage
			}
		}
	}
	fs_in.close();

	mytime = clock() - mytime;
	cout << "Repair pacp: " << mytime << " miniseconds." << endl;
	cout << "Number of deleted headers: " << headercount << endl;


	mytime = clock();

	if (headercount || isBroken){
		fs_out << outBuf.rdbuf();
		fs_out.close();
#ifdef _MSC_VER
		wstring originFileName, newFileName;
		originFileName = str2wstr(inFileName);
		newFileName = str2wstr(inFileName.insert(inFileName.rfind("."), "_origin"));

		int flag = MoveFileExW(originFileName.c_str(), newFileName.c_str(), 0);
		if (!flag)
		{
			cout << "fail to rename origin file" << endl;
			cout << showbase // show the 0x prefix
				<< internal // fill between the prefix and the number
				<< setfill('0'); // fill with 0s
			cout << "Error code: " << hex << setw(4) << GetLastError() << dec << endl;
		}
		else
		{
			newFileName = originFileName;
			originFileName = str2wstr(outFileName);
			flag = MoveFileExW(originFileName.c_str(), newFileName.c_str(), 0);
			if (!flag)
			{
				cout << "fail to rename output file" << endl;
				cout << showbase // show the 0x prefix
					<< internal // fill between the prefix and the number
					<< setfill('0'); // fill with 0s
				cout << "Error code: " << hex << setw(4) << GetLastError() << dec << endl;
			}
		}

#endif //_MSC_VER 		

	}
	else
	{
		wstring tmpwstr = str2wstr(outFileName);
		fs_out.close();
		if (!DeleteFileW(tmpwstr.c_str()))
		{
			cout << "Cannot deleted tmp file (integrated)" << endl;
		}
		cout << "The file is completed. Do nothing." << endl;
	}

	mytime = clock() - mytime;
	cout << "Rename file: " << mytime << " miniseconds." << endl;
	system("pause"); 
	return 0;

}