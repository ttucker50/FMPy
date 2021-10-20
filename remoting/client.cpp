#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#include <iostream>

using namespace std;

#include "fmi2Functions.h"

#include "remoting.h"


#define BUF_SIZE (1024*16)
TCHAR szName[] = TEXT("MyFileMappingObject");
TCHAR szMsg[] = TEXT("Message from first process.");

HANDLE inputReady = INVALID_HANDLE_VALUE;
HANDLE outputReady = INVALID_HANDLE_VALUE;

HANDLE hMapFile;
LPTSTR pBuf;

fmi2CallbackLogger s_logger = NULL;
fmi2ComponentEnvironment s_componentEnvironment = NULL;

//#define BUFSIZE 4096 
//
//HANDLE g_hChildStd_IN_Rd = NULL;
//HANDLE g_hChildStd_IN_Wr = NULL;
//HANDLE g_hChildStd_OUT_Rd = NULL;
//HANDLE g_hChildStd_OUT_Wr = NULL;
//
////HANDLE g_hInputFile = NULL;
////msgpack_sbuffer sbuf;
//
//void CreateChildProcess(void);
//void WriteToPipe(msgpack_sbuffer &sbuf, msgpack_object &deserialized);
//void ReadFromPipe(void);
//void ErrorExit(PTSTR);
//
//#define RETURN_FMI2STATUS return (fmi2Status)deserialized.via.array.ptr[0].via.i64;
//
////int main()
////{
////    CreateChildProcess();
////
////    WriteToPipe();
////
////    printf("\nContents of child process STDOUT: >>>");
////    ReadFromPipe();
////    printf("<<< End of parent execution.\n");
////
////    return 0;
////}
//
//void CreateChildProcess()
//// Create a child process that uses the previously created pipes for STDIN and STDOUT.
//{
//    SECURITY_ATTRIBUTES saAttr;
//
//    printf("\n->Start of parent execution.\n");
//
//    // Set the bInheritHandle flag so pipe handles are inherited. 
//
//    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
//    saAttr.bInheritHandle = TRUE;
//    saAttr.lpSecurityDescriptor = NULL;
//
//    // Create a pipe for the child process's STDOUT. 
//
//    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
//        ErrorExit(TEXT("StdoutRd CreatePipe"));
//
//    // Ensure the read handle to the pipe for STDOUT is not inherited.
//
//    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
//        ErrorExit(TEXT("Stdout SetHandleInformation"));
//
//    // Create a pipe for the child process's STDIN. 
//
//    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
//        ErrorExit(TEXT("Stdin CreatePipe"));
//
//    // Ensure the write handle to the pipe for STDIN is not inherited. 
//
//    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
//        ErrorExit(TEXT("Stdin SetHandleInformation"));
//
//    TCHAR szCmdline[] = TEXT("server");
//    PROCESS_INFORMATION piProcInfo;
//    STARTUPINFO siStartInfo;
//    BOOL bSuccess = FALSE;
//
//    // Set up members of the PROCESS_INFORMATION structure. 
//
//    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
//
//    // Set up members of the STARTUPINFO structure. 
//    // This structure specifies the STDIN and STDOUT handles for redirection.
//
//    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
//    siStartInfo.cb = sizeof(STARTUPINFO);
//    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
//    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
//    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
//    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
//
//    // Create the child process.
//
//    char path[MAX_PATH];
//    
//    HMODULE hm = NULL;
//    
//    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
//    	GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
//    	(LPCSTR)&fmi2GetTypesPlatform, &hm) == 0) {
//    	int ret = GetLastError();
//    	//fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
//    	// Return or however you want to handle an error.
//    }
//    
//    if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
//    	int ret = GetLastError();
//    	//fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
//    	// Return or however you want to handle an error.
//	}
//
//    const std::string filename(path);
//
//    const std::string linux64Path = filename.substr(0, filename.find_last_of('\\'));
//
//    const std::string modelIdentifier = filename.substr(filename.find_last_of('\\') + 1, filename.find_last_of('.') - filename.find_last_of('\\') - 1);
//
//    const std::string binariesPath = linux64Path.substr(0, linux64Path.find_last_of('\\'));
//
////    if (!modelIdentifier.compare("client")) {
////
////        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Remoting server started externally.");
////	
////    } else {
//
//    const std::string command = binariesPath + "\\win32\\server.exe " + binariesPath + "\\win32\\" + modelIdentifier + ".dll";
//
//    std::cout << "Command: " << command << std::endl;
//
//    bSuccess = CreateProcess(NULL,
//        (LPSTR)command.c_str(),     // command line 
//        NULL,          // process security attributes 
//        NULL,          // primary thread security attributes 
//        TRUE,          // handles are inherited 
//        0,             // creation flags 
//        NULL,          // use parent's environment 
//        NULL,          // use parent's current directory 
//        &siStartInfo,  // STARTUPINFO pointer 
//        &piProcInfo);  // receives PROCESS_INFORMATION 
//
//     // If an error occurs, exit the application. 
//    if (!bSuccess)
//        ErrorExit(TEXT("CreateProcess"));
//    else
//    {
//        // Close handles to the child process and its primary thread.
//        // Some applications might keep these handles to monitor the status
//        // of the child process, for example. 
//
//        CloseHandle(piProcInfo.hProcess);
//        CloseHandle(piProcInfo.hThread);
//
//        // Close handles to the stdin and stdout pipes no longer needed by the child process.
//        // If they are not explicitly closed, there is no way to recognize that the child process has ended.
//
//        CloseHandle(g_hChildStd_OUT_Wr);
//        CloseHandle(g_hChildStd_IN_Rd);
//    }
//
//    /* msgpack::sbuffer is a simple buffer implementation. */
//    //msgpack_sbuffer_init(&sbuf);
//}
//
//void WriteToPipe(msgpack_sbuffer &sbuf, msgpack_object &deserialized)
//
//// Read from a file and write its contents to the pipe for the child's STDIN.
//// Stop when there is no more data. 
//{    
//    DWORD dwWritten;
//    BOOL bSuccess = FALSE;
//
//    bSuccess = WriteFile(g_hChildStd_IN_Wr, &sbuf.size, sizeof(size_t), &dwWritten, NULL);
//
//    bSuccess = WriteFile(g_hChildStd_IN_Wr, sbuf.data, sbuf.size, &dwWritten, NULL);
//
//    //if (!bSuccess) break;
//    //}
//
//    DWORD dwRead;
//
//    bSuccess = ReadFile(g_hChildStd_OUT_Rd, &sbuf.size, sizeof(size_t), &dwRead, NULL);
//
//    bSuccess = ReadFile(g_hChildStd_OUT_Rd, sbuf.data, sbuf.size, &dwRead, NULL);
//
//    msgpack_zone mempool;
//    msgpack_zone_init(&mempool, 2048);
//
//    msgpack_unpack(sbuf.data, sbuf.size, NULL, &mempool, &deserialized);
//
//    ///* print the deserialized object. */
//    //msgpack_object_print(stdout, deserialized);
//    //puts("");
//}
//
//void ReadFromPipe(void)
//
//// Read output from the child process's pipe for STDOUT
//// and write to the parent process's pipe for STDOUT. 
//// Stop when there is no more data. 
//{
//    DWORD dwRead, dwWritten;
//    CHAR chBuf[BUFSIZE];
//    BOOL bSuccess = FALSE;
//    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
//
//    for (;;)
//    {
//        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
//        if (!bSuccess || dwRead == 0) break;
//
//        bSuccess = WriteFile(hParentStdOut, chBuf,
//            dwRead, &dwWritten, NULL);
//        if (!bSuccess) break;
//    }
//}
//
//void ErrorExit(PTSTR lpszFunction)
//
//// Format a readable error message, display a message box, 
//// and exit from the application.
//{
//    LPVOID lpMsgBuf;
//    LPVOID lpDisplayBuf;
//    DWORD dw = GetLastError();
//
//    FormatMessage(
//        FORMAT_MESSAGE_ALLOCATE_BUFFER |
//        FORMAT_MESSAGE_FROM_SYSTEM |
//        FORMAT_MESSAGE_IGNORE_INSERTS,
//        NULL,
//        dw,
//        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//        (LPTSTR)&lpMsgBuf,
//        0, NULL);
//
//    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
//        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
//    StringCchPrintf((LPTSTR)lpDisplayBuf,
//        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
//        TEXT("%s failed with error %d: %s"),
//        lpszFunction, dw, lpMsgBuf);
//    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
//
//    LocalFree(lpMsgBuf);
//    LocalFree(lpDisplayBuf);
//    ExitProcess(1);
//}
//
////#ifdef _WIN32
////#include "Windows.h"
////#include "Shlwapi.h"
////#pragma comment(lib, "shlwapi.lib")
////#pragma warning(disable:4996)  // for strdup()
////#else
////#include <dlfcn.h>
////#include <libgen.h>
////#include <unistd.h>
////#include <signal.h>
////#include <sys/wait.h>
////#define MAX_PATH 2048
////#endif
////
////#include <chrono>
////#include <thread>
////#include <iostream>
////#include <vector>
////#include "rpc/client.h"
////
////#include "remoting.h"
////#include "fmi2Functions.h"
////
////
////using namespace std;
////
////using double_vector = vector<double>;
////
////
////static void functionInThisDll() {}
////
////static rpc::client *client = nullptr;
////
#ifdef _WIN32
static 	PROCESS_INFORMATION s_proccessInfo = { 0 };
#else
pid_t s_pid = 0;
#endif
////
////static fmi2CallbackLogger s_logger = nullptr;
////static fmi2ComponentEnvironment s_componentEnvironment = nullptr;
////static char *s_instanceName = nullptr;

