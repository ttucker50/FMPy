/* This file is part of FMPy. See LICENSE.txt for license information. */

#if defined(_WIN32)
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#elif defined(__APPLE__)
#include <libgen.h>
#include <sys/syslimits.h>
#include<pthread.h>
#else
#define _GNU_SOURCE
#include <libgen.h>
#include <linux/limits.h>
#include<pthread.h>
#endif

#include <mpack.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>   /* for fabs() */


#include "FMI2.h"


#include <cvode/cvode.h>
#include <nvector/nvector_serial.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunlinsol/sunlinsol_dense.h>
#include <sundials/sundials_types.h>

#define EPSILON 1e-14
#define RTOL  RCONST(1.0e-4)

typedef struct {

    size_t size;
	size_t *ci;
	fmi2ValueReference *vr;

} VariableMapping;

typedef struct {

	char type;
	size_t startComponent;
	fmi2ValueReference startValueReference;
	size_t endComponent;
	fmi2ValueReference endValueReference;

} Connection;

typedef struct {

    FMIInterfaceType interfaceType;
    FMIInstance *instance;
    size_t nx;
    size_t nz;

#ifdef _WIN32
    HANDLE thread;
    HANDLE mutex;
#else
    pthread_t thread;
    pthread_mutex_t mutex;
#endif

    fmi2Real currentCommunicationPoint;
    fmi2Real communicationStepSize;
    fmi2Status status;
    bool doStep;
    bool terminate;

} Component;

typedef struct {

    fmi2String instanceName;
    fmi2CallbackLogger logger;
    fmi2ComponentEnvironment envrionment;

	size_t nComponents;
    Component **components;
	
	size_t nVariables;
	VariableMapping *variables;

	size_t nConnections;
	Connection *connections;

    size_t nx;
    size_t nz;
    N_Vector x;
    N_Vector abstol;
    SUNMatrix A;
    void *cvode_mem;
    SUNLinearSolver LS;

    bool parallelDoStep;

} System;

#ifdef _WIN32
DWORD WINAPI doStep(LPVOID lpParam) {

    Component *c = (Component *)lpParam;

    while (true) {

        DWORD dwWaitResult = WaitForSingleObject(c->mutex, INFINITE);

        if (c->terminate) {
            return TRUE;
        }

        if (c->doStep) {
            c->status = FMI2DoStep(c->instance, c->currentCommunicationPoint, c->communicationStepSize, fmi2True);
            c->doStep = false;
        }

        BOOL success = ReleaseMutex(c->mutex);
    }

    return TRUE;
}
#else
void* doStep(void *arg) {

    Component *c = (Component *)arg;

    while (true) {

        pthread_mutex_lock(&c->mutex);

        if (c->terminate) {
            return NULL;
        }

        if (c->doStep) {
            c->status = FMI2DoStep(c->instance, c->currentCommunicationPoint, c->communicationStepSize, fmi2True);
            c->doStep = false;
        }

        pthread_mutex_unlock(&c->mutex);
    }

    return NULL;
}
#endif

static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data) {

    System *s = (System *)user_data;

    size_t ix = 0;
    fmi2Status status; // TODO: check status

    for (size_t i = 0; i < s->nComponents; i++) {

        Component *c = s->components[i];
        FMIInstance *m = c->instance;
        
        status = FMI2SetTime(m, t);

        // TODO: forward signals?

        if (c->nx > 0) {
            status = FMI2GetContinuousStates(m, &(NV_DATA_S(y)[ix]), c->nx);
            status = FMI2GetDerivatives(m, &(NV_DATA_S(ydot)[ix]), c->nx);
            ix += c->nx;
        }
    }

    return 0;

}

