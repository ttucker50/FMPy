import os

import sys
from lxml import etree
from ctypes import *
import _ctypes
from itertools import combinations


# determine the platform
if sys.platform == 'win32':
    platform = 'win'
    sharedLibraryExtension = '.dll'
    calloc = cdll.msvcrt.calloc
    free = cdll.msvcrt.free
    freeLibrary = _ctypes.FreeLibrary
elif sys.platform == 'linux':
    platform = 'linux'
    sharedLibraryExtension = '.so'
    from ctypes.util import find_library
    libc = CDLL(find_library("c"))
    calloc = libc.calloc
    free = libc.free
    freeLibrary = _ctypes.dlclose
else:
    raise Exception("Usupported platfrom: " + sys.platform)

calloc.argtypes = [c_size_t, c_size_t]
calloc.restype = c_void_p

free.argtypes = [c_void_p]

if sys.maxsize > 2**32:
    platform += '64'
else:
    platform += '32'


fmi2Component            = c_void_p
fmi2ComponentEnvironment = c_void_p
fmi2FMUstate             = c_void_p
fmi2ValueReference       = c_uint
fmi2Real                 = c_double
fmi2Integer              = c_int
fmi2Boolean              = c_int
fmi2Char                 = c_char
fmi2String               = c_char_p
fmi2Type                 = c_int
fmi2Byte                 = c_char

fmi2Status = c_int

fmi2CallbackLoggerTYPE         = CFUNCTYPE(None, fmi2ComponentEnvironment, fmi2String, fmi2Status, fmi2String, fmi2String)
fmi2CallbackAllocateMemoryTYPE = CFUNCTYPE(c_void_p, c_size_t, c_size_t)
fmi2CallbackFreeMemoryTYPE     = CFUNCTYPE(None, c_void_p)
fmi2StepFinishedTYPE           = CFUNCTYPE(None, fmi2ComponentEnvironment, fmi2Status)

fmi2ModelExchange = 0
fmi2CoSimulation  = 1

fmi2True  = 1
fmi2False = 0

fmi2StatusKind = c_int
fmi2DoStepStatus       = 0
fmi2PendingStatus      = 1
fmi2LastSuccessfulTime = 2
fmi2Terminated         = 3


def logger(a, b, c, d, e):
    print(a, b, c, d, e)

def allocateMemory(nobj, size):
    return calloc(nobj, size)

def freeMemory(obj):
    free(obj)

def stepFinished(componentEnvironment, status):
    print(combinations, status)

class fmi2CallbackFunctions(Structure):
    _fields_ = [('logger',               fmi2CallbackLoggerTYPE),
                ('allocateMemory',       fmi2CallbackAllocateMemoryTYPE),
                ('freeMemory',           fmi2CallbackFreeMemoryTYPE),
                ('stepFinished',         fmi2StepFinishedTYPE),
                ('componentEnvironment', fmi2ComponentEnvironment)]

callbacks = fmi2CallbackFunctions()
callbacks.logger               = fmi2CallbackLoggerTYPE(logger)
callbacks.allocateMemory       = fmi2CallbackAllocateMemoryTYPE(allocateMemory)
callbacks.stepFinished         = fmi2StepFinishedTYPE(stepFinished)
callbacks.freeMemory           = fmi2CallbackFreeMemoryTYPE(freeMemory)
callbacks.componentEnvironment = None

variables = {}

class ScalarVariable(object):

    def __init__(self, name, valueReference):
        self.name = name
        self.valueReference = valueReference
        self.description = None
        self.type = None
        self.start = None
        self.causality = None
        self.variability = None

