#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")

#include <iostream>
#include <list>

using namespace std;

#include "FMI2.h"

typedef enum {
    rpc_fmi2GetTypesPlatform,
    rpc_fmi2GetVersion,
    rpc_fmi2Instantiate,
    rpc_fmi2SetupExperiment,
    rpc_fmi2EnterInitializationMode,
    rpc_fmi2ExitInitializationMode,
    rpc_fmi2GetReal,
    rpc_fmi2DoStep,
    rpc_fmi2Terminate,
    rpc_fmi2FreeInstance,
} rpcFunction;

struct LogMessage {
    std::string instanceName;
    fmi2Status status;
    std::string category;
    std::string message;
};


FMIInstance *m_instance = NULL;

static std::list<LogMessage> s_logMessages;

void logMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
    s_logMessages.push_back({ instance->name, (fmi2Status)status, category, message });
}



#define BUF_SIZE (1024*8)
TCHAR szName[] = TEXT("MyFileMappingObject");


#define ARG(T,IDX) (T)&pBuf[1024 * IDX]


int main(int argc, char *argv[]) {

    HANDLE inputMutex = INVALID_HANDLE_VALUE;
    HANDLE outputMutex = INVALID_HANDLE_VALUE;

    inputMutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        "InputMutex");        // named mutex

    if (inputMutex == NULL)
    {
        printf("CreateMutex InputMutex error: %d\n", GetLastError());
        return 1;
    }

    outputMutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        "OutputMutex");        // named mutex

    if (outputMutex == NULL)
    {
        printf("CreateMutex OutputMutex error: %d\n", GetLastError());
        return 1;
    }

    HANDLE hMapFile;
    LPTSTR pBuf;

    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        szName);               // name of mapping object

    if (hMapFile == NULL)
    {
        _tprintf(TEXT("Could not open file mapping object (%d).\n"),
            GetLastError());
        return 1;
    }

    pBuf = (LPTSTR)MapViewOfFile(hMapFile, // handle to map object
        FILE_MAP_ALL_ACCESS,  // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBuf == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());

        CloseHandle(hMapFile);

        return 1;
    }

    cout << "Waiting for outputMutex... ";

    DWORD outputWaitResult = WaitForSingleObject(
        outputMutex, // handle to mutex
        INFINITE);   // no time-out interval

    cout << "OK" << endl;

    cout << "Waiting for inputMutex... ";

    DWORD inputWaitResult = WaitForSingleObject(
        inputMutex, // handle to mutex
        INFINITE);  // no time-out interval

    const char *libraryPath = argv[1]; // "C:\\Users\\tsr2\\Downloads\\BouncingBall\\binaries\\win64\\BouncingBall.dll";

    rpcFunction rpc                 = *((rpcFunction *) &pBuf[1024 * 0]);
    fmi2String  instanceName        = (fmi2String)      &pBuf[1024 * 1];
    fmi2Type    fmuType             = *((fmi2Type *)    &pBuf[1024 * 2]);
    fmi2String  fmuGUID             = (fmi2String)      &pBuf[1024 * 3];
    fmi2String  fmuResourceLocation = (fmi2String)      &pBuf[1024 * 4];
    fmi2Boolean visible             = *((fmi2Boolean *) &pBuf[1024 * 5]);
    fmi2Boolean loggingOn           = *((fmi2Boolean *) &pBuf[1024 * 6]);

    m_instance = FMICreateInstance(instanceName, libraryPath, logMessage, nullptr);

    fmi2Status status = FMI2Instantiate(m_instance, fmuResourceLocation, fmuType, fmuGUID, visible, loggingOn);

    memcpy(&pBuf[1024 * 7], &status, sizeof(fmi2Status));

    cout << "OK" << endl;

    MessageBox(NULL, pBuf, TEXT("Process2"), MB_OK);

    ReleaseMutex(inputMutex);
    ReleaseMutex(outputMutex);

    UnmapViewOfFile(pBuf);

    CloseHandle(hMapFile);

    return 0;
}

