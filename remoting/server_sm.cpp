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

int main(int argc, char *argv[]) {

    if (argc != 2) {
        cout << "Usage: server_sm <library_path>" << endl;
        return 1;
    }

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

        rpcFunction rpc = *((rpcFunction *)ARG(0));

        switch (rpc) {
        case rpc_fmi2Instantiate: {

            const char *libraryPath = argv[1];

            fmi2String  instanceName = (fmi2String)ARG(1);
            fmi2Type    fmuType = *((fmi2Type *)ARG(2));
            fmi2String  fmuGUID = (fmi2String)ARG(3);
            fmi2String  fmuResourceLocation = (fmi2String)ARG(4);
            fmi2Boolean visible = *((fmi2Boolean *)ARG(5));
            fmi2Boolean loggingOn = *((fmi2Boolean *)ARG(6));

            m_instance = FMICreateInstance(instanceName, libraryPath, logMessage, logFunctionCall);

            fmi2Status status = FMI2Instantiate(m_instance, fmuResourceLocation, fmuType, fmuGUID, visible, loggingOn);

            memcpy(ARG(10), &status, sizeof(fmi2Status));

            break;
        }
        case rpc_fmi2Terminate: {
            STATUS = FMI2Terminate(m_instance);
            break;
        }
        case rpc_fmi2FreeInstance: {
            FMI2FreeInstance(m_instance);
            STATUS = fmi2OK;
            receive = false;
            break;
        }
        case rpc_fmi2SetupExperiment: {

            fmi2Boolean toleranceDefined = *((fmi2Boolean *)ARG(1));
            fmi2Real tolerance           = *((fmi2Real    *)ARG(2));
            fmi2Real startTime           = *((fmi2Real    *)ARG(3));
            fmi2Boolean stopTimeDefined  = *((fmi2Boolean *)ARG(4));
            fmi2Real stopTime            = *((fmi2Real    *)ARG(5));
                                   
            STATUS = FMI2SetupExperiment(m_instance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
            
            break;
        }
        case rpc_fmi2EnterInitializationMode: {
            STATUS = FMI2EnterInitializationMode(m_instance);
            break;
        }
        case rpc_fmi2ExitInitializationMode: {
            STATUS = FMI2ExitInitializationMode(m_instance);
            break;
        }
        case rpc_fmi2GetReal: {

            fmi2ValueReference *vr =   (fmi2ValueReference*) ARG(1);
            size_t nvr             = *((size_t*)             ARG(2));
            fmi2Real *value        =   (fmi2Real*)           ARG(3);

            STATUS = FMI2GetReal(m_instance, vr, nvr, value);

            break; 
        }
        case rpc_fmi2GetInteger: {

            fmi2ValueReference *vr =   (fmi2ValueReference*)ARG(1);
            size_t             nvr = *((size_t*)            ARG(2));
            fmi2Integer     *value =   (fmi2Integer*)       ARG(3);

            STATUS = FMI2GetInteger(m_instance, vr, nvr, value);

            break;
        }
        case rpc_fmi2GetBoolean: {

            fmi2ValueReference *vr = (fmi2ValueReference*)ARG(1);
            size_t             nvr = *((size_t*)ARG(2));
            fmi2Boolean     *value = (fmi2Boolean*)ARG(3);

            STATUS = FMI2GetBoolean(m_instance, vr, nvr, value);

            break;
        }
        case rpc_fmi2SetReal: {

            fmi2ValueReference *vr = (fmi2ValueReference*)ARG(1);
            size_t nvr = *((size_t*)ARG(2));
            fmi2Real *value = (fmi2Real*)ARG(3);

            STATUS = FMI2SetReal(m_instance, vr, nvr, value);

            break;
        }
        case rpc_fmi2SetInteger: {

            fmi2ValueReference *vr = (fmi2ValueReference*)ARG(1);
            size_t nvr = *((size_t*)ARG(2));
            fmi2Integer *value = (fmi2Integer*)ARG(3);

            STATUS = FMI2SetInteger(m_instance, vr, nvr, value);

            break;
        }
        case rpc_fmi2SetBoolean: {

            fmi2ValueReference *vr = (fmi2ValueReference*)ARG(1);
            size_t nvr = *((size_t*)ARG(2));
            fmi2Boolean *value = (fmi2Boolean*)ARG(3);

            STATUS = FMI2SetBoolean(m_instance, vr, nvr, value);

            break;
        }
        case rpc_fmi2DoStep: {
                                               
            fmi2Real    currentCommunicationPoint        = *((fmi2Real    *)ARG(1));
            fmi2Real    communicationStepSize            = *((fmi2Real    *)ARG(2));
            fmi2Boolean noSetFMUStatePriorToCurrentPoint = *((fmi2Boolean *)ARG(3));
                                   
            STATUS = FMI2DoStep(m_instance, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
                                                                      
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