#define NOT_IMPLEMENTED return fmi2Error;


static fmi2Status makeRPC(rpcFunction rpc) {

    fmi2Status status = fmi2Discard;

    memcpy(&pBuf[1024 * 0], &rpc, sizeof(rpcFunction));
    memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

    DWORD dwEvent = SignalObjectAndWait(inputReady, outputReady, INFINITE, TRUE);

    status = *((fmi2Status*)&pBuf[1024 * 10]);

    return status;
}

/***************************************************
Types for Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* fmi2GetTypesPlatform() {
    return fmi2TypesPlatform;
}

const char* fmi2GetVersion() {
    return fmi2Version;
}

////static void forwardLogMessages(const list<LogMessage> &logMessages) {
////	for (auto it = logMessages.begin(); it != logMessages.end(); it++) {
////		auto &m = *it;
////		s_logger(s_componentEnvironment, m.instanceName.c_str(), fmi2Status(m.status), m.category.c_str(), m.message.c_str());
////	}
////}
////
////static fmi2Status handleReturnValue(ReturnValue r) {
////	forwardLogMessages(r.logMessages);
////	return fmi2Status(r.status);
////}
//
//fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn,	size_t nCategories,	const fmi2String categories[]) {
//	NOT_IMPLEMENTED
//}
//
//
//static void handleLogMessages(msgpack_object_array logMessages) {
//
//    for (int i = 0; i < logMessages.size; i++) {
//        auto m = logMessages.ptr[i].via.array;
//        auto instanceName = m.ptr[0].via.str.ptr;
//        auto status = m.ptr[1].via.i64;
//        auto category = m.ptr[2].via.str.ptr;
//        auto message = m.ptr[3].via.str.ptr;
//        s_logger(s_componentEnvironment, instanceName, (fmi2Status)status, category, message);
//    }
//}

/* Creation and destruction of FMU instances and setting debug status */
fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn) {

    s_logger = functions->logger;
    s_componentEnvironment = functions->componentEnvironment;

    inputReady = CreateEvent(
        NULL,           // default security attributes
        FALSE,          // auto-reset event object
        FALSE,          // initial state is nonsignaled
        "inputReady");  // unnamed object

    outputReady = CreateEvent(
        NULL,           // default security attributes
        FALSE,          // auto-reset event object
        FALSE,          // initial state is nonsignaled
        "outputReady"); // unnamed object

    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        BUF_SIZE,                // maximum object size (low-order DWORD)
        szName);                 // name of mapping object

    if (hMapFile == NULL)
    {
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),
            GetLastError());
        return NULL;
    }
    pBuf = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBuf == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());

        CloseHandle(hMapFile);

        return NULL;
    }

    strncpy(&pBuf[1024 * 1], instanceName, 1024);
    memcpy(&pBuf[1024 * 2], &fmuType, sizeof(fmi2Type));
    strncpy(&pBuf[1024 * 3], fmuGUID, 1024);
    strncpy(&pBuf[1024 * 4], fmuResourceLocation, 1024);
    memcpy(&pBuf[1024 * 5], &visible, sizeof(fmi2Boolean));
    memcpy(&pBuf[1024 * 6], &loggingOn, sizeof(fmi2Boolean));

