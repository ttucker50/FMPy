#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <stdint.h>
#include <iostream>

//#include "rpc/msgpack.hpp"
#include <msgpack.h>

#include "fmi2Functions.h"


#define BUFSIZE 4096 

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

//HANDLE g_hInputFile = NULL;

void CreateChildProcess(void);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(PTSTR);

int main()
{
    SECURITY_ATTRIBUTES saAttr;

    printf("\n->Start of parent execution.\n");

    // Set the bInheritHandle flag so pipe handles are inherited. 

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 

    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
        ErrorExit(TEXT("StdoutRd CreatePipe"));

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
        ErrorExit(TEXT("Stdout SetHandleInformation"));

    // Create a pipe for the child process's STDIN. 

    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
        ErrorExit(TEXT("Stdin CreatePipe"));

    // Ensure the write handle to the pipe for STDIN is not inherited. 

    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
        ErrorExit(TEXT("Stdin SetHandleInformation"));

    // Create the child process. 

    CreateChildProcess();

    // Get a handle to an input file for the parent. 
    // This example assumes a plain text file and uses string output to verify data flow. 

    //if (argc == 1)
    //    ErrorExit(TEXT("Please specify an input file.\n"));

    //g_hInputFile = CreateFile(
    //    argv[1],
    //    GENERIC_READ,
    //    0,
    //    NULL,
    //    OPEN_EXISTING,
    //    FILE_ATTRIBUTE_READONLY,
    //    NULL);

    //if (g_hInputFile == INVALID_HANDLE_VALUE)
    //    ErrorExit(TEXT("CreateFile"));

    // Write to the pipe that is the standard input for a child process. 
    // Data is written to the pipe's buffers, so it is not necessary to wait
    // until the child process is running before writing data.

    WriteToPipe();
    //printf("\n->Contents of %S written to child STDIN pipe.\n", argv[1]);

    // Read from pipe that is the standard output for child process. 

    printf("\n->Contents of child process STDOUT:\n\n");
    ReadFromPipe();

    printf("\n->End of parent execution.\n");

    // The remaining open handles are cleaned up when this process terminates. 
    // To avoid resource leaks in a larger application, close handles explicitly. 

    return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
    TCHAR szCmdline[] = TEXT("server");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process. 

    bSuccess = CreateProcess(NULL,
        szCmdline,     // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION 

     // If an error occurs, exit the application. 
    if (!bSuccess)
        ErrorExit(TEXT("CreateProcess"));
    else
    {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example. 

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        // Close handles to the stdin and stdout pipes no longer needed by the child process.
        // If they are not explicitly closed, there is no way to recognize that the child process has ended.

        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }
}

void WriteToPipe(void)

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{
    
    /* msgpack::sbuffer is a simple buffer implementation. */
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);

    /* serialize values into the buffer using msgpack_sbuffer_write callback function. */
    msgpack_packer pk;
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_int(&pk, 1);
    msgpack_pack_true(&pk);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "example", 7);

    /* deserialize the buffer into msgpack_object instance. */
    /* deserialized object is valid during the msgpack_zone instance alive. */
    msgpack_zone mempool;
    msgpack_zone_init(&mempool, 2048);

    msgpack_object deserialized;
    msgpack_unpack(sbuf.data, sbuf.size, NULL, &mempool, &deserialized);

    /* print the deserialized object. */
    msgpack_object_print(stdout, deserialized);
    puts("");

    auto array = deserialized.via.array;

    auto o1 = array.ptr[0];
    auto v1 = o1.via.i64;

    auto o2 = array.ptr[1];
    auto v2 = o1.via.boolean;

    auto o3 = array.ptr[2];
    auto v3 = o3.via.str;

    std::string s3(v3.ptr, v3.size);

    msgpack_zone_destroy(&mempool);
    msgpack_sbuffer_destroy(&sbuf);
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    DWORD dwWritten;
    //CHAR chBuf[BUFSIZE] = "dummy";
    BOOL bSuccess = FALSE;
    //DWORD dwRead = strlen(chBuf);

    char *msg = "A dummy message.";
    uint64_t length = strlen(msg);

    //for (;;)
    //{
    //    bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
    //    if (!bSuccess || dwRead == 0) break;

    bSuccess = WriteFile(g_hChildStd_IN_Wr, &length, sizeof(uint64_t), &dwWritten, NULL);

    bSuccess = WriteFile(g_hChildStd_IN_Wr, msg, length, &dwWritten, NULL);

    //if (!bSuccess) break;
    //}

    // Close the pipe handle so the child process stops reading. 

    if (!CloseHandle(g_hChildStd_IN_Wr))
        ErrorExit(TEXT("StdInWr CloseHandle"));
}