class FMU2(object):

    def __init__(self, unzipdir, validate=True):

        self.unzipdir = unzipdir

        if validate:
            base_path, _ = os.path.split(__file__)
            schema_file = os.path.join(base_path, 'schema', 'fmi2', 'fmi2ModelDescription.xsd')
            schema = etree.XMLSchema(file=schema_file)
        else:
            schema = None

        parser = etree.XMLParser(schema=schema)

        tree = etree.parse(os.path.join(unzipdir, 'modelDescription.xml'), parser=parser)

        root = tree.getroot()

        self.guid       = root.get('guid')
        self.fmiVersion = root.get('fmiVersion')
        self.modelName  = root.get('modelName')
        self.causality  = root.get('causality')
        self.variability  = root.get('variability')

        coSimulation = root.find('CoSimulation')
        self.modelIdentifier = coSimulation.get('modelIdentifier')

        modelVariables = root.find('ModelVariables')

        self.variables = {}
        self.variableNames = []

        for variable in modelVariables:

            if variable.get("name") is None:
                continue

            sv = ScalarVariable(name=variable.get('name'), valueReference=int(variable.get('valueReference')))
            sv.description = variable.get('description')
            sv.start = variable.get('start')

            value = next(variable.iterchildren())
            sv.type = value.tag
            start = value.get('start')

            if start is not None:
                if sv.type == 'Real':
                    sv.start = float(start)
                elif sv.type == 'Integer':
                    sv.start = int(start)
                elif sv.type == 'Boolean':
                    sv.start = start == 'true'
                else:
                    sv.start = start

            self.variableNames.append(sv.name)
            self.variables[sv.name] = sv

        # load the shared library
        library = cdll.LoadLibrary(os.path.join(unzipdir, 'binaries', platform, self.modelIdentifier + sharedLibraryExtension))
        self.dll = library

        self.fmi2Instantiate = getattr(library, 'fmi2Instantiate')
        self.fmi2Instantiate.argtypes = [fmi2String, fmi2Type, fmi2String, fmi2String, POINTER(fmi2CallbackFunctions), fmi2Boolean, fmi2Boolean]
        self.fmi2Instantiate.restype = fmi2ComponentEnvironment

        self.fmi2SetupExperiment          = getattr(library, 'fmi2SetupExperiment')
        self.fmi2SetupExperiment.argtypes = [fmi2Component, fmi2Boolean, fmi2Real, fmi2Real, fmi2Boolean, fmi2Real]
        self.fmi2SetupExperiment.restype  = fmi2Status

        self.fmi2EnterInitializationMode          = getattr(library, 'fmi2EnterInitializationMode')
        self.fmi2EnterInitializationMode.argtypes = [fmi2Component]
        self.fmi2EnterInitializationMode.restype  = fmi2Status

        self.fmi2ExitInitializationMode          = getattr(library, 'fmi2ExitInitializationMode')
        self.fmi2ExitInitializationMode.argtypes = [fmi2Component]
        self.fmi2ExitInitializationMode.restype  = fmi2Status

        self.fmi2DoStep          = getattr(library, 'fmi2DoStep')
        self.fmi2DoStep.argtypes = [fmi2Component, fmi2Real, fmi2Real, fmi2Boolean]
        self.fmi2DoStep.restype  = fmi2Status

        self.fmi2GetReal          = getattr(library, 'fmi2GetReal')
        self.fmi2GetReal.argtypes = [fmi2Component, POINTER(fmi2ValueReference), c_size_t, POINTER(fmi2Real)]
        self.fmi2GetReal.restype  = fmi2Status

        self.fmi2GetInteger          = getattr(library, 'fmi2GetInteger')
        self.fmi2GetInteger.argtypes = [fmi2Component, POINTER(fmi2ValueReference), c_size_t, POINTER(fmi2Integer)]
        self.fmi2GetInteger.restype  = fmi2Status

        self.fmi2GetBoolean          = getattr(library, 'fmi2GetBoolean')
        self.fmi2GetBoolean.argtypes = [fmi2Component, POINTER(fmi2ValueReference), c_size_t, POINTER(fmi2Boolean)]
        self.fmi2GetBoolean.restype  = fmi2Status

        self.fmi2SetReal          = getattr(library, 'fmi2SetReal')
        self.fmi2SetReal.argtypes = [fmi2Component, POINTER(fmi2ValueReference), c_size_t, POINTER(fmi2Real)]
        self.fmi2SetReal.restype  = fmi2Status

        self.fmi2SetInteger          = getattr(library, 'fmi2SetInteger')
        self.fmi2SetInteger.argtypes = [fmi2Component, POINTER(fmi2ValueReference), c_size_t, POINTER(fmi2Integer)]
        self.fmi2SetInteger.restype  = fmi2Status

        self.fmi2SetBoolean          = getattr(library, 'fmi2SetBoolean')
        self.fmi2SetBoolean.argtypes = [fmi2Component, POINTER(fmi2ValueReference), c_size_t, POINTER(fmi2Boolean)]
        self.fmi2SetBoolean.restype  = fmi2Status

        self.fmi2GetBooleanStatus          = getattr(library, 'fmi2GetBooleanStatus')
        self.fmi2GetBooleanStatus.argtypes = [fmi2Component, fmi2StatusKind, POINTER(fmi2Boolean)]
        self.fmi2GetBooleanStatus.restype  = fmi2Status

        self.fmi2Terminate          = getattr(library, 'fmi2Terminate')
        self.fmi2Terminate.argtypes = [fmi2Component]
        self.fmi2Terminate.restype  = fmi2Status

        self.fmi2FreeInstance          = getattr(library, 'fmi2FreeInstance')
        self.fmi2FreeInstance.argtypes = [fmi2Component]
        self.fmi2FreeInstance.restype  = None

    def instantiate(self, instance_name, kind):
        self.component = self.fmi2Instantiate(instance_name.encode('UTF-8'), kind,
                                              self.guid.encode('UTF-8'),
                                              ('file://' + self.unzipdir).encode('UTF-8'),
                                              byref(callbacks), fmi2False,
                                              fmi2False)

    def setupExperiment(self, tolerance, startTime, stopTime=None):

        toleranceDefined = tolerance is not None

        if tolerance is None:
            tolerance = 0.0

        stopTimeDefined = stopTime is not None

        if stopTime is None:
            stopTime = 0.0

        status = self.fmi2SetupExperiment(self.component, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime)

    def enterInitializationMode(self):
        status = self.fmi2EnterInitializationMode(self.component)

    def exitInitializationMode(self):
        status = self.fmi2ExitInitializationMode(self.component)

    def doStep(self, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint):
        status = self.fmi2DoStep(self.component, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint)
        return status

    def getBooleanStatus(self, kind):
        value = fmi2Boolean(fmi2False)
        status = self.fmi2GetBooleanStatus(self.component, kind, byref(value))
        return value

    def getReal(self, vr):
        value = (fmi2Real * len(vr))()
        status = self.fmi2GetReal(self.component, vr, len(vr), value)
        return list(value)

    def setReal(self, vr, value):
        status = self.fmi2SetReal(self.component, vr, len(vr), value)

    def terminate(self):
        status = self.fmi2Terminate(self.component)

    def freeInstance(self):
        self.fmi2FreeInstance(self.component)

        # unload the shared library
        freeLibrary(self.dll._handle)