static int g(realtype t, N_Vector y, realtype *gout, void *user_data) {

    System *s = (System *)user_data;

    size_t ix = 0;
    size_t iz = 0;
    fmi2Status status; // TODO: check status

    for (size_t i = 0; i < s->nComponents; i++) {

        Component *c = s->components[i];
        FMIInstance *m = c->instance;

        status = FMI2SetTime(m, t);

        if (c->nx > 0) {
            status = FMI2SetContinuousStates(m, &(NV_DATA_S(y)[ix]), c->nx);
            ix += c->nx;
        }

        // TODO: forward signals?

        if (c->nz > 0) {
            status = FMI2GetEventIndicators(m, &(gout[iz]), c->nz);
            iz += c->nz;
        }
    }

    return 0;
}

static void ehfun(int error_code, const char *module, const char *function, char *msg, void *user_data) {

    System *system = (System *)user_data;

    system->logger(system->envrionment, system->instanceName, fmi2Error, "logError", "CVode error(code %d) in module %s, function %s: %s.", error_code, module, function, msg);
}


#define GET_SYSTEM \
	if (!c) return fmi2Error; \
	System *s = (System *)c; \
	fmi2Status status = fmi2OK;

#define CHECK_STATUS(S) status = S; if (status > fmi2Warning) goto END;

#define NOT_IMPLEMENTED \
    if (c) { \
        System *system = (System *)c; \
        system->logger(system->envrionment, system->instanceName, fmi2Error, "fmi2Error", "Function is not implemented."); \
    } \
    return fmi2Error;

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

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
    
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i]->instance;
        CHECK_STATUS(FMI2SetDebugLogging(m, loggingOn, nCategories, categories));
	}

END:
	return status;
}

void logFMIMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
    
    System *s = instance->userData;
    
    size_t message_len = strlen(message);
    size_t instanceName_len = strlen(instance->name);
    size_t total_len = message_len + instanceName_len + 5;
    
    char *buf = malloc(total_len);

    snprintf(buf, total_len, "[%s]: %s", instance->name, message);

    s->logger(s->envrionment, s->instanceName, status, category, buf);

    free(buf);
}

static void logFunctionCall(FMIInstance *instance, FMIStatus status, const char *message, ...) {

    System *s = instance->userData;

    char buf[FMI_MAX_MESSAGE_LENGTH];

    va_list args;

    va_start(args, message);
    vsnprintf(buf, FMI_MAX_MESSAGE_LENGTH, message, args);
    va_end(args);

    s->logger(s->envrionment, s->instanceName, (fmi2Status)status, "debug", "[%s]: %s", instance->name, buf);
}

