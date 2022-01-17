import os
import unittest
from fmpy import simulate_fmu, plot_result
from fmpy.fmucontainer import create_fmu_container, Variable, Connection, Configuration, Component, Unit, BaseUnit, DisplayUnit, SimpleType
from fmpy.validation import validate_fmu
import numpy as np


class FMUContainerBBTest(unittest.TestCase):

    def test_create_fmu_container(self):

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

        result = simulate_fmu(filename, stop_time=4, fmi_call_logger=print)

        plot_result(result)

    # def test_create_fmu_container(self):
    #
    #     resources = os.path.join(os.path.dirname(__file__), 'resources')
    #
    #     configuration = Configuration(
    #         parallelDoStep=False,
    #         variables=[
    #                 Variable(
    #                     type='Real',
    #                     variability='tunable',
    #                     causality='parameter',
    #                     initial='exact',
    #                     name='e',
    #                     start='0.7',
    #                     description='Coefficient',
    #                     mapping=[('bouncingBall', 'e')]
    #                 ),
    #                 Variable(
    #                     type='Real',
    #                     variability='continuous',
    #                     causality='output',
    #                     initial='calculated',
    #                     name='h',
    #                     description="Height",
    #                     mapping=[('bouncingBall', 'h')]
    #                 ),
    #             ],
    #         components=[
    #                 Component(
    #                     filename=os.path.join(resources, 'BouncingBall.fmu'),
    #                     interfaceType='ModelExchange',
    #                     name='bouncingBall'
    #                 )
    #             ]
    #     )
    #
    #     filename = 'BouncingBall_container.fmu'
    #
    #     create_fmu_container(configuration, filename)
    #
    #     problems = validate_fmu(filename)
    #
    #     self.assertEqual(problems, [])
    #
    #     result = simulate_fmu(filename, stop_time=4, fmi_call_logger=print)
    #
    #     plot_result(result)