////#ifdef _WIN32
    char path[MAX_PATH];

	HMODULE hm = NULL;

	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&fmi2GetTypesPlatform, &hm) == 0) {
		int ret = GetLastError();
		//fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}

	if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
		int ret = GetLastError();
		//fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}

    const string filename(path);

    const string linux64Path = filename.substr(0, filename.find_last_of('\\'));

    const string modelIdentifier = filename.substr(filename.find_last_of('\\') + 1, filename.find_last_of('.') - filename.find_last_of('\\') - 1);

    const string binariesPath = linux64Path.substr(0, linux64Path.find_last_of('\\'));

    if (!modelIdentifier.compare("client")) {

        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Remoting server started externally.");
	
    } else {

        const string command = binariesPath + "\\win32\\server.exe " + binariesPath + "\\win32\\" + modelIdentifier + ".dll";

		// additional information
		STARTUPINFO si;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&s_proccessInfo, sizeof(s_proccessInfo));

        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Starting remoting server. Command: %s", command.c_str());

		// start the program up
		const BOOL success = CreateProcessA(NULL, // the path
            (LPSTR)command.c_str(),               // command line
			NULL,                                 // process handle not inheritable
			NULL,                                 // thread handle not inheritable
			FALSE,                                // set handle inheritance to FALSE
            CREATE_NO_WINDOW,                     // creation flags
			NULL,                                 // use parent's environment block
			NULL,                                 // use parent's starting directory 
			&si,                                  // pointer to STARTUPINFO structure
			&s_proccessInfo                       // pointer to PROCESS_INFORMATION structure
		);

        if (success) {
            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Server process id is %d.", s_proccessInfo.dwProcessId);
        } else {
            s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to start server.");
            return nullptr;
        }
	}