/* Creation and destruction of FMU instances and setting debug status */
fmi2Component fmi2Instantiate(fmi2String instanceName,
                              fmi2Type fmuType,
                              fmi2String fmuGUID,
                              fmi2String fmuResourceLocation,
                              const fmi2CallbackFunctions* functions,
                              fmi2Boolean visible,
                              fmi2Boolean loggingOn) {

	if (!functions || !functions->logger) {
		return NULL;
	}

    if (fmuType != fmi2CoSimulation) {
        functions->logger(NULL, instanceName, fmi2Error, "logError", "Argument fmuType must be fmi2CoSimulation.");
        return NULL;
    }

    char configFilename[4096] = "";
    char resourcesDir[4096]   = "";

    FMIURIToPath(fmuResourceLocation, resourcesDir, 4096);

    strcpy(configFilename, resourcesDir);
	strcat(configFilename, "config.mp");

	// parse a file into a node tree
	mpack_tree_t tree;
	mpack_tree_init_filename(&tree, configFilename, 0);
	mpack_tree_parse(&tree);
	mpack_node_t root = mpack_tree_root(&tree);

	//mpack_node_print_to_stdout(root);

    mpack_node_t parallelDoStep = mpack_node_map_cstr(root, "parallelDoStep");

    System *s = calloc(1, sizeof(System));

    s->instanceName = strdup(instanceName);
    s->logger = functions->logger;
    s->envrionment = functions->componentEnvironment;
    s->parallelDoStep = mpack_node_bool(parallelDoStep);

	mpack_node_t components = mpack_node_map_cstr(root, "components");

	s->nComponents = mpack_node_array_length(components);

	s->components = calloc(s->nComponents, sizeof(FMIInstance *));

	for (size_t i = 0; i < s->nComponents; i++) {
		mpack_node_t component = mpack_node_array_at(components, i);

		mpack_node_t name = mpack_node_map_cstr(component, "name");
		char *_name = mpack_node_cstr_alloc(name, 1024);

		mpack_node_t guid = mpack_node_map_cstr(component, "guid");
        char *_guid = mpack_node_cstr_alloc(guid, 1024);

		mpack_node_t modelIdentifier = mpack_node_map_cstr(component, "modelIdentifier");
        char *_modelIdentifier = mpack_node_cstr_alloc(modelIdentifier, 1024);

        char unzipdir[4069] = "";
        char componentResourcesDir[4069] = "";

#ifdef _WIN32
        PathCombine(unzipdir, resourcesDir, _modelIdentifier);
        PathCombine(componentResourcesDir, unzipdir, "resources");
#else
        sprintf(unzipdir, "%s/%s", resourcesDir, _modelIdentifier);
        sprintf(componentResourcesDir, "%s/%s", unzipdir, "resources");
#endif
        char componentResourcesUri[4069] = "";

        FMIPathToURI(componentResourcesDir, componentResourcesUri, 4096);

        char libraryPath[4069] = "";

        FMIPlatformBinaryPath(unzipdir, _modelIdentifier, FMIVersion2, libraryPath, 4096);

        FMIInstance *m = FMICreateInstance(_name, libraryPath, logFMIMessage, loggingOn ? logFunctionCall : NULL);

        if (!m) {
            return NULL;
        }

        m->userData = s;

        Component *c = calloc(1, sizeof(Component));

        mpack_node_t interfaceType = mpack_node_map_cstr(component, "interfaceType");
        
        mpack_node_t nx = mpack_node_map_cstr(component, "nx");
        mpack_node_t nz = mpack_node_map_cstr(component, "nz");

        c->interfaceType = (FMIInterfaceType)mpack_node_int(interfaceType);
        
        c->nx = mpack_node_int(nx);
        c->nz = mpack_node_int(nz);

        s->nx += c->nx;
        s->nz += c->nz;

        if (FMI2Instantiate(m, componentResourcesUri, (fmi2Type)c->interfaceType, _guid, visible, loggingOn) > FMIWarning) {
            return NULL;
        }

        c->instance = m;

        if (s->parallelDoStep) {
            c->doStep = false;
            c->terminate = false;
#ifdef _WIN32
            // TODO: check for invalid handles
            c->mutex = CreateMutexA(NULL, FALSE, NULL);
            c->thread = CreateThread(NULL, 0, doStep, c, 0, NULL);
#else
            // TODO: check return codes
            pthread_mutex_init(&c->mutex, NULL);
            pthread_create(&c->thread, NULL, &doStep, c);
#endif
        }

        s->components[i] = c;
	}

	mpack_node_t connections = mpack_node_map_cstr(root, "connections");

	s->nConnections = mpack_node_array_length(connections);

	s->connections = calloc(s->nConnections, sizeof(Connection));

	for (size_t i = 0; i < s->nConnections; i++) {
		mpack_node_t connection = mpack_node_array_at(connections, i);

		mpack_node_t type = mpack_node_map_cstr(connection, "type");
		s->connections[i].type = mpack_node_str(type)[0];

		mpack_node_t startComponent = mpack_node_map_cstr(connection, "startComponent");
		s->connections[i].startComponent = mpack_node_u64(startComponent);

		mpack_node_t endComponent = mpack_node_map_cstr(connection, "endComponent");
		s->connections[i].endComponent = mpack_node_u64(endComponent);

		mpack_node_t startValueReference = mpack_node_map_cstr(connection, "startValueReference");
		s->connections[i].startValueReference = mpack_node_u32(startValueReference);

		mpack_node_t endValueReference = mpack_node_map_cstr(connection, "endValueReference");
		s->connections[i].endValueReference = mpack_node_u32(endValueReference);
	}

	mpack_node_t variables = mpack_node_map_cstr(root, "variables");

	s->nVariables = mpack_node_array_length(variables);

	s->variables = calloc(s->nVariables, sizeof(VariableMapping));

	for (size_t i = 0; i < s->nVariables; i++) {
		
        mpack_node_t variable = mpack_node_array_at(variables, i);

        mpack_node_t components = mpack_node_map_cstr(variable, "components");
        mpack_node_t valueReferences = mpack_node_map_cstr(variable, "valueReferences");

        bool hasStartValue = mpack_node_map_contains_cstr(variable, "start");
        
        mpack_node_t start;
        mpack_type_t variableType;

        if (hasStartValue) {
            start = mpack_node_map_cstr(variable, "start");
            variableType = mpack_node_type(start);
        }

        s->variables[i].size = mpack_node_array_length(components);
        s->variables[i].ci = calloc(s->variables[i].size, sizeof(size_t));
        s->variables[i].vr = calloc(s->variables[i].size, sizeof(fmi2ValueReference));

        for (size_t j = 0; j < s->variables[i].size; j++) {

            mpack_node_t component = mpack_node_array_at(components, j);
            mpack_node_t valueReference = mpack_node_array_at(valueReferences, j);
            fmi2ValueReference vr = mpack_node_u32(valueReference);

            if (hasStartValue) {

                fmi2Status status;
                FMIInstance *m = s->components[j]->instance;

                switch (variableType) {
                case mpack_type_double: {
                    fmi2Real value = mpack_node_double(start);
                    status = FMI2SetReal(m, &vr, 1, &value);
                    break;
                }
                case mpack_type_int: {
                    fmi2Integer value = mpack_node_int(start);
                    status = FMI2SetInteger(m, &vr, 1, &value);
                    break;
                }
                case mpack_type_bool: {
                    fmi2Boolean value = mpack_node_bool(start);
                    status = FMI2SetBoolean(m, &vr, 1, &value);
                    break;
                }
                case mpack_type_str: {
                    fmi2String value = mpack_node_cstr_alloc(start, 2048);
                    status = FMI2SetString(m, &vr, 1, &value);
                    MPACK_FREE((void*)value);
                    break;
                }
                default:
                    break;
                }

                if (status > fmi2Warning) {
                    return NULL;
                }
            }

            s->variables[i].ci[j] = mpack_node_u64(component);
            s->variables[i].vr[j] = vr;
        }
		
	}

	// clean up and check for errors
	if (mpack_tree_destroy(&tree) != mpack_ok) {
        functions->logger(NULL, instanceName, fmi2Error, "logError", "An error occurred decoding %s.", configFilename);
		return NULL;
	}

    return s;
}

