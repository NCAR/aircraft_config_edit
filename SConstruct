# -*- python -*-
## 2011, Copyright University Corporation for Atmospheric Research
#
# configedit:
#  A Qt based application that allows visualization of a nidas/nimbus
#  configuration (e.g. default.xml) file.
#

import os
import SCons
import eol_scons

env = Environment(tools = ['default', 'nidas', 'qt5', 'qtwidgets', 'qtgui', 'qtcore', 'qtnetwork', 'raf', 'boost_regex', 'netcdf'])

env['CXXFLAGS'] = [ '-Wall','-O2','-std=c++11' ]

env.Require(['prefixoptions', 'vardb'])

SConscript('tests/SConscript')

sources = Split("""
    main.cc
    configwindow.cc
    Document.cc
    exceptions/UserFriendlyExceptionHandler.cc
    exceptions/CuteLoggingExceptionHandler.cc
    exceptions/CuteLoggingStreamHandler.cc
    AddSensorComboDialog.cc
    AddDSMComboDialog.cc
    AddA2DVariableComboDialog.cc
    NewProjectDialog.cc
    VariableComboDialog.cc
    DeviceValidator.cc
    nidas_qmv/ProjectItem.cc
    nidas_qmv/SiteItem.cc
    nidas_qmv/DSMItem.cc
    nidas_qmv/SensorItem.cc
    nidas_qmv/A2DSensorItem.cc
    nidas_qmv/PMSSensorItem.cc
    nidas_qmv/VariableItem.cc
    nidas_qmv/A2DVariableItem.cc
    nidas_qmv/NidasItem.cc
    nidas_qmv/NidasModel.cc
""")

headers = Split("""
    configwindow.h
""")

headers += env.Uic("""AddSensorComboDialog.ui""")
headers += env.Uic("""AddDSMComboDialog.ui""")
headers += env.Uic("""AddA2DVariableComboDialog.ui""")
headers += env.Uic("""VariableComboDialog.ui""")
headers += env.Uic("""NewProjectDialog.ui""")

configedit = env.Program('configedit', sources)
env.Default(configedit)

env.Install('$INSTALL_PREFIX/bin', 'configedit')
