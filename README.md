> [!WARNING]
> ## As of 2022, RAF has chosen not to maintain this tool.
> * New analog cards mean configedit would require significant refactoring to be fully functional. Going forward, the nidas config file is hand edited by the RAF SEs.
> * Unimplemented issues have been marked "Closed as not planned" and closed. Should development continue at a future date, these unimplemented
issues can be found by looking for the grey circle icon with a slash or by going to the issues queue and filtering on
"is:issue state:closed reason:not-planned"

# configedit

The configuration editor, 'configedit', is a Qt based application that allows visualization of a nidas/nimbus configuration (e.g. default.xml) file.

To run the configuration editor, type:
    > configedit
When the GUI comes up, go to File -> Open and navigate to the default.xml file you would like to edit.

## Setting up your environment

In order to run configedit, you must have the following packages installed and environment variables set:

### Required

    > export PROJ_DIR=...
    > yum install nidas
    > yum install cmigits-nidas
    > git clone http://github.com/NCAR/aircraft_projects projects

### Required if adding a new Project via configedit

    > export RAW_DATA_DIR=..
    > export DATA_DIR=..

### Optional (but helpful)

    > export PROJECT=...
    > export AIRCRAFT=...


### Development environment

    > yum install raf-devel
    > yum install nidas-devel

### Testing

    > yum install gtest-devel

To run the tests, type:

    > scons test

## Documentation
[Doxygen](http://doxygen.nl/manual/starting.html) can ge used to generate documentation from the code, run

    > doxygen Doxyfile

Then the documentation can be viewed by pointing your browser to the html/index.html file, e.g. file:////<path_to_code_checkout>/aircraft_config_edit/html/index.html.

## Continuous Deployment

http://jenkins.eol.ucar.edu/jenkins/job/aircraft_configedit/