void ReadFromPipe(void)

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{
    DWORD dwRead, dwWritten;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    for (;;)
    {
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;

        bSuccess = WriteFile(hParentStdOut, chBuf,
            dwRead, &dwWritten, NULL);
        if (!bSuccess) break;
    }
}

void ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box, 
// and exit from the application.
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}

//#ifdef _WIN32
//#include "Windows.h"
//#include "Shlwapi.h"
//#pragma comment(lib, "shlwapi.lib")
//#pragma warning(disable:4996)  // for strdup()
//#else
//#include <dlfcn.h>
//#include <libgen.h>
//#include <unistd.h>
//#include <signal.h>
//#include <sys/wait.h>
//#define MAX_PATH 2048
//#endif
//
//#include <chrono>
//#include <thread>
//#include <iostream>
//#include <vector>
//#include "rpc/client.h"
//
//#include "remoting.h"
//#include "fmi2Functions.h"
//
//
//using namespace std;
//
//using double_vector = vector<double>;
//
//
//static void functionInThisDll() {}
//
//static rpc::client *client = nullptr;
//
//#ifdef _WIN32
//static 	PROCESS_INFORMATION s_proccessInfo = { 0 };
//#else
//pid_t s_pid = 0;
//#endif
//
//static fmi2CallbackLogger s_logger = nullptr;
//static fmi2ComponentEnvironment s_componentEnvironment = nullptr;
//static char *s_instanceName = nullptr;
//
//#define NOT_IMPLEMENTED return fmi2Error;
//
//
/***************************************************
Types for Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* fmi2GetTypesPlatform() {
    main();
	return fmi2TypesPlatform;
}

const char* fmi2GetVersion() {
    return fmi2Version;
}

//static void forwardLogMessages(const list<LogMessage> &logMessages) {
//	for (auto it = logMessages.begin(); it != logMessages.end(); it++) {
//		auto &m = *it;
//		s_logger(s_componentEnvironment, m.instanceName.c_str(), fmi2Status(m.status), m.category.c_str(), m.message.c_str());
//	}
//}
//
//static fmi2Status handleReturnValue(ReturnValue r) {
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn,	size_t nCategories,	const fmi2String categories[]) {
//	NOT_IMPLEMENTED
//}
//
///* Creation and destruction of FMU instances and setting debug status */
//fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn) {
//	
//	s_logger = functions->logger;
//    s_componentEnvironment = functions->componentEnvironment;
//    s_instanceName = strdup(instanceName);
//
//#ifdef _WIN32
//    char path[MAX_PATH];
//
//	HMODULE hm = NULL;
//
//	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
//		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
//		(LPCSTR)&functionInThisDll, &hm) == 0) {
//		int ret = GetLastError();
//		//fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
//		// Return or however you want to handle an error.
//	}
//
//	if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
//		int ret = GetLastError();
//		//fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
//		// Return or however you want to handle an error.
//	}
//
//    const string filename(path);
//
//    const string linux64Path = filename.substr(0, filename.find_last_of('\\'));
//
//    const string modelIdentifier = filename.substr(filename.find_last_of('\\') + 1, filename.find_last_of('.') - filename.find_last_of('\\') - 1);
//
//    const string binariesPath = linux64Path.substr(0, linux64Path.find_last_of('\\'));
//
//    if (!modelIdentifier.compare("client")) {
//
//        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Remoting server started externally.");
//	
//    } else {
//
//        const string command = binariesPath + "\\win32\\server.exe " + binariesPath + "\\win32\\" + modelIdentifier + ".dll";
//
//		// additional information
//		STARTUPINFO si;
//
//		// set the size of the structures
//		ZeroMemory(&si, sizeof(si));
//		si.cb = sizeof(si);
//		ZeroMemory(&s_proccessInfo, sizeof(s_proccessInfo));
//
//        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Starting remoting server. Command: %s", command.c_str());
//
//		// start the program up
//		const BOOL success = CreateProcessA(NULL, // the path
//            (LPSTR)command.c_str(),               // command line
//			NULL,                                 // process handle not inheritable
//			NULL,                                 // thread handle not inheritable
//			FALSE,                                // set handle inheritance to FALSE
//            CREATE_NO_WINDOW,                     // creation flags
//			NULL,                                 // use parent's environment block
//			NULL,                                 // use parent's starting directory 
//			&si,                                  // pointer to STARTUPINFO structure
//			&s_proccessInfo                       // pointer to PROCESS_INFORMATION structure
//		);
//
//        if (success) {
//            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Server process id is %d.", s_proccessInfo.dwProcessId);
//        } else {
//            s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to start server.");
//            return nullptr;
//        }
//	}
//
//#else
//
//    Dl_info info = { nullptr };
//
//    dladdr((const void *)functionInThisDll, &info);
//
//    const string filename(info.dli_fname);
//
//    const string linux64Path = filename.substr(0, filename.find_last_of('/'));
//    
//    const string modelIdentifier = filename.substr(filename.find_last_of('/') + 1, filename.find_last_of('.') - filename.find_last_of('/') - 1);
//
//    const string binariesPath = linux64Path.substr(0, linux64Path.find_last_of('/'));
//
//    if (!modelIdentifier.compare("client")) {
//    
//        s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Remoting server started externally.");
//    
//    } else {
//
//        const pid_t pid = fork();
//
//        if (pid < 0) {
//
//            s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to create server process.");
//            return nullptr;
//
//        } else if (pid == 0) {
//
//            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Child process (pid = %d).", pid);
//
//            pid_t pgid = setsid();
//
//            if (pgid == -1) {
//                s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to create session id.");
//                return nullptr;
//            }
//
//            const string command = "wine64 " + binariesPath + "/win64/server.exe " + binariesPath + "/win64/" + modelIdentifier + ".dll";
//
//            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Starting server. Command: %s", command.c_str());
//
//            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
//
//            s_logger(s_componentEnvironment, instanceName, fmi2Error, "error", "Failed to start server.");
//
//            return nullptr;
//
//        } else {
//
//            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Server process id is %d.", pid);
//            s_pid = pid;
//
//        }
//
//    }
//#endif
//
//    ReturnValue r;
//
//    for (int attempts = 0;; attempts++) {
//        try {
//            s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Trying to connect...");
//            client = new rpc::client("localhost", rpc::constants::DEFAULT_PORT);
//            r = client->call("fmi2Instantiate", instanceName, (int)fmuType, fmuGUID ? fmuGUID : "", 
//                fmuResourceLocation ? fmuResourceLocation : "", visible, loggingOn).as<ReturnValue>();
//            break;
//        } catch (exception e) {
//            if (attempts < 20) {
//                s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Connection failed.");
//                delete client;
//                this_thread::sleep_for(chrono::milliseconds(500));  // wait for the server to start
//            } else {
//                s_logger(s_componentEnvironment, instanceName, fmi2Error, "info", e.what());
//                return nullptr;
//            }
//        }
//    }
//    
//    s_logger(s_componentEnvironment, instanceName, fmi2OK, "info", "Connected.");
//
//	forwardLogMessages(r.logMessages);
//	return fmi2Component(r.status);
//}
//
//void fmi2FreeInstance(fmi2Component c) {
//	client->call("fmi2FreeInstance");
//
//	//// Close process and thread handles
//	//CloseHandle(s_proccessInfo.hProcess);
//	//CloseHandle(s_proccessInfo.hThread);
//#ifdef _WIN32
//    if (s_proccessInfo.hProcess) {
//        cout << "Terminating server." << endl;
//        BOOL s = TerminateProcess(s_proccessInfo.hProcess, EXIT_SUCCESS);
//    }
//#else
//    if (s_pid != 0) {
//
//        s_logger(s_componentEnvironment, s_instanceName, fmi2OK, "info", "Terminating server (process group id %d).", s_pid);
//
//        killpg(s_pid, SIGKILL);
//
//        int status;
//        
//        while (wait(&status) > 0) {
//            s_logger(s_componentEnvironment, s_instanceName, fmi2OK, "info", "Waiting for child processes to terminate.");
//        }
//
//        s_logger(s_componentEnvironment, s_instanceName, fmi2OK, "info", "Server terminated.");
//    }
//#endif
//}
//
///* Enter and exit initialization mode, terminate and reset */
//fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
//	auto r = client->call("fmi2SetupExperiment", toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
//	auto r = client->call("fmi2EnterInitializationMode").as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
//	auto r = client->call("fmi2ExitInitializationMode").as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2Terminate(fmi2Component c) {
//	auto r = client->call("fmi2Terminate").as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2Reset(fmi2Component c) {
//	auto r = client->call("fmi2Reset").as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
///* Getting and setting variable values */
//fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
//	vector<unsigned int> v_vr(vr, vr + nvr);
//	auto r = client->call("fmi2GetReal", v_vr).as<RealReturnValue>();
//	copy(r.value.begin(), r.value.end(), value);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
//	vector<unsigned int> v_vr(vr, vr + nvr);
//	auto r = client->call("fmi2GetInteger", v_vr).as<IntegerReturnValue>();
//	copy(r.value.begin(), r.value.end(), value);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
//	vector<unsigned int> v_vr(vr, vr + nvr);
//	auto r = client->call("fmi2GetBoolean", v_vr).as<IntegerReturnValue>();
//	copy(r.value.begin(), r.value.end(), value);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String  value[]) {
//	NOT_IMPLEMENTED
//}
//
//fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
//	auto vr_ = static_cast<const unsigned int*>(vr);
//	vector<unsigned int> v_vr(vr_, vr_ + nvr);
//	vector<double> v_value(value, value + nvr);
//	auto r = client->call("fmi2SetReal", v_vr, v_value).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
//	auto vr_ = static_cast<const unsigned int*>(vr);
//	vector<unsigned int> v_vr(vr_, vr_ + nvr);
//	vector<int> v_value(value, value + nvr);
//	auto r = client->call("fmi2SetInteger", v_vr, v_value).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
//	auto vr_ = static_cast<const unsigned int*>(vr);
//	vector<unsigned int> v_vr(vr_, vr_ + nvr);
//	vector<int> v_value(value, value + nvr);
//	auto r = client->call("fmi2SetBoolean", v_vr, v_value).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String  value[]) {
//	NOT_IMPLEMENTED
//}
//
///* Getting and setting the internal FMU state */
//fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
//	NOT_IMPLEMENTED
//}
//
//fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate  FMUstate) {
//	NOT_IMPLEMENTED
//}
//
//fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
//	NOT_IMPLEMENTED
//}
//
//fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate  FMUstate, size_t* size) {
//	NOT_IMPLEMENTED
//}
//
//fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate  FMUstate, fmi2Byte[], size_t size) {
//	NOT_IMPLEMENTED
//}
//
//fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
//	NOT_IMPLEMENTED
//}
//
///* Getting partial derivatives */
//fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown, const fmi2ValueReference vKnown_ref[], size_t nKnown, const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
//	NOT_IMPLEMENTED
//}
//
///***************************************************
//Types for Functions for FMI2 for Model Exchange
//****************************************************/
//
///* Enter and exit the different modes */
//fmi2Status fmi2EnterEventMode(fmi2Component c) {
//	auto r = client->call("fmi2EnterEventMode").as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo* eventInfo) {
//	auto r = client->call("fmi2NewDiscreteStates").as<EventInfoReturnValue>();
//	eventInfo->newDiscreteStatesNeeded           = r.newDiscreteStatesNeeded;
//	eventInfo->terminateSimulation               = r.terminateSimulation;
//	eventInfo->nominalsOfContinuousStatesChanged = r.nominalsOfContinuousStatesChanged;
//	eventInfo->valuesOfContinuousStatesChanged   = r.valuesOfContinuousStatesChanged;
//	eventInfo->nextEventTimeDefined              = r.nextEventTimeDefined;
//	eventInfo->nextEventTime                     = r.nextEventTime;
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {
//	auto r = client->call("fmi2EnterContinuousTimeMode").as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2CompletedIntegratorStep(fmi2Component c,	fmi2Boolean  noSetFMUStatePriorToCurrentPoint, fmi2Boolean* enterEventMode, fmi2Boolean* terminateSimulation) {
//	auto r = client->call("fmi2CompletedIntegratorStep", noSetFMUStatePriorToCurrentPoint).as<IntegerReturnValue>();
//	*enterEventMode = r.value[0];
//	*terminateSimulation = r.value[1];
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
///* Providing independent variables and re-initialization of caching */
//fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {
//	auto r = client->call("fmi2SetTime", time).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx) {
//	vector<double> _x(x, x + nx);
//	auto r = client->call("fmi2SetContinuousStates", _x).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
///* Evaluation of the model equations */
//fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {
//	auto r = client->call("fmi2GetDerivatives", nx).as<RealReturnValue>();
//	copy(r.value.begin(), r.value.end(), derivatives);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {
//	auto r = client->call("fmi2GetEventIndicators", ni).as<RealReturnValue>();
//	copy(r.value.begin(), r.value.end(), eventIndicators);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real x[], size_t nx) {
//	auto r = client->call("fmi2GetContinuousStates", nx).as<RealReturnValue>();
//	copy(r.value.begin(), r.value.end(), x);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {
//	auto r = client->call("fmi2GetNominalsOfContinuousStates", nx).as<RealReturnValue>();
//	copy(r.value.begin(), r.value.end(), x_nominal);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
///***************************************************
//Types for Functions for FMI2 for Co-Simulation
//****************************************************/
//
///* Simulating the slave */
//fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], const fmi2Real value[]) {
//	auto vr_ = static_cast<const unsigned int*>(vr);
//	vector<unsigned int> v_vr(vr_, vr_ + nvr);
//	vector<int> v_order(order, order + nvr);
//	vector<double> v_value(value, value + nvr);
//	auto r = client->call("fmi2SetRealInputDerivatives", v_vr, v_order, v_value).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], fmi2Real value[]) {
//	vector<unsigned int> v_vr(vr, vr + nvr);
//	vector<int> v_order(order, order + nvr);
//	auto r = client->call("fmi2GetRealOutputDerivatives", v_vr, v_order).as<RealReturnValue>();
//	copy(r.value.begin(), r.value.end(), value);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
//	auto r = client->call("fmi2DoStep", double(currentCommunicationPoint), double(communicationStepSize), int(noSetFMUStatePriorToCurrentPoint)).as<ReturnValue>();
//	return handleReturnValue(r);
//}
//
//fmi2Status fmi2CancelStep(fmi2Component c) {
//    NOT_IMPLEMENTED
//}
//
///* Inquire slave status */
//fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value) {
//	auto r = client->call("fmi2GetStatus", int(s)).as<IntegerReturnValue>();
//	*value = fmi2Status(r.value[0]);
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value) {
//	auto r = client->call("fmi2GetRealStatus", int(s)).as<RealReturnValue>();
//	*value = r.value[0];
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value) {
//	auto r = client->call("fmi2GetIntegerStatus", int(s)).as<IntegerReturnValue>();
//	*value = r.value[0];
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value) {
//	auto r = client->call("fmi2GetBooleanStatus", int(s)).as<IntegerReturnValue>();
//	*value = r.value[0];
//	forwardLogMessages(r.logMessages);
//	return fmi2Status(r.status);
//}
//
//fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String*  value) {
//	NOT_IMPLEMENTED
//}
