/* This file is part of FMPy. See LICENSE.txt for license information. */

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include <libgen.h>
#else
#define _GNU_SOURCE
#include <libgen.h>
#endif

#include "cJSON.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "FMI2.h"


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

	size_t nComponents;
	FMIInstance **components;
	
	size_t nVariables;
	VariableMapping *variables;

	size_t nConnections;
	Connection *connections;

} System;


#define GET_SYSTEM \
	if (!c) return fmi2Error; \
	System *s = (System *)c; \
	fmi2Status status = fmi2OK;

#define CHECK_STATUS(S) status = S; if (status > fmi2Warning) goto END;


/***************************************************
Types for Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* fmi2GetTypesPlatform(void) { return fmi2TypesPlatform; }

const char* fmi2GetVersion(void) { return fmi2Version; }

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
    
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i];
        CHECK_STATUS(FMI2SetDebugLogging(m, loggingOn, nCategories, categories));
	}

END:
	return status;
}

static fmi2CallbackLogger s_logger = NULL;
static fmi2ComponentEnvironment s_envrionment = NULL;

void logFMIMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
    // TODO: fix instance name and prepend child instance name
    s_logger(s_envrionment, "instance", status, category, message);
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

    s_logger = functions->logger;
    s_envrionment = functions->componentEnvironment;

    if (fmuType != fmi2CoSimulation) {
        functions->logger(NULL, instanceName, fmi2Error, "logError", "Argument fmuType must be fmi2CoSimulation.");
        return NULL;
    }

	System *s = calloc(1, sizeof(System));

    char resourcesPath[4096] = "";
    char configPath[4096] = "";

    FMIURIToPath(fmuResourceLocation, resourcesPath, 4096);

    strncpy(configPath, resourcesPath, 4096);
    strncat(configPath, "config.json", 4096);

    // read the JSON file
    char * buffer = 0;
    long length;
    
    FILE * f = fopen(configPath, "rb");

    if (!f) {
        functions->logger(NULL, instanceName, fmi2Error, "logError", "Failed to read %s.", configPath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    fread(buffer, 1, length, f);
    fclose(f);
    
    cJSON *json = cJSON_Parse(buffer);

    cJSON *components = cJSON_GetObjectItem(json, "components");

    s->nComponents = cJSON_GetArraySize(components);
    s->components = calloc(s->nComponents, sizeof(FMIInstance *));

    for (int i = 0; i < s->nComponents; i++) {
            
        cJSON *component = cJSON_GetArrayItem(components, i);
    
        cJSON *name = cJSON_GetObjectItemCaseSensitive(component, "name");
        cJSON *guid = cJSON_GetObjectItemCaseSensitive(component, "guid");
        cJSON *modelIdentifier = cJSON_GetObjectItemCaseSensitive(component, "modelIdentifier");

        char libraryPath[4096] = "";

        strcpy(libraryPath, resourcesPath);
        strcat(libraryPath, "/");
        strcat(libraryPath, modelIdentifier->valuestring);

#ifdef _WIN32

#ifdef _WIN64
        strcat(libraryPath, "/binaries/win64/");
#else
        strcat(libraryPath, "/binaries/win32/");
#endif
        
        strcat(libraryPath, modelIdentifier->valuestring);
        strcat(libraryPath, ".dll");

#else

#ifdef __APPLE__
        strcat(libraryPath, "/binaries/darwin64/");
        strcat(libraryPath, modelIdentifier->valuestring);
        strcat(libraryPath, ".dylib");
#else
        strcat(libraryPath, "/binaries/linux64/");
        strcat(libraryPath, modelIdentifier->valuestring);
        strcat(libraryPath, ".so");
#endif
#endif

        FMIInstance *m = FMICreateInstance(name->valuestring, libraryPath, logFMIMessage, NULL);

        FMIStatus status = FMI2Instantiate(m, NULL, fmi2CoSimulation, guid->valuestring, visible, loggingOn);

        if (status > FMIWarning) {
            functions->logger(NULL, instanceName, fmi2Error, "logError", "Failed to instantiate %s.", name->valuestring);
            return NULL;
        }

        s->components[i] = m;
    }

    cJSON *connections = cJSON_GetObjectItem(json, "connections");

    s->nConnections = cJSON_GetArraySize(connections);
    s->connections = calloc(s->nConnections, sizeof(Connection));

    for (int i = 0; i < s->nConnections; i++) {

        cJSON *connection = cJSON_GetArrayItem(connections, i);

        cJSON *type = cJSON_GetObjectItemCaseSensitive(connection, "type");
        s->connections[i].type = type->valuestring[0];

        cJSON *startComponent = cJSON_GetObjectItemCaseSensitive(connection, "startComponent");
        s->connections[i].startComponent = startComponent->valueint;

        cJSON *endComponent = cJSON_GetObjectItemCaseSensitive(connection, "endComponent");
        s->connections[i].endComponent = endComponent->valueint;
        
        cJSON *startValueReference = cJSON_GetObjectItemCaseSensitive(connection, "startValueReference");
        s->connections[i].startValueReference = startValueReference->valueint;

        cJSON *endValueReference = cJSON_GetObjectItemCaseSensitive(connection, "endValueReference");
        s->connections[i].endValueReference = endValueReference->valueint;
	}

    cJSON *variables = cJSON_GetObjectItem(json, "variables");

	s->nVariables = cJSON_GetArraySize(variables);
	s->variables = calloc(s->nVariables, sizeof(VariableMapping));

	for (int i = 0; i < s->nVariables; i++) {
		
        cJSON *variable = cJSON_GetArrayItem(variables, i);

        cJSON *components      = cJSON_GetObjectItem(variable, "components");
        cJSON *valueReferences = cJSON_GetObjectItem(variable, "valueReferences");

        s->variables[i].size = cJSON_GetArraySize(components);
        s->variables[i].ci = calloc(s->variables[i].size, sizeof(size_t));
        s->variables[i].vr = calloc(s->variables[i].size, sizeof(fmi2ValueReference));

        for (int j = 0; j < s->variables[i].size; j++) {

            cJSON *component = cJSON_GetArrayItem(components, j);
            cJSON *valueReference = cJSON_GetArrayItem(valueReferences, j);

            s->variables[i].ci[j] = component->valueint;
            s->variables[i].vr[j] = valueReference->valueint;
        }
		
	}

    return s;
}

void fmi2FreeInstance(fmi2Component c) {

	if (!c) return;
	
	System *s = (System *)c;

	for (size_t i = 0; i < s->nComponents; i++) {
		FMIInstance *m = s->components[i];
		FMI2FreeInstance(m);
        FMIFreeInstance(m);
	}

	free(s);
}

/* Enter and exit initialization mode, terminate and reset */
fmi2Status fmi2SetupExperiment(fmi2Component c,
                               fmi2Boolean toleranceDefined,
                               fmi2Real tolerance,
                               fmi2Real startTime,
                               fmi2Boolean stopTimeDefined,
                               fmi2Real stopTime) {
    
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i];
        CHECK_STATUS(FMI2SetupExperiment(m, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime));
	}

