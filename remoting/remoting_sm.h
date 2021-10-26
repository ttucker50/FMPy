#pragma once

typedef enum {

    /***************************************************
    Common Functions
    ****************************************************/
    rpc_fmi2GetTypesPlatform,
    rpc_fmi2GetVersion,
    rpc_fmi2SetDebugLogging,
    rpc_fmi2Instantiate,
    rpc_fmi2FreeInstance,
    rpc_fmi2SetupExperiment,
    rpc_fmi2EnterInitializationMode,
    rpc_fmi2ExitInitializationMode,
    rpc_fmi2Terminate,
    rpc_fmi2Reset,
    rpc_fmi2GetReal,
    rpc_fmi2GetInteger,
    rpc_fmi2GetBoolean,
    rpc_fmi2GetString,
    rpc_fmi2SetReal,
    rpc_fmi2SetInteger,
    rpc_fmi2SetBoolean,
    rpc_fmi2SetString,
    rpc_fmi2GetFMUstate,
    rpc_fmi2SetFMUstate,
    rpc_fmi2FreeFMUstate,
    rpc_fmi2SerializedFMUstateSize,
    rpc_fmi2SerializeFMUstate,
    rpc_fmi2DeSerializeFMUstate,
    rpc_fmi2GetDirectionalDerivative,

    /***************************************************
    Functions for FMI2 for Model Exchange
    ****************************************************/
    rpc_fmi2EnterEventMode,
    rpc_fmi2NewDiscreteStates,
    rpc_fmi2EnterContinuousTimeMode,
    rpc_fmi2CompletedIntegratorStep,
    rpc_fmi2SetTime,
    rpc_fmi2SetContinuousStates,
    rpc_fmi2GetDerivatives,
    rpc_fmi2GetEventIndicators,
    rpc_fmi2GetContinuousStates,
    rpc_fmi2GetNominalsOfContinuousStates,

    /***************************************************
    Functions for FMI2 for Co-Simulation
    ****************************************************/
    rpc_fmi2SetRealInputDerivatives,
    rpc_fmi2GetRealOutputDerivatives,
    rpc_fmi2DoStep,
    rpc_fmi2CancelStep,
    rpc_fmi2GetStatus,
    rpc_fmi2GetRealStatus,
    rpc_fmi2GetIntegerStatus,
    rpc_fmi2GetBooleanStatus,
    rpc_fmi2GetStringStatus,

} rpcFunction;

//#include "rpc/msgpack.hpp"
//#include <string>
//#include <vector>
//
//struct LogMessage {
//	std::string instanceName;
//	int status;
//	std::string category;
//	std::string message;
//	MSGPACK_DEFINE_ARRAY(instanceName, status, category, message)
//};
//
//struct ReturnValue {
//	int status;
//	std::list<LogMessage> logMessages;
//	MSGPACK_DEFINE_ARRAY(status, logMessages)
//};
//
//struct RealReturnValue {
//	int status;
//	std::list<LogMessage> logMessages;
//	std::vector<double> value;
//	MSGPACK_DEFINE_ARRAY(status, logMessages, value)
//};
//
//struct IntegerReturnValue {
//	int status;
//	std::list<LogMessage> logMessages;
//	std::vector<int> value;
//	MSGPACK_DEFINE_ARRAY(status, logMessages, value)
//};
//
//struct EventInfoReturnValue {
//	int status;
//	std::list<LogMessage> logMessages;
//	int newDiscreteStatesNeeded;
//	int terminateSimulation;
//	int nominalsOfContinuousStatesChanged;
//	int valuesOfContinuousStatesChanged;
//	int nextEventTimeDefined;
//	double nextEventTime;
//	MSGPACK_DEFINE_ARRAY(status, logMessages, newDiscreteStatesNeeded, terminateSimulation, nominalsOfContinuousStatesChanged, valuesOfContinuousStatesChanged, nextEventTimeDefined, nextEventTime)
//};
