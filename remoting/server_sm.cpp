#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")

#include <iostream>
#include <list>

using namespace std;

#include "FMI2.h"

#include "remoting_sm.h"


struct LogMessage {
    std::string instanceName;
    fmi2Status status;
    std::string category;
    std::string message;
};


FMIInstance *m_instance = NULL;

static std::list<LogMessage> s_logMessages;

void logMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
    cout << message << endl;
    s_logMessages.push_back({ instance->name, (fmi2Status)status, category, message });
}

static void logFunctionCall(FMIInstance *instance, FMIStatus status, const char *message, ...) {

    va_list args;
    va_start(args, message);

    vprintf(message, args);

    va_end(args);

    switch (status) {
    case FMIOK:
        printf(" -> OK\n");
        break;
    case FMIWarning:
        printf(" -> Warning\n");
        break;
    case FMIDiscard:
        printf(" -> Discard\n");
        break;
    case FMIError:
        printf(" -> Error\n");
        break;
    case FMIFatal:
        printf(" -> Fatal\n");
        break;
    case FMIPending:
        printf(" -> Pending\n");
        break;
    default:
        printf(" -> Unknown status (%d)\n", status);
        break;
    }
}



#define BUF_SIZE (1024*16)
TCHAR szName[] = TEXT("MyFileMappingObject");


#define ARG(T,IDX) (T)&pBuf[1024 * IDX]


int main(int argc, char *argv[]) {

    cout << "Server started" << endl;

    HANDLE inputReady = INVALID_HANDLE_VALUE;
    HANDLE outputReady = INVALID_HANDLE_VALUE;

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

    bool receive = true;

    while (receive) {

        //cout << "Waiting for inputReady... ";
        DWORD dwEvent = WaitForSingleObject(inputReady, INFINITE);
        //cout << dwEvent << endl;

        rpcFunction rpc = *((rpcFunction *)&pBuf[1024 * 0]);

        switch (rpc) {
        case rpc_fmi2Instantiate: {

            const char *libraryPath = argv[1];

            fmi2String  instanceName = (fmi2String)&pBuf[1024 * 1];
            fmi2Type    fmuType = *((fmi2Type *)&pBuf[1024 * 2]);
            fmi2String  fmuGUID = (fmi2String)&pBuf[1024 * 3];
            fmi2String  fmuResourceLocation = (fmi2String)&pBuf[1024 * 4];
            fmi2Boolean visible = *((fmi2Boolean *)&pBuf[1024 * 5]);
            fmi2Boolean loggingOn = *((fmi2Boolean *)&pBuf[1024 * 6]);

            m_instance = FMICreateInstance(instanceName, libraryPath, logMessage, NULL /*logFunctionCall*/);

            fmi2Status status = FMI2Instantiate(m_instance, fmuResourceLocation, fmuType, fmuGUID, visible, loggingOn);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2Terminate: {
            fmi2Status status = FMI2Terminate(m_instance);
            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));
            break;
        }
        case rpc_fmi2FreeInstance: {
            FMI2FreeInstance(m_instance);
            fmi2Status status = fmi2OK;
            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));
            receive = false;
            break;
        }
        case rpc_fmi2SetupExperiment: {

            fmi2Boolean toleranceDefined = *((fmi2Boolean *)&pBuf[1024 * 1]);
            fmi2Real tolerance           = *((fmi2Real    *)&pBuf[1024 * 2]);
            fmi2Real startTime           = *((fmi2Real    *)&pBuf[1024 * 3]);
            fmi2Boolean stopTimeDefined  = *((fmi2Boolean *)&pBuf[1024 * 4]);
            fmi2Real stopTime            = *((fmi2Real    *)&pBuf[1024 * 5]);
                                   
            fmi2Status status = FMI2SetupExperiment(m_instance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
                     
            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2EnterInitializationMode: {
            fmi2Status status = FMI2EnterInitializationMode(m_instance);
            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));
            break;
        }
        case rpc_fmi2ExitInitializationMode: {
            fmi2Status status = FMI2ExitInitializationMode(m_instance);
            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));
            break;
        }
        case rpc_fmi2GetReal: {

            fmi2ValueReference *vr =   (fmi2ValueReference*) &pBuf[1024 * 1];
            size_t nvr             = *((size_t*)             &pBuf[1024 * 2]);
            fmi2Real *value        =   (fmi2Real*)           &pBuf[1024 * 3];

            fmi2Status status = FMI2GetReal(m_instance, vr, nvr, value);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break; 
        }
        case rpc_fmi2GetInteger: {

            fmi2ValueReference *vr = (fmi2ValueReference*)&pBuf[1024 * 1];
            size_t nvr = *((size_t*)&pBuf[1024 * 2]);
            fmi2Integer *value = (fmi2Integer*)&pBuf[1024 * 3];

            fmi2Status status = FMI2GetInteger(m_instance, vr, nvr, value);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2GetBoolean: {

            fmi2ValueReference *vr = (fmi2ValueReference*)&pBuf[1024 * 1];
            size_t nvr = *((size_t*)&pBuf[1024 * 2]);
            fmi2Boolean *value = (fmi2Boolean*)&pBuf[1024 * 3];

            fmi2Status status = FMI2GetBoolean(m_instance, vr, nvr, value);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2SetReal: {

            fmi2ValueReference *vr = (fmi2ValueReference*)&pBuf[1024 * 1];
            size_t nvr = *((size_t*)&pBuf[1024 * 2]);
            fmi2Real *value = (fmi2Real*)&pBuf[1024 * 3];

            fmi2Status status = FMI2SetReal(m_instance, vr, nvr, value);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2SetInteger: {

            fmi2ValueReference *vr = (fmi2ValueReference*)&pBuf[1024 * 1];
            size_t nvr = *((size_t*)&pBuf[1024 * 2]);
            fmi2Integer *value = (fmi2Integer*)&pBuf[1024 * 3];

            fmi2Status status = FMI2SetInteger(m_instance, vr, nvr, value);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2SetBoolean: {

            fmi2ValueReference *vr = (fmi2ValueReference*)&pBuf[1024 * 1];
            size_t nvr = *((size_t*)&pBuf[1024 * 2]);
            fmi2Boolean *value = (fmi2Boolean*)&pBuf[1024 * 3];

            fmi2Status status = FMI2SetBoolean(m_instance, vr, nvr, value);

            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2DoStep: {
                                               
            fmi2Real    currentCommunicationPoint        = *((fmi2Real    *)&pBuf[1024 * 1]);
            fmi2Real    communicationStepSize            = *((fmi2Real    *)&pBuf[1024 * 2]);
            fmi2Boolean noSetFMUStatePriorToCurrentPoint = *((fmi2Boolean *)&pBuf[1024 * 3]);
                                   
            fmi2Status status = FMI2DoStep(m_instance, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
                                   
            memcpy(&pBuf[1024 * 10], &status, sizeof(fmi2Status));
                                   
            break;
        }
        default: {
            cout << "Unknown RPC: " << rpc << endl;
            receive = false;
            break;
        }
        }

        //cout << "SetEvent(outputReady)" << endl;
        if (!SetEvent(outputReady)) {
            printf("SetEvent failed (%d)\n", GetLastError());
            exit(1);
        }

    }

    UnmapViewOfFile(pBuf);

    CloseHandle(hMapFile);

    return 0;
}
