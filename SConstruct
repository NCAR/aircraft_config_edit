# -*- python -*-
## 2011, Copyright University Corporation for Atmospheric Research

import os
import eol_scons

env = Environment(tools = ['default', 'nidas', 'qt4', 'qtgui', 'qtcore', 'qtnetwork', 'jlocal', 'boost_regex', 'logx', 'netcdf'])

opts = eol_scons.GlobalVariables('config.py')
opts.AddVariables(('PREFIX',
                   'installation path',
                   '/opt/nidas', None, eol_scons.PathToAbsolute))

opts.Update(env)

# Check if $JLOCAL/include/raf and $JLOCAL/lib exist.
if not env.JLocalValid():
    print("Cannot find $JLOCAL/include/raf or $JLOCAL/lib. "
          "configedit will not be built")
    Return()

# Override CXXFLAGS in order to turn off -Weffc++ for now
env['CXXFLAGS'] = [ '-Wall','-O2' ]

# Add this (possibly variant) directory to CPPPATH, so header files built
# by uic will be found.
env.Append(CPPPATH = ['.'])
env.Append(LIBS=['raf','VarDB', 'domx'])
#env.Require(['logx'])

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

configedit = env.NidasProgram('configedit', sources)

name = env.subst("${TARGET.filebase}", target=configedit)

inode = env.Install('$PREFIX/bin', configedit)
env.Clean('install', inode)