//#include <windows.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdint.h>
//
//#include <list>
//#include <string>
//#include <iostream>
//
//#include <msgpack.h>
//
//extern "C" {
//#include "FMI2.h"
//}
//
//#define BUFSIZE 4096
//
//typedef enum {
//    rpc_fmi2GetTypesPlatform,
//    rpc_fmi2GetVersion,
//    rpc_fmi2Instantiate,
//    rpc_fmi2SetupExperiment,
//    rpc_fmi2EnterInitializationMode,
//    rpc_fmi2ExitInitializationMode,
//    rpc_fmi2GetReal,
//    rpc_fmi2DoStep,
//    rpc_fmi2Terminate,
//    rpc_fmi2FreeInstance,
//} rpcFunction;
//
//struct LogMessage {
//    std::string instanceName;
//    fmi2Status status;
//    std::string category;
//    std::string message;
//};
//
//
//static char s_response[1024];
//static size_t s_responseSize = 1024;
//
//FMIInstance *m_instance = NULL;
//
//static std::list<LogMessage> s_logMessages;
//
//void logMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
//    s_logMessages.push_back({ instance->name, (fmi2Status)status, category, message });
//}
//
//void packLogMessages(msgpack_packer &pk, msgpack_sbuffer &sbuf) {
//
//    msgpack_pack_array(&pk, s_logMessages.size());
//
//    for (auto it = s_logMessages.begin(); it != s_logMessages.end(); it++) {
//        
//        msgpack_pack_array(&pk, 4);
//
//        msgpack_pack_str_with_body(&pk, it->instanceName.c_str(), it->instanceName.size() + 1);
//        msgpack_pack_int(&pk, it->status);
//        msgpack_pack_str_with_body(&pk, it->category.c_str(), it->category.size() + 1);
//        msgpack_pack_str_with_body(&pk, it->message.c_str(), it->message.size() + 1);
//    }
//
//    s_logMessages.clear();
//}
//
//
//void handleGetTypesPlatform(uint32_t *length, char **message) {
//    char *platform = "dummy2";
//    *length = strlen(platform);
//    *message = platform;
//}
//
//
//
//int main(int argc, char *argv[]) {
//
//	if (argc != 2) {
//        std::cerr << "Usage: server <path_to_fmu>" << std::endl;
//		return EXIT_FAILURE;
//	}
//
//    //CHAR chBuf[BUFSIZE];
//    DWORD dwRead, dwWritten;
//    HANDLE hStdin, hStdout;
//    BOOL bSuccess;
//
//    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
//    hStdin = GetStdHandle(STD_INPUT_HANDLE);
//    
//    if ((hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE)) {
//        ExitProcess(1);
//    }
//
//    for (;;) {
//
//        msgpack_sbuffer sbuf;
//        msgpack_sbuffer_init(&sbuf);
//
//        size_t size;
//
//        bSuccess = ReadFile(hStdin, &size, sizeof(size_t), &dwRead, NULL);
//        if (!bSuccess) break;
//
//        char *data = (char *)malloc(size);
//
//        bSuccess = ReadFile(hStdin, data, size, &dwRead, NULL);
//        if (!bSuccess) break;
//
//        msgpack_zone mempool;
//        msgpack_zone_init(&mempool, 2048);
//
//        msgpack_object deserialized;
//        msgpack_unpack(data, size, NULL, &mempool, &deserialized);
//
//        auto& array = deserialized.via.array;
//
//        msgpack_packer pk;
//        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
//
//        switch (array.ptr[0].via.i64) {
//        case rpc_fmi2GetTypesPlatform:
//            msgpack_pack_str(&pk, sizeof(fmi2TypesPlatform));
//            msgpack_pack_str_body(&pk, fmi2TypesPlatform, sizeof(fmi2TypesPlatform));
//            break;
//        case rpc_fmi2GetVersion:
//            msgpack_pack_str(&pk, sizeof(fmi2Version));
//            msgpack_pack_str_body(&pk, fmi2Version, sizeof(fmi2Version));
//            break;
//        case rpc_fmi2Instantiate: {
//            const char *instanceName = array.ptr[1].via.str.ptr;
//            const char *libraryPath = argv[1]; // "C:\\Users\\tsr2\\Downloads\\BouncingBall\\binaries\\win64\\BouncingBall.dll";
//            fmi2Type fmuType = (fmi2Type)array.ptr[2].via.i64;
//            const char *fmuGUID = array.ptr[3].via.str.ptr;
//            const char *fmuResourceLocation = array.ptr[4].via.str.ptr;
//            fmi2Boolean visible = (fmi2Boolean)array.ptr[5].via.i64;
//            fmi2Boolean loggingOn = (fmi2Boolean)array.ptr[6].via.i64;
//
//            m_instance = FMICreateInstance(instanceName, libraryPath, logMessage, nullptr);
//
//            fmi2Status status = FMI2Instantiate(m_instance, fmuResourceLocation, fmuType, fmuGUID, visible, loggingOn);
//
//            //logMessage(m_instance, FMIOK, "test", "message");
//            //logMessage(m_instance, FMIWarning, "test2", "message2");
//
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_fix_uint64(&pk, reinterpret_cast<uint64_t>(m_instance));
//
//            packLogMessages(pk, sbuf);
//            break; }
//        case rpc_fmi2Terminate: {
//            fmi2Status status = FMI2Terminate(m_instance);
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, status);
//            packLogMessages(pk, sbuf);
//            break; }
//        case rpc_fmi2FreeInstance: {
//            FMI2FreeInstance(m_instance);
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, fmi2OK);
//            packLogMessages(pk, sbuf);
//            break; }
//        case rpc_fmi2SetupExperiment: {
//            fmi2Boolean toleranceDefined = (fmi2Boolean)array.ptr[1].via.i64;
//            fmi2Real tolerance = (fmi2Real)array.ptr[2].via.f64;
//            fmi2Real startTime = (fmi2Real)array.ptr[3].via.f64;
//            fmi2Boolean stopTimeDefined = (fmi2Boolean)array.ptr[4].via.i64;
//            fmi2Real stopTime = (fmi2Real)array.ptr[5].via.f64;
//
//            fmi2Status status = FMI2SetupExperiment(m_instance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
//
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, status);
//            packLogMessages(pk, sbuf);
//
//            break; }
//        case rpc_fmi2EnterInitializationMode: {
//            fmi2Status status = FMI2EnterInitializationMode(m_instance);
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, status);
//            packLogMessages(pk, sbuf);
//            break; }
//        case rpc_fmi2ExitInitializationMode: {
//            fmi2Status status = FMI2ExitInitializationMode(m_instance);
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, status);
//            packLogMessages(pk, sbuf);
//            break; }
//        case rpc_fmi2GetReal: {
//            auto vrs = array.ptr[1].via.array;
//            size_t nvr = vrs.size;
//            fmi2ValueReference *vr = (fmi2ValueReference *)calloc(nvr, sizeof(fmi2ValueReference));
//            fmi2Real *value = (fmi2Real *)calloc(nvr, sizeof(fmi2Real));
//            for (int i = 0; i < nvr; i++) {
//                vr[i] = vrs.ptr[i].via.u64;
//            }
//            fmi2Status status = FMI2GetReal(m_instance, vr, nvr, value);
//            msgpack_pack_array(&pk, 3);
//            msgpack_pack_int(&pk, status);
//            packLogMessages(pk, sbuf);
//            msgpack_pack_array(&pk, nvr);
//            for (int i = 0; i < nvr; i++) {
//                msgpack_pack_double(&pk, value[i]);
//            }
//            break; }
//
//        case rpc_fmi2DoStep: {
//            
//            fmi2Real currentCommunicationPoint           = array.ptr[1].via.f64;
//            fmi2Real communicationStepSize               = array.ptr[2].via.f64;
//            fmi2Boolean noSetFMUStatePriorToCurrentPoint = (fmi2Boolean)array.ptr[3].via.i64;
//
//            fmi2Status status = FMI2DoStep(m_instance, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
//
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, status);
//            packLogMessages(pk, sbuf);
//
//            break;
//        }
//        default:
//            msgpack_pack_array(&pk, 2);
//            msgpack_pack_int(&pk, fmi2Error);
//            msgpack_pack_str_with_body(&pk, "Unknown RPC", strlen("Unknown RPC") + 1);
//            break;
//        }
//
//        bSuccess = WriteFile(hStdout, &sbuf.size, sizeof(size_t), &dwWritten, NULL);
//        if (!bSuccess) break;
//
//        bSuccess = WriteFile(hStdout, sbuf.data, sbuf.size, &dwWritten, NULL);
//        if (!bSuccess) break;
//    }
//
//    // TODO: clean up
//
//    return 0;
//}
//
////#ifdef _WIN32
////#include <Windows.h>
////#else
////#include <pthread.h>
////#include <unistd.h>
////#define MAX_PATH 2048
////#endif
////
////#include <time.h>
////#include <list>
////#include <iostream>
////#include <stdexcept>
////
////#include "rpc/server.h"
////
////#include "remoting.h"
////
////extern "C" {
////#include "FMI2.h"
////}
////
////using namespace std;
////
////#define NOT_IMPLEMENTED return static_cast<int>(fmi2Error);
////
////static list<LogMessage> s_logMessages;
////
////static rpc::server *s_server = nullptr;
////
////time_t s_lastActive;
////
////void logMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
////	s_logMessages.push_back({instance->name, status, category, message});
////}
////
////static void resetExitTimer() {
////	time(&s_lastActive);
////}
////
////#ifdef _WIN32
////DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
////#else
////void *doSomeThing(void *arg) {
////#endif
////    
////    while (s_server) {
////
////		time_t currentTime;
////		time(&currentTime);
////
////		if (difftime(currentTime, s_lastActive) > 10) {
////			cout << "Client inactive for more than 10 seconds. Exiting." << endl;
////			s_server->stop();
////			return 0;
////		}
////
////#ifdef _WIN32
////        Sleep(500);
////#else
////        usleep(500000);
////#endif		
////	}
////
////	return 0;
////}
////
////class FMU {
////
////private:
////
////    string libraryPath;
////
////#ifdef _WIN32
////	HMODULE libraryHandle = nullptr;
////#else
////    void *libraryHandle = nullptr;
////#endif
////
////	ReturnValue createReturnValue(int status) {
////		ReturnValue r = { status, s_logMessages };
////		s_logMessages.clear();
////		return r;
////	}
////
////	RealReturnValue createRealReturnValue(int status, const vector<double> &value) {
////		RealReturnValue r = { status, s_logMessages, value };
////		s_logMessages.clear();
////		return r;
////	}
////
////	IntegerReturnValue createIntegerReturnValue(int status, const vector<int> &value) {
////		IntegerReturnValue r = { status, s_logMessages, value };
////		s_logMessages.clear();
////		return r;
////	}
////
////	EventInfoReturnValue createEventInfoReturnValue(int status, const fmi2EventInfo *eventInfo) {
////		EventInfoReturnValue r = {
////			status,
////			s_logMessages,
////			eventInfo->newDiscreteStatesNeeded, 
////			eventInfo->terminateSimulation,
////			eventInfo->nominalsOfContinuousStatesChanged,
////			eventInfo->valuesOfContinuousStatesChanged,
////			eventInfo->nextEventTimeDefined,
////			eventInfo->nextEventTime,
////		};
////		s_logMessages.clear();
////		return r;
////	}
////
////public:
////	rpc::server srv;
////
////    FMIInstance *m_instance;
////
////	FMU(const string &libraryPath) : srv(rpc::constants::DEFAULT_PORT) {
////
////        this->libraryPath = libraryPath;
////		
////		srv.bind("echo", [](string const& s) {
////			return s;
////		});
////
////        srv.bind("sum", [this](double a, double b) {
////            return a + b;
////        });
////
////		srv.bind("fmi2SetDebugLogging", [this]() { 
////            NOT_IMPLEMENTED
////        });
////
////		/* Creation and destruction of FMU instances and setting debug status */
////		srv.bind("fmi2Instantiate", [this](string const& instanceName, int fmuType, string const& fmuGUID, string const& fmuResourceLocation, int visible, int loggingOn) {
////
////            m_instance = FMICreateInstance(instanceName.c_str(), this->libraryPath.c_str(), logMessage, nullptr);
////
////            if (!m_instance) {
////                return createReturnValue(0);
////            }
////
////			resetExitTimer();
////
////            fmi2Status status = FMI2Instantiate(m_instance, fmuResourceLocation.c_str(), static_cast<fmi2Type>(fmuType), fmuGUID.c_str(), visible, loggingOn);
////
////            if (status > fmi2Warning) {
////                return createReturnValue(0);
////            }
////			
////            long int_value = reinterpret_cast<long>(m_instance);
////            
////            return createReturnValue(static_cast<int>(int_value));
////		});
////
////		srv.bind("fmi2FreeInstance", [this]() { 
////			resetExitTimer();
////			FMI2FreeInstance(m_instance);
////		});
////
////		/* Enter and exit initialization mode, terminate and reset */
////		srv.bind("fmi2SetupExperiment", [this](int toleranceDefined, double tolerance, double startTime, int stopTimeDefined, double stopTime) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetupExperiment(m_instance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
////			return createReturnValue(status);
////		});
////		
////		srv.bind("fmi2EnterInitializationMode", [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2EnterInitializationMode(m_instance);
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2ExitInitializationMode",  [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2ExitInitializationMode(m_instance);
////			return createReturnValue(status);
////		});
////		
////		srv.bind("fmi2Terminate", [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2Terminate(m_instance);
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2Reset", [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2Reset(m_instance);
////			return createReturnValue(status);
////		});
////
////		/* Getting and setting variable values */
////		srv.bind("fmi2GetReal", [this](const vector<unsigned int> &vr) {
////			resetExitTimer();
////			vector<double> value(vr.size());
////			const fmi2Status status = FMI2GetReal(m_instance, vr.data(), vr.size(), value.data());
////            return createRealReturnValue(status, value);
////		});
////
////		srv.bind("fmi2GetInteger", [this](const vector<unsigned int> &vr) {
////			resetExitTimer();
////			vector<int> value(vr.size());
////			const fmi2Status status = FMI2GetInteger(m_instance, vr.data(), vr.size(), value.data());
////			return createIntegerReturnValue(status, value);
////		});
////
////		srv.bind("fmi2GetBoolean", [this](const vector<unsigned int> &vr) {
////			resetExitTimer();
////			vector<int> value(vr.size());
////			const fmi2Status status = FMI2GetBoolean(m_instance, vr.data(), vr.size(), value.data());
////			return createIntegerReturnValue(status, value);
////		});
////
////		srv.bind("fmi2SetReal", [this](const vector<unsigned int> &vr, const vector<double> &value) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetReal(m_instance, vr.data(), vr.size(), value.data());
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2SetInteger", [this](const vector<unsigned int> &vr, const vector<int> &value) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetInteger(m_instance, vr.data(), vr.size(), value.data());
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2SetBoolean", [this](const vector<unsigned int> &vr, const vector<int> &value) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetBoolean(m_instance, vr.data(), vr.size(), value.data());
////			return createReturnValue(status);
////		});
////
////		/* Getting and setting the internal FMU state */
////		// fmi2GetFMUstateTYPE *m_fmi2Component c, fmi2FMUstate* FMUstate);
////		// fmi2SetFMUstateTYPE *m_fmi2Component c, fmi2FMUstate  FMUstate);
////		// fmi2FreeFMUstateTYPE *m_fmi2Component c, fmi2FMUstate* FMUstate);
////		// fmi2SerializedFMUstateSizeTYPE *m_fmi2Component c, fmi2FMUstate  FMUstate, size_t* size);
////		// fmi2SerializeFMUstateTYPE *m_fmi2Component c, fmi2FMUstate  FMUstate, fmi2Byte[], size_t size);
////		// fmi2DeSerializeFMUstateTYPE *m_fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate);
////
////		srv.bind("fmi2GetDirectionalDerivative", [this](const vector<unsigned int> &vUnknown_ref, const vector<unsigned int> &vKnown_ref, const vector<double> &dvKnown) {
////			resetExitTimer();
////			vector<double> dvUnknown(vKnown_ref.size());
////			const fmi2Status status = FMI2GetDirectionalDerivative(m_instance, vUnknown_ref.data(), vUnknown_ref.size(),
////				vKnown_ref.data(), vKnown_ref.size(), dvKnown.data(), dvUnknown.data());
////			return createRealReturnValue(status, dvUnknown);
////		});
////
////		/***************************************************
////		Types for Functions for FMI2 for Model Exchange
////		****************************************************/
////
////		/* Enter and exit the different modes */
////		srv.bind("fmi2EnterEventMode", [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2EnterEventMode(m_instance);
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2NewDiscreteStates", [this]() {
////			resetExitTimer();
////			fmi2EventInfo eventInfo = { 0 };
////			const fmi2Status status = FMI2NewDiscreteStates(m_instance, &eventInfo);
////			return createEventInfoReturnValue(status, &eventInfo);
////		});
////
////		srv.bind("fmi2EnterContinuousTimeMode", [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2EnterContinuousTimeMode(m_instance);
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2CompletedIntegratorStep", [this](int noSetFMUStatePriorToCurrentPoint) {
////			resetExitTimer();
////			vector<int> value(2);
////			fmi2Boolean* enterEventMode = &(value.data()[0]);
////			fmi2Boolean* terminateSimulation = &(value.data()[1]);
////			const fmi2Status status = FMI2CompletedIntegratorStep(m_instance, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);
////			return createIntegerReturnValue(status, value);
////		});
////
////		/* Providing independent variables and re-initialization of caching */
////		srv.bind("fmi2SetTime", [this](double time) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetTime(m_instance, time);
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2SetContinuousStates", [this](const vector<double> &x) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetContinuousStates(m_instance, x.data(), x.size());
////			return createReturnValue(status);
////		});
////
////		/* Evaluation of the model equations */
////		srv.bind("fmi2GetDerivatives", [this](size_t nx) {
////			resetExitTimer();
////			vector<double> derivatives(nx);
////			const fmi2Status status = FMI2GetDerivatives(m_instance, derivatives.data(), nx);
////			return createRealReturnValue(status, derivatives);
////		});
////		
////		srv.bind("fmi2GetEventIndicators", [this](size_t ni) {
////			resetExitTimer();
////			vector<double> eventIndicators(ni);
////			const fmi2Status status = FMI2GetEventIndicators(m_instance, eventIndicators.data(), ni);
////			return createRealReturnValue(status, eventIndicators);
////		});
////
////		srv.bind("fmi2GetContinuousStates", [this](size_t nx) {
////			resetExitTimer();
////			vector<double> x(nx);
////			const fmi2Status status = FMI2GetContinuousStates(m_instance, x.data(), nx);
////			return createRealReturnValue(status, x);
////		});
////
////		srv.bind("fmi2GetNominalsOfContinuousStates", [this](size_t nx) {
////			resetExitTimer();
////			vector<double> x_nominal(nx);
////			const fmi2Status status = FMI2GetNominalsOfContinuousStates(m_instance, x_nominal.data(), nx);
////			return createRealReturnValue(status, x_nominal);
////		});
////
////		/***************************************************
////		Types for Functions for FMI2 for Co-Simulation
////		****************************************************/
////
////		/* Simulating the slave */
////		srv.bind("fmi2SetRealInputDerivatives", [this](const vector<unsigned int> &vr, const vector<int> &order, const vector<double> &value) {
////			resetExitTimer();
////			const fmi2Status status = FMI2SetRealInputDerivatives(m_instance, vr.data(), vr.size(), order.data(), value.data());
////			return createReturnValue(status);
////		});
////
////		srv.bind("fmi2GetRealOutputDerivatives", [this](const vector<unsigned int> &vr, const vector<int> &order) {
////			resetExitTimer();
////			vector<double> value(vr.size());
////			const fmi2Status status = FMI2GetRealOutputDerivatives(m_instance, vr.data(), vr.size(), order.data(), value.data());
////			return createRealReturnValue(status, value);
////		});
////
////		srv.bind("fmi2DoStep", [this](double currentCommunicationPoint, double communicationStepSize, int noSetFMUStatePriorToCurrentPoint) {
////			resetExitTimer();
////			const fmi2Status status = FMI2DoStep(m_instance, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
////			return createReturnValue(status);
////		});
////		
////		srv.bind("fmi2CancelStep", [this]() {
////			resetExitTimer();
////			const fmi2Status status = FMI2CancelStep(m_instance);
////			return createReturnValue(status);
////		});
////
////		/* Inquire slave status */
////		srv.bind("fmi2GetStatus", [this](int s) {
////			resetExitTimer();
////			vector<int> value(1);
////			const fmi2Status status = FMI2GetStatus(m_instance, fmi2StatusKind(s), reinterpret_cast<fmi2Status *>(value.data()));
////			return createIntegerReturnValue(status, value);
////		});
////
////		srv.bind("fmi2GetRealStatus", [this](int s) {
////			resetExitTimer();
////			vector<double> value(1);
////			const fmi2Status status = FMI2GetRealStatus(m_instance, fmi2StatusKind(s), value.data());
////			return createRealReturnValue(status, value);
////		});
////
////		srv.bind("fmi2GetIntegerStatus", [this](int s) {
////			resetExitTimer();
////			vector<int> value(1);
////			const fmi2Status status = FMI2GetIntegerStatus(m_instance, fmi2StatusKind(s), value.data());
////			return createIntegerReturnValue(status, value);
////		});
////
////		srv.bind("fmi2GetBooleanStatus", [this](int s) {
////			resetExitTimer();
////			vector<int> value(1);
////			const fmi2Status status = FMI2GetBooleanStatus(m_instance, fmi2StatusKind(s), value.data());
////			return createIntegerReturnValue(status, value);
////		});
////
////		//fmi2GetStringStatusTYPE  *m_fmi2GetStringStatus;
////
////	}
////
////};
////
////
////int main(int argc, char *argv[]) {
////
////	if (argc != 2) {
////        cerr << "Usage: server <path_to_fmu>" << endl;
////		return EXIT_FAILURE;
////	}
////
////    try {
////
////        cout << "Loading " << argv[1] << endl;
////
////	    FMU fmu(argv[1]);
////
////	    s_server = &fmu.srv;
////	    time(&s_lastActive);
////
////#ifdef _WIN32
////	    DWORD dwThreadIdArray;
////
////	    HANDLE hThreadArray = CreateThread(
////	    	NULL,                   // default security attributes
////	    	0,                      // use default stack size  
////	    	MyThreadFunction,       // thread function name
////	    	NULL,                   // argument to thread function 
////	    	0,                      // use default creation flags 
////	    	&dwThreadIdArray);      // returns the thread identifier
////#else
////        pthread_t tid;
////        int err = pthread_create(&tid, NULL, &doSomeThing, NULL);
////        if (err != 0)
////            printf("Can't create thread :[%s]", strerror(err));
////        else
////            printf("Thread created successfully\n");
////#endif
////
////        cout << "Starting RPC server" << endl;
////
////	    fmu.srv.run();
////
////    } catch (const std::exception& e) {
////        cerr << e.what() << endl;
////        return EXIT_FAILURE;
////    }
////
////	return EXIT_SUCCESS;
////}
