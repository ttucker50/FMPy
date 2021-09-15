from fmpy.fmucontainer import create_fmu_container, Variable, Connection

configuration = {
    'description': 'remoting test',
    'variables': [
        Variable('Real', 'continuous', 'output', 'x0', None, 'h', [('VanDerPol', 'x0')]),
        Variable('Real', 'continuous', 'output', 'x1', None, 'v', [('VanDerPol', 'x1')])],
    'components': [{
      'filename': 'VanDerPol.fmu',
      'name': 'VanDerPol'
    }],
    'connections': []
}

create_fmu_container(configuration, 'VanDerPol_container.fmu')