////#else
////
////    Dl_info info = { nullptr };
////
////    dladdr((const void *)functionInThisDll, &info);
////
////    const string filename(info.dli_fname);
////
////    const string linux64Path = filename.substr(0, filename.find_last_of('/'));
////    
////    const string modelIdentifier = filename.substr(filename.find_last_of('/') + 1, filename.find_last_of('.') - filename.find_last_of('/') - 1);
////
////    const string binariesPath = linux64Path.substr(0, linux64Path.find_last_of('/'));
////
////    if (!modelIdentifier.compare("client")) {
////    
////        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Remoting server started externally.");
////    
////    } else {
////
////        const pid_t pid = fork();
////
////        if (pid < 0) {
////
////            s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to create server process.");
////            return nullptr;
////
////        } else if (pid == 0) {
////
////            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Child process (pid = %d).", pid);
////
////            pid_t pgid = setsid();
////
////            if (pgid == -1) {
////                s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to create session id.");
////                return nullptr;
////            }
////
////            const string command = "wine64 " + binariesPath + "/win64/server.exe " + binariesPath + "/win64/" + modelIdentifier + ".dll";
////
////            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Starting server. Command: %s", command.c_str());
////
////            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
////
////            s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to start server.");
////
////            return nullptr;
////
////        } else {
////
////            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Server process id is %d.", pid);
////            s_pid = pid;
////
////        }
////
////    }
////#endif

    fmi2Status status = makeRPC(rpc_fmi2Instantiate);

    if (status > fmi2Warning) {
        return NULL;
    }

    return (fmi2Component)0x1;
}

void fmi2FreeInstance(fmi2Component c) {

    fmi2Status status = makeRPC(rpc_fmi2FreeInstance);

    UnmapViewOfFile(pBuf);

    CloseHandle(hMapFile);

    CloseHandle(inputReady);
    CloseHandle(outputReady);
}

/* Enter and exit initialization mode, terminate and reset */
fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {

    memcpy(&pBuf[1024 * 1], &toleranceDefined, sizeof(fmi2Boolean));
    memcpy(&pBuf[1024 * 2], &tolerance, sizeof(fmi2Real));
    memcpy(&pBuf[1024 * 3], &startTime, sizeof(fmi2Real));
    memcpy(&pBuf[1024 * 4], &stopTimeDefined, sizeof(fmi2Boolean));
    memcpy(&pBuf[1024 * 5], &stopTime, sizeof(fmi2Real));

    fmi2Status status = makeRPC(rpc_fmi2SetupExperiment);

    return status;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    return makeRPC(rpc_fmi2EnterInitializationMode);
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    return makeRPC(rpc_fmi2ExitInitializationMode);
}