END:
	return status;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i];
		CHECK_STATUS(FMI2EnterInitializationMode(m))
	}

END:
	return status;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i];
		CHECK_STATUS(FMI2ExitInitializationMode(m))
	}

END:
	return status;
}

fmi2Status fmi2Terminate(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i];
		CHECK_STATUS(FMI2Terminate(m))
	}

END:
	return status;
}

fmi2Status fmi2Reset(fmi2Component c) {

	GET_SYSTEM

		for (size_t i = 0; i < s->nComponents; i++) {
            FMIInstance *m = s->components[i];
			CHECK_STATUS(FMI2Reset(m))
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
        FMIInstance *m = s->components[vm.ci[0]];
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
            FMIInstance *m = s->components[vm.ci[0]];
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
            FMIInstance *m = s->components[vm.ci[0]];
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
            FMIInstance *m = s->components[vm.ci[0]];
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
            FMIInstance *m = s->components[vm.ci[j]];
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
            FMIInstance *m = s->components[vm.ci[j]];
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
            FMIInstance *m = s->components[vm.ci[j]];
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
            FMIInstance *m = s->components[vm.ci[j]];
            CHECK_STATUS(FMI2SetString(m, &(vm.vr[j]), 1, &value[i]))
        }
	}
END:
	return status;
}

/* Getting and setting the internal FMU state */
fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    return fmi2Error;
}

fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate  FMUstate) {
    return fmi2Error;
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    return fmi2Error;
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate  FMUstate, size_t* size) {
    return fmi2Error;
}

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
    return fmi2Error;
}

fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    return fmi2Error;
}

/* Getting partial derivatives */
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c,
                                        const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[],   size_t nKnown,
                                        const fmi2Real dvKnown[],
                                        fmi2Real dvUnknown[]) {
    return fmi2Error;
}

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c,
                                       const fmi2ValueReference vr[], size_t nvr,
                                       const fmi2Integer order[],
                                       const fmi2Real value[]) {
    return fmi2Error;
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c,
                                        const fmi2ValueReference vr[], size_t nvr,
                                        const fmi2Integer order[],
                                        fmi2Real value[]) {
    return fmi2Error;
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
        FMIInstance *m1 = s->components[k->startComponent];
        FMIInstance *m2 = s->components[k->endComponent];
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

	for (size_t i = 0; i < s->nComponents; i++) {
        FMIInstance *m = s->components[i];
		CHECK_STATUS(FMI2DoStep(m, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint))
	}

END:
	return status;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    return fmi2Error;
}

/* Inquire slave status */
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status*  value) {
    return fmi2Error;
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real*    value) {
    return fmi2Error;
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value) {
    return fmi2Error;
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value) {
    return fmi2Error;
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String*  value) {
    return fmi2Error;
}