void fmi2FreeInstance(fmi2Component c) {

	if (!c) return;
	
	System *system = (System *)c;

	for (size_t i = 0; i < system->nComponents; i++) {
        Component *component = system->components[i];
		FMIInstance *m = component->instance;
		FMI2FreeInstance(m);
        FMIFreeInstance(m);
        if (system->parallelDoStep) {
#ifdef _WIN32
            CloseHandle(component->mutex);       
            CloseHandle(component->thread);
#else
            pthread_mutex_destroy(&component->mutex);
            pthread_join(component->thread, NULL);
#endif
        }
        free(component);
	}

    free((void *)system->instanceName);

    N_VDestroy(system->x);
    N_VDestroy(system->abstol);
    CVodeFree(&system->cvode_mem);
    SUNLinSolFree(system->LS);
    SUNMatDestroy(system->A);

	free(system);
}

#define ASSERT_CV_SUCCESS(f) if (f != CV_SUCCESS) { return fmi2Error; }

/* Enter and exit initialization mode, terminate and reset */
fmi2Status fmi2SetupExperiment(fmi2Component c,
                               fmi2Boolean toleranceDefined,
                               fmi2Real tolerance,
                               fmi2Real startTime,
                               fmi2Boolean stopTimeDefined,
                               fmi2Real stopTime) {
    
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i]->instance;
        CHECK_STATUS(FMI2SetupExperiment(m, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime));
	}

    // setup CVode
    if (s->nx > 0) {
        s->x = N_VNew_Serial(s->nx);
        s->abstol = N_VNew_Serial(s->nx);
        for (size_t i = 0; i < s->nx; i++) {
            NV_DATA_S(s->abstol)[i] = RTOL;
        }
        s->A = SUNDenseMatrix(s->nx, s->nx);
    } else {
        s->x = N_VNew_Serial(1);
        s->abstol = N_VNew_Serial(1);
        NV_DATA_S(s->abstol)[0] = RTOL;
        s->A = SUNDenseMatrix(1, 1);
    }

    s->cvode_mem = CVodeCreate(CV_BDF);

    int flag;

    flag = CVodeInit(s->cvode_mem, f, 0, s->x);
    ASSERT_CV_SUCCESS(flag);

    flag = CVodeSVtolerances(s->cvode_mem, RTOL, s->abstol);
    ASSERT_CV_SUCCESS(flag);

    if (s->nz > 0) {
        flag = CVodeRootInit(s->cvode_mem, (int)s->nz, g);
        ASSERT_CV_SUCCESS(flag);
    }

    s->LS = SUNLinSol_Dense(s->x, s->A);

    flag = CVodeSetLinearSolver(s->cvode_mem, s->LS, s->A);
    ASSERT_CV_SUCCESS(flag);

    flag = CVodeSetNoInactiveRootWarn(s->cvode_mem);
    ASSERT_CV_SUCCESS(flag);

    flag = CVodeSetErrHandlerFn(s->cvode_mem, ehfun, NULL);
    ASSERT_CV_SUCCESS(flag);

    flag = CVodeSetUserData(s->cvode_mem, s);
    ASSERT_CV_SUCCESS(flag);