fmi2Status fmi2Terminate(fmi2Component c) {
    return makeRPC(rpc_fmi2Terminate);
}

fmi2Status fmi2Reset(fmi2Component c) {
    return makeRPC(rpc_fmi2Reset);
}

/* Getting and setting variable values */
fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    
    memcpy(&pBuf[1024 * 1], vr, sizeof(fmi2ValueReference) * nvr);
    memcpy(&pBuf[1024 * 2], &nvr, sizeof(size_t));

    fmi2Status status = makeRPC(rpc_fmi2GetReal);

    memcpy(value, &pBuf[1024 * 3], sizeof(fmi2Real) * nvr);

    return status;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    
    memcpy(&pBuf[1024 * 1], vr, sizeof(fmi2ValueReference) * nvr);
    memcpy(&pBuf[1024 * 2], &nvr, sizeof(size_t));

    fmi2Status status = makeRPC(rpc_fmi2GetInteger);

    memcpy(value, &pBuf[1024 * 3], sizeof(fmi2Integer) * nvr);

    return status;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    
    memcpy(&pBuf[1024 * 1], vr, sizeof(fmi2ValueReference) * nvr);
    memcpy(&pBuf[1024 * 2], &nvr, sizeof(size_t));

    fmi2Status status = makeRPC(rpc_fmi2GetBoolean);

    memcpy(value, &pBuf[1024 * 3], sizeof(fmi2Boolean) * nvr);

    return status;
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String  value[]) {
	NOT_IMPLEMENTED
}

fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {

    memcpy(&pBuf[1024 * 1], vr, sizeof(fmi2ValueReference) * nvr);
    memcpy(&pBuf[1024 * 2], &nvr, sizeof(size_t));
    memcpy(&pBuf[1024 * 3], value, sizeof(fmi2Real) * nvr);

    return makeRPC(rpc_fmi2SetReal);
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {

    memcpy(&pBuf[1024 * 1], vr, sizeof(fmi2ValueReference) * nvr);
    memcpy(&pBuf[1024 * 2], &nvr, sizeof(size_t));
    memcpy(&pBuf[1024 * 3], value, sizeof(fmi2Integer) * nvr);

    return makeRPC(rpc_fmi2SetInteger);
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {

    memcpy(&pBuf[1024 * 1], vr, sizeof(fmi2ValueReference) * nvr);
    memcpy(&pBuf[1024 * 2], &nvr, sizeof(size_t));
    memcpy(&pBuf[1024 * 3], value, sizeof(fmi2Boolean) * nvr);

    return makeRPC(rpc_fmi2SetBoolean);
}

fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String  value[]) {
	NOT_IMPLEMENTED
}

/* Getting and setting the internal FMU state */
fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
	NOT_IMPLEMENTED
}

fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate  FMUstate) {
	NOT_IMPLEMENTED
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
	NOT_IMPLEMENTED
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate  FMUstate, size_t* size) {
	NOT_IMPLEMENTED
}

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate  FMUstate, fmi2Byte[], size_t size) {
	NOT_IMPLEMENTED
}

fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
	NOT_IMPLEMENTED
}

/* Getting partial derivatives */
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown, const fmi2ValueReference vKnown_ref[], size_t nKnown, const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
	NOT_IMPLEMENTED
}

/***************************************************
Types for Functions for FMI2 for Model Exchange
****************************************************/

/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo* eventInfo) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2CompletedIntegratorStep(fmi2Component c,	fmi2Boolean  noSetFMUStatePriorToCurrentPoint, fmi2Boolean* enterEventMode, fmi2Boolean* terminateSimulation) {
    NOT_IMPLEMENTED
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx) {
    NOT_IMPLEMENTED
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real x[], size_t nx) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {
    NOT_IMPLEMENTED
}

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], const fmi2Real value[]) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], fmi2Real value[]) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    
    memcpy(&pBuf[1024 * 1], &currentCommunicationPoint, sizeof(fmi2Real));
    memcpy(&pBuf[1024 * 2], &communicationStepSize, sizeof(fmi2Real));
    memcpy(&pBuf[1024 * 3], &noSetFMUStatePriorToCurrentPoint, sizeof(fmi2Boolean));

    fmi2Status status = makeRPC(rpc_fmi2DoStep);

    return status;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    NOT_IMPLEMENTED
}

/* Inquire slave status */
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String*  value) {
	NOT_IMPLEMENTED
}
