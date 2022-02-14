import os
import unittest
from fmpy import simulate_fmu, plot_result
from fmpy.fmucontainer import create_fmu_container, Variable, Connection, Configuration, Component, Unit, BaseUnit, DisplayUnit, SimpleType
from fmpy.validation import validate_fmu
import numpy as np


class FMUContainerBBTest(unittest.TestCase):

    def test_create_fmu_container_me(self):

        resources = os.path.join(os.path.dirname(__file__), 'resources')

        configuration = Configuration(
            parallelDoStep=False,
            variables=[
                Variable(
                    type='Real',
                    variability='continuous',
                    causality='output',
                    initial='calculated',
                    name='h',
                    description='Height',
                    mapping=[('ball', 'h')]
                ),
                Variable(
                    type='Boolean',
                    variability='discrete',
                    causality='output',
                    initial='calculated',
                    name='reset',
                    description="Reset",
                    mapping=[('bounce', 'reset')]
                ),
                Variable(
                    type='Real',
                    variability='discrete',
                    causality='output',
                    initial='calculated',
                    name='ticks',
                    description='Ticks',
                    mapping=[('ticker', 'ticks')]
                ),
            ],
            components=[
                Component(
                    filename=os.path.join(resources, 'Bounce.fmu'),
                    interfaceType='ModelExchange',
                    name='bounce'
                ),
                Component(
                    filename=os.path.join(resources, 'Ball.fmu'),
                    interfaceType='ModelExchange',
                    name='ball'
                ),
                Component(
                    filename=os.path.join(resources, 'Ticker.fmu'),
                    interfaceType='ModelExchange',
                    name='ticker'
                )
            ],
            connections=[
                Connection('ball', 'h', 'bounce', 'h'),
                Connection('bounce', 'reset', 'ball', 'reset'),
            ]
        )

        filename = 'BouncingAndBall.fmu'

        create_fmu_container(configuration, filename)

        problems = validate_fmu(filename)

        self.assertEqual(problems, [])

        result = simulate_fmu(filename, stop_time=3.5, fmi_call_logger=None)

        plot_result(result)

    def test_create_fmu_container_cs(self):

        resources = os.path.join(os.path.dirname(__file__), 'resources')

        configuration = Configuration(
            parallelDoStep=True,
            description="A controlled drivetrain",
            variableNamingConvention='structured',
            unitDefinitions=[
                Unit(name='rad/s', baseUnit=BaseUnit(rad=1, s=-1), displayUnits=[DisplayUnit(name='rpm', factor=0.1047197551196598)])
            ],
            typeDefinitions=[
                SimpleType(name='AngularVelocity', type='Real', unit='rad/s')
            ],
            variables=[
                Variable(
                    type='Real',
                    variability='tunable',
                    causality='parameter',
                    initial='exact',
                    name='k',
                    start='100',
                    description='Gain of controller',
                    mapping=[('controller', 'PI.k')]
                ),
                Variable(
                    type='Real',
                    variability='continuous',
                    causality='input',
                    name='w_ref',
                    start='0',
                    description='Reference speed',
                    mapping=[('controller', 'u_s')],
                    declaredType='AngularVelocity'
                ),
                Variable(
                    type='Real',
                    variability='continuous',
                    causality='output',
                    initial='calculated',
                    name='w',
                    description="Gain of controller",
                    mapping=[('drivetrain', 'w')],
                    unit='rad/s',
                    displayUnit='rpm'
                ),
            ],
            components=[
                Component(
                    filename=os.path.join(resources, 'Controller.fmu'),
                    interfaceType='CoSimulation',
                    name='controller'
                ),
                Component(
                    filename=os.path.join(resources, 'Drivetrain.fmu'),
                    interfaceType='CoSimulation',
                    name='drivetrain',
                )
            ],
            connections=[
                Connection('drivetrain', 'w', 'controller', 'u_m'),
                Connection('controller', 'y', 'drivetrain', 'tau'),
            ]
        )

        filename = 'ControlledDrivetrain.fmu'

        create_fmu_container(configuration, filename)

        problems = validate_fmu(filename)

        self.assertEqual(problems, [])

        w_ref = np.array([(0.5, 0), (1.5, 1), (2, 1), (3, 0)], dtype=[('time', 'f8'), ('w_ref', 'f8')])

        result = simulate_fmu(filename, start_values={'k': 20}, input=w_ref, output=['w_ref', 'w'], stop_time=4)

        plot_result(result)