END:
	return status;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i]->instance;
		CHECK_STATUS(FMI2EnterInitializationMode(m));
	}

END:
	return status;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        Component *c = s->components[i];
        FMIInstance *m = c->instance;
        CHECK_STATUS(FMI2ExitInitializationMode(m));
        if (c->interfaceType == FMIModelExchange) {
            CHECK_STATUS(FMI2EnterContinuousTimeMode(m));
        }
	}

END:
	return status;
}

fmi2Status fmi2Terminate(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        
        Component *component = s->components[i];
        
        CHECK_STATUS(FMI2Terminate(component->instance));

        if (s->parallelDoStep) {
#ifdef _WIN32
            WaitForSingleObject(component->mutex, INFINITE);
            component->terminate = true;
            ReleaseMutex(component->mutex);
            WaitForSingleObject(component->thread, INFINITE);
#else
            pthread_mutex_lock(&component->mutex);
            component->terminate = true;
            pthread_mutex_unlock(&component->mutex);
            pthread_join(component->thread, NULL);
#endif
        }
    }

END:
	return status;
}

fmi2Status fmi2Reset(fmi2Component c) {

	GET_SYSTEM

		for (size_t i = 0; i < s->nComponents; i++) {
            FMIInstance *m = s->components[i]->instance;
			CHECK_STATUS(FMI2Reset(m));
		}

END:
	return status;
}

/* Getting and setting variable values */
fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    
	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
        FMIInstance *m = s->components[vm.ci[0]]->instance;
		CHECK_STATUS(FMI2GetReal(m, &(vm.vr[0]), 1, &value[i]))
	}
END:
	return status;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
            FMIInstance *m = s->components[vm.ci[0]]->instance;
			CHECK_STATUS(FMI2GetInteger(m, &(vm.vr[0]), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
            FMIInstance *m = s->components[vm.ci[0]]->instance;
			CHECK_STATUS(FMI2GetBoolean(m, &(vm.vr[0]), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String  value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
            FMIInstance *m = s->components[vm.ci[0]]->instance;
			CHECK_STATUS(FMI2GetString(m, &(vm.vr[0]), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
	
	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
        for (size_t j = 0; j < vm.size; j++) {
            FMIInstance *m = s->components[vm.ci[j]]->instance;
		    CHECK_STATUS(FMI2SetReal(m, &(vm.vr[j]), 1, &value[i]))
        }
	}
END:
	return status;
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {

	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
        for (size_t j = 0; j < vm.size; j++) {
            FMIInstance *m = s->components[vm.ci[j]]->instance;
            CHECK_STATUS(FMI2SetInteger(m, &(vm.vr[j]), 1, &value[i]))
        }
	}
END:
	return status;
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {

	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
        for (size_t j = 0; j < vm.size; j++) {
            FMIInstance *m = s->components[vm.ci[j]]->instance;
            CHECK_STATUS(FMI2SetBoolean(m, &(vm.vr[j]), 1, &value[i]))
        }
	}
END:
	return status;
}

fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String  value[]) {

	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
        for (size_t j = 0; j < vm.size; j++) {
            FMIInstance *m = s->components[vm.ci[j]]->instance;
            CHECK_STATUS(FMI2SetString(m, &(vm.vr[j]), 1, &value[i]))
        }
	}
END:
	return status;
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

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    NOT_IMPLEMENTED
}

/* Getting partial derivatives */
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c,
                                        const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[],   size_t nKnown,
                                        const fmi2Real dvKnown[],
                                        fmi2Real dvUnknown[]) {
    NOT_IMPLEMENTED
}

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c,
                                       const fmi2ValueReference vr[], size_t nvr,
                                       const fmi2Integer order[],
                                       const fmi2Real value[]) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c,
                                        const fmi2ValueReference vr[], size_t nvr,
                                        const fmi2Integer order[],
                                        fmi2Real value[]) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2DoStep(fmi2Component c,
                      fmi2Real      currentCommunicationPoint,
                      fmi2Real      communicationStepSize,
                      fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {

	GET_SYSTEM

	for (size_t i = 0; i < s->nConnections; i++) {
		fmi2Real realValue;
		fmi2Integer integerValue;
		fmi2Boolean booleanValue;
		Connection *k = &(s->connections[i]);
        FMIInstance *m1 = s->components[k->startComponent]->instance;
        FMIInstance *m2 = s->components[k->endComponent]->instance;
		fmi2ValueReference vr1 = k->startValueReference;
		fmi2ValueReference vr2 = k->endValueReference;

		switch (k->type) {
		case 'R':
			CHECK_STATUS(FMI2GetReal(m1, &(vr1), 1, &realValue))
			CHECK_STATUS(FMI2SetReal(m2, &(vr2), 1, &realValue))
			break;
		case 'I':
			CHECK_STATUS(FMI2GetInteger(m1, &(vr1), 1, &integerValue))
			CHECK_STATUS(FMI2SetInteger(m2, &(vr2), 1, &integerValue))
			break;
		case 'B':
			CHECK_STATUS(FMI2GetBoolean(m1, &(vr1), 1, &booleanValue))
			CHECK_STATUS(FMI2SetBoolean(m2, &(vr2), 1, &booleanValue))
			break;
		}
		
	}

    if (s->parallelDoStep) {
    
        for (size_t i = 0; i < s->nComponents; i++) {
            Component *component = s->components[i];
#ifdef _WIN32
            WaitForSingleObject(component->mutex, INFINITE);
#else
            pthread_mutex_lock(&component->mutex);
#endif
            component->currentCommunicationPoint = currentCommunicationPoint;
            component->communicationStepSize = communicationStepSize;
            component->doStep = true;
#ifdef _WIN32
            ReleaseMutex(component->mutex);
#else
            pthread_mutex_unlock(&component->mutex);
#endif
        }

        for (size_t i = 0; i < s->nComponents; i++) {
            Component *component = s->components[i];
            bool waitForThread = true;
            fmi2Status status;

            while (waitForThread) {
#ifdef _WIN32
                WaitForSingleObject(component->mutex, INFINITE);
#else
                pthread_mutex_lock(&component->mutex);
#endif
                waitForThread = component->doStep;
                status = component->status;
#ifdef _WIN32
                ReleaseMutex(component->mutex);
#else
                pthread_mutex_unlock(&component->mutex);
#endif
            }

            CHECK_STATUS(status);
        }

    } else {

        // Co-Simulation
        for (size_t i = 0; i < s->nComponents; i++) {
            Component *c = s->components[i];
            if (c->interfaceType == FMICoSimulation) {
                FMIInstance *m = s->components[i]->instance;
                CHECK_STATUS(FMI2DoStep(m, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint))
            }
        }

        // Model Exchange
        const realtype tret = currentCommunicationPoint;
        const realtype tNext = currentCommunicationPoint + communicationStepSize;
        const realtype epsilon = (1.0 + fabs(tNext)) * EPSILON;

        const realtype tout = tNext;

        while (tret < tNext) {
            
            int flag = CVode(s->cvode_mem, tout, s->x, &tret, CV_NORMAL);

            if (flag < 0) {
                return fmi2Error;
            }

            size_t ix = 0;

            bool stepEvent = false;

            for (size_t i = 0; i < s->nComponents; i++) {

                Component *c = s->components[i];
                FMIInstance *m = c->instance;

                CHECK_STATUS(FMI2SetTime(m, tret));

                if (c->nx > 0) {
                    CHECK_STATUS(FMI2SetContinuousStates(m, &(NV_DATA_S(s->x)[ix]), c->nx));
                    ix += c->nx;
                }

                fmi2Boolean enterEventMode, terminateSimulation;

                CHECK_STATUS(FMI2CompletedIntegratorStep(m, fmi2False, &enterEventMode, &terminateSimulation));

                if (terminateSimulation) {
                    return fmi2Error;
                }

                stepEvent &= enterEventMode;
            }

            if (stepEvent || flag == CV_ROOT_RETURN /* || (eventInfo.nextEventTimeDefined && eventInfo.nextEventTime == tret)*/) {

                fmi2EventInfo eventInfo;

                ix = 0;

                for (size_t i = 0; i < s->nComponents; i++) {

                    Component *c = s->components[i];
                    FMIInstance *m = c->instance;

                    CHECK_STATUS(FMI2EnterEventMode(m));

                    // TODO: forward signals

                    do {
                        CHECK_STATUS(FMI2NewDiscreteStates(m, &eventInfo));
                    } while (eventInfo.newDiscreteStatesNeeded && !eventInfo.terminateSimulation);

                    CHECK_STATUS(FMI2EnterContinuousTimeMode(m));

                    if (s->nx > 0 && eventInfo.valuesOfContinuousStatesChanged) {
                        CHECK_STATUS(FMI2GetContinuousStates(m, &(NV_DATA_S(s->x)[ix]), c->nx));
                        ix += c->nx;
                    }
                }

                flag = CVodeReInit(s->cvode_mem, tret, s->x);
                
                if (flag < 0) return fmi2Error;

            }

        }
    }

END:
	return status;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    NOT_IMPLEMENTED
}

/* Inquire slave status */
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status*  value) {
    NOT_IMPLEMENTED
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real*    value) {
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
