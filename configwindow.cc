/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*- */
/* vim: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 ********************************************************************
 ** NIDAS: NCAR In-situ Data Acquistion Software
 **
 ** 2009, Copyright University Corporation for Atmospheric Research
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** The LICENSE.txt file accompanying this software contains
 ** a copy of the GNU General Public License. If it is not found,
 ** write to the Free Software Foundation, Inc.,
 ** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **
 ********************************************************************
*/
/*
 * This file is part of configedit:
 * A Qt based application that allows visualization of a nidas/nimbus
 * configuration (e.g. default.xml) file.
 */


#include <QtGui>
#include <ctime>
#include <list>
#include <iostream>
#include <fstream>
#include "sys/stat.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QHeaderView>

#include "configwindow.h"
#include "exceptions/exceptions.h"
#include "exceptions/QtExceptionHandler.h"
#include "exceptions/CuteLoggingExceptionHandler.h"
#include "exceptions/CuteLoggingStreamHandler.h"

using namespace nidas::core;
using namespace nidas::util;


ConfigWindow::ConfigWindow() :
   // Directory paths are relative to $PROJ_DIR
   _doc(NULL), _noProjDir(false),
   _gvDefault("/Configuration/GV_N677F/default.xml"),
   _c130Default("/Configuration/C130_N130AR/default.xml"),
   _a2dCalDir("/Configuration/cal_files/A2D/"),
    _engCalDirRoot("/Configuration/cal_files/Engineering/"),
   _pmsSpecsFile("/Configuration/PMSspecs"),
   _filename(""), _fileOpen(false)
{
try {
    //if (!(exceptionHandler = new QtExceptionHandler()))
    //if (!(exceptionHandler = new CuteLoggingExceptionHandler(this)))
    //To allow cerr debugging comments to show in gdb comment out the next two
    //lines
     if (!(exceptionHandler = new CuteLoggingStreamHandler(std::cerr,0)))
        throw 0;

    XMLPlatformUtils::Initialize(); //xercesc class
    _errorMessage = new QMessageBox(this);
    setupDefaultDir();
    buildMenus();
    sensorComboDialog = new AddSensorComboDialog(_projDir+_a2dCalDir,
                                                 _projDir+_pmsSpecsFile, this);
    dsmComboDialog = new AddDSMComboDialog(this);
    a2dVariableComboDialog = new AddA2DVariableComboDialog(this);
    variableComboDialog = new VariableComboDialog(this);
    newProjDialog = new NewProjectDialog(_projDir, this);
    } catch (InternalProcessingException &e) {

        _errorMessage->setText(QString::fromStdString
                    ("Internal Error. Get Help! " + std::string(e.what())));
        _errorMessage->exec();

    } catch (std::exception& e) {
        _errorMessage->setText(QString::fromStdString
                    ("Caught Standard Error: " + std::string(e.what())));
        _errorMessage->exec();
    } catch (...) {
        _errorMessage->setText(QString
                    ("Caught Unknown Initialization Exception"));
        _errorMessage->exec();
    }
}



/**
 * Build the dropdown menus at the top of the main configedit window
 */
void ConfigWindow::buildMenus()
{
    buildFileMenu();
    buildProjectMenu();
    buildWindowMenu();
    buildDSMMenu();
    buildSensorMenu();
    buildA2DVariableMenu();
    buildVariableMenu();
}

/**
 * Build the "File" dropdown menu
 */
void ConfigWindow::buildFileMenu()
{
    QAction * ProjAct = new QAction(tr("New &Proj Config"), this);
    ProjAct->setShortcut(tr("Ctrl+P"));
    ProjAct->setStatusTip(tr("Create a new ADS configuration file"));
    connect(ProjAct, SIGNAL(triggered()), this, SLOT(newProj()));

    QAction * openAct = new QAction(tr("&Open"), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing configuration file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(newFile()));

    QAction * saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save a configuration file"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveOldFile()));

    QAction * saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcut(tr("Ctrl+A"));
    saveAsAct->setStatusTip(tr("Save configuration as a new file"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAsFile()));

    QAction * exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(quit()));

    QMenu * fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(ProjAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(exitAct);
}


void ConfigWindow::buildProjectMenu()
{
    QMenu * menu = menuBar()->addMenu(tr("&Project"));
    QAction * projEditAct = new QAction(tr("&Edit Name"), this);
    projEditAct->setShortcut(tr("Ctrl+E"));
    projEditAct->setStatusTip(tr("Edit the Project Name"));
    connect(projEditAct, SIGNAL(triggered()), this, SLOT(editProjName()));
    menu->addAction(projEditAct);
}


void ConfigWindow::quit()
{
cerr<<"ConfigWindow::quit() called \n";
    if (_fileOpen)
        if (!askSaveFileAndContinue()) return;

    QCoreApplication::quit();
}



void ConfigWindow::buildWindowMenu()
{
    QMenu * menu = menuBar()->addMenu(tr("&Windows"));
    QAction * act;

    act = new QAction(tr("&Errors"), this);
    act->setStatusTip(tr("Toggle errors window"));
    act->setCheckable(true);
    act->setChecked(false);
    connect(act, SIGNAL(toggled(bool)), this, SLOT(toggleErrorsWindow(bool)));
    menu->addAction(act);
}



void ConfigWindow::buildSensorMenu()
{
    buildSensorActions();

    sensorMenu = menuBar()->addMenu(tr("&Sensor"));
    sensorMenu->addAction(addSensorAction);
    sensorMenu->addAction(editSensorAction);
    sensorMenu->addAction(deleteSensorAction);
    sensorMenu->setEnabled(false);
}

void ConfigWindow::buildDSMMenu()
{
    buildDSMActions();

    dsmMenu = menuBar()->addMenu(tr("&DSM"));
    dsmMenu->addAction(addDSMAction);
    dsmMenu->addAction(editDSMAction);
    dsmMenu->addAction(deleteDSMAction);
    dsmMenu->setEnabled(false);
}

void ConfigWindow::buildA2DVariableMenu()
{
    buildA2DVariableActions();

    a2dVariableMenu = menuBar()->addMenu(tr("&A2DVariable"));
    a2dVariableMenu->addAction(addA2DVariableAction);
    a2dVariableMenu->addAction(editA2DVariableAction);
    a2dVariableMenu->addAction(deleteA2DVariableAction);
    a2dVariableMenu->setEnabled(false);
}

void ConfigWindow::buildVariableMenu()
{
    buildVariableActions();

    variableMenu = menuBar()->addMenu(tr("&Variable"));
    variableMenu->addAction(editVariableAction);
    variableMenu->setEnabled(false);
}

void ConfigWindow::buildSensorActions()
{
    addSensorAction = new QAction(tr("&Add Sensor"), this);
    connect(addSensorAction, SIGNAL(triggered()), this,
            SLOT(addSensorCombo()));

    editSensorAction = new QAction(tr("&Edit Sensor"), this);
    connect(editSensorAction, SIGNAL(triggered()), this,
            SLOT(editSensorCombo()));

    deleteSensorAction = new QAction(tr("&Delete Sensor"), this);
    connect(deleteSensorAction, SIGNAL(triggered()), this,
            SLOT(deleteSensor()));
}

void ConfigWindow::buildDSMActions()
{
    addDSMAction = new QAction(tr("&Add DSM"), this);
    connect(addDSMAction, SIGNAL(triggered()), this,  SLOT(addDSMCombo()));

    editDSMAction = new QAction(tr("&Edit DSM"), this);
    connect(editDSMAction, SIGNAL(triggered()), this, SLOT(editDSMCombo()));

    deleteDSMAction = new QAction(tr("&Delete DSM"), this);
    connect(deleteDSMAction, SIGNAL(triggered()), this, SLOT(deleteDSM()));
}

void ConfigWindow::buildA2DVariableActions()
{
    editA2DVariableAction = new QAction(tr("&Edit A2DVariable"), this);
    connect(editA2DVariableAction, SIGNAL(triggered()), this,
            SLOT(editA2DVariableCombo()));

    addA2DVariableAction = new QAction(tr("&Add A2DVariable"), this);
    connect(addA2DVariableAction, SIGNAL(triggered()), this,
            SLOT(addA2DVariableCombo()));

    deleteA2DVariableAction = new QAction(tr("&Delete A2DVariable"), this);
    connect(deleteA2DVariableAction, SIGNAL(triggered()), this,
            SLOT(deleteA2DVariable()));
}

void ConfigWindow::buildVariableActions()
{
    editVariableAction = new QAction(tr("&Edit Variable"), this);
    connect(editVariableAction, SIGNAL(triggered()), this,
            SLOT(editVariableCombo()));
}

void ConfigWindow::toggleErrorsWindow(bool checked)
{
    exceptionHandler->setVisible(checked);
}

void ConfigWindow::addSensorCombo()
{
    QModelIndexList indexList; // create an empty list
    sensorComboDialog->setModal(true);
    sensorComboDialog->show(model, indexList);
    cerr<<"after call to addSensorCombo->show\n";
    tableview->resizeColumnsToContents();
}

void ConfigWindow::editSensorCombo()
{
  // Get selected index list and make sure it's only one
  //    (see note in editA2DVariableCombo)
  QModelIndexList indexList = tableview->selectionModel()->selectedIndexes();
  if (indexList.size() > 6) {
    cerr << "ConfigWindow::editSensorCombo - found more than " <<
            "one row to edit\n";
    cerr << "indexList.size() = " << indexList.size() << "\n";
    return;
  }

  // allow user to edit variable
  sensorComboDialog->setModal(true);
  sensorComboDialog->show(model, indexList);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::deleteSensor()
{
  model->removeIndexes(tableview->selectionModel()->selectedIndexes());
  cerr << "ConfigWindow::deleteSensor after removeIndexes\n";
  _doc->setIsChangedBig(true);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::addDSMCombo()
{
  QModelIndexList indexList; // create an empty list
  dsmComboDialog->setModal(true);
  dsmComboDialog->show(model, indexList);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::editDSMCombo()
{
  // Get selected index list and make sure it's only one
  //    (see note in editA2DVariableCombo)
  QModelIndexList indexList = tableview->selectionModel()->selectedIndexes();
  if (indexList.size() > 2) {
    cerr << "ConfigWindow::editSensorCombo - found more than " <<
            "one row to edit\n";
    cerr << "indexList.size() = " << indexList.size() << "\n";
    return;
  }

  // allow user to edit DSM
  dsmComboDialog->setModal(true);
  dsmComboDialog->show(model, indexList);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::deleteDSM()
{
  model->removeIndexes(tableview->selectionModel()->selectedIndexes());
  cerr << "ConfigWindow::deleteDSM after removeIndexes\n";
  _doc->setIsChangedBig(true);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::addA2DVariableCombo()
{
  QModelIndexList indexList; // create an empty list
  a2dVariableComboDialog->setModal(true);
  a2dVariableComboDialog->show(model,indexList);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::editA2DVariableCombo()
{
  // Get selected indexes and make sure it's only one
  //   NOTE: properties should force this, but if it comes up may need to
  //         provide a GUI indication.
  QModelIndexList indexList = tableview->selectionModel()->selectedIndexes();
  if (indexList.size() > 8) {
    cerr << "ConfigWindow::editA2DVariableCombo - found more than " <<
            "one row to edit \n";
    cerr << "indexList.size() = " << indexList.size() << "\n";
    return;
  }

  // allow user to edit/add variable
  a2dVariableComboDialog->setModal(true);
  a2dVariableComboDialog->show(model, indexList);
  tableview->resizeColumnsToContents();
}

void ConfigWindow::deleteA2DVariable()
{
  model->removeIndexes(tableview->selectionModel()->selectedIndexes());
  _doc->setIsChangedBig(true);
  cerr << "ConfigWindow::deleteA2DVariable after removeIndexes\n";
  tableview->resizeColumnsToContents();
}

void ConfigWindow::editVariableCombo()
{
  // Get selected indexes and make sure it's only one
  //   NOTE: properties should force this, but if it comes up may need to
  //         provide a GUI indication.
  QModelIndexList indexList = tableview->selectionModel()->selectedIndexes();
  if (indexList.size() > 6) {
    cerr << "ConfigWindow::editVariableCombo - found more than " <<
            "one row to edit \n";
    cerr << "indexList.size() = " << indexList.size() << "\n";
    return;
  }

  // allow user to edit/add variable
  variableComboDialog->setModal(true);
  variableComboDialog->show(model, indexList);
  tableview->resizeColumnsToContents();
}

/**
 *  Setup _defaultDir and _defaultCaption class variables for use in
 *  opening/viewing files.
 *  @param[in] $PROJ_DIR an environment variable
 *  @param[in] $PROJECT an environment variable
 *  @param[in] $AIRCRAFT an environment variable
 *  @param[out] _defaultDir the default directory to look for default.xml
 *  @param[out] _defaultCaption the default caption to show at the top of the
 *                              file selection window
 *  @param[out] _projDir directory where project info is stored, from PROJ_DIR
 */
void ConfigWindow::setupDefaultDir()
{
    char * _tmpStr;

    _tmpStr = getenv("PROJ_DIR");
    if (_tmpStr) {
       _defaultDir.append(_tmpStr);
       _projDir.append(_tmpStr);
    } else { // No $PROJ_DIR - warn user, set default to current and bail.
       _defaultCaption.append("No $PROJ_DIR!! ");
       QString firstPart("No $PROJ_DIR Environment Variable Defined.\n");
       QString secondPart("Configuration Editor will be missing some "
                          "functionality.\n");
       QString thirdPart("Proceeding using current working directory.\n");
       _errorMessage->setText(firstPart+secondPart+thirdPart);
       _errorMessage->exec();
       _tmpStr = getenv("PWD");
       _defaultDir.append(_tmpStr);
       _noProjDir = true;

       return;
    }

    if (_tmpStr) {
        _tmpStr = NULL;
        _tmpStr = getenv("PROJECT");
        if (_tmpStr)
        {
            _defaultDir.append("/");
            _defaultDir.append(_tmpStr);
        }
        else
            _defaultCaption.append("No $PROJECT.");
    }

    if (_tmpStr) {
        _tmpStr = NULL;
        _tmpStr = getenv("AIRCRAFT");
        if (_tmpStr)
        {
            _defaultDir.append("/");
            _defaultDir.append(_tmpStr);
            _defaultDir.append("/nidas");
        }
        else
            _defaultCaption.append(" No $AIRCRAFT.");
    }

    return;
}

bool ConfigWindow::fileExists(QString filename)
{
  struct stat buffer;
  if (stat(filename.toStdString().c_str(), &buffer) == 0) return true;
  return false;
}

void ConfigWindow::newProj()
{
  QString oldFileName = _filename;
  std::string projDir;
  char * tmpStr;

  tmpStr = getenv("PROJ_DIR");
  if (tmpStr) {
    projDir.append(tmpStr);
  } else {
    QString firstPart("No $PROJ_DIR Environment Variable Defined.\n");
    QString secondPart("Cannot create a new project.\n");
    _errorMessage->setText(firstPart+secondPart);
    _errorMessage->exec();
    return;
  }

  newProjDialog->show();  // create new project

  return;
}

void ConfigWindow::newFile()
{
    if (_fileOpen)
        if (!askSaveFileAndContinue()) return;

    getFile();
    openFile();
}

void ConfigWindow::getFile()
{
    QString caption;

    caption = _defaultCaption;
    caption.append(" Choose a file...");

    _filename = QFileDialog::getOpenFileName(
                0,
                caption,
                QString::fromStdString(_defaultDir),
                "Config Files (*.xml)");

    return;
}

void ConfigWindow::openFile()
{
    QString winTitle("configedit:  ");

    if (_filename.isNull() || _filename.isEmpty()) {
        cerr << "filename null/empty ; not opening" << endl;
        _errorMessage->setText("No File Selected...");
        _errorMessage->exec();
        winTitle.append("(no file selected)");
        setWindowTitle(winTitle);
        return;
    }
    else if (!a2dVariableComboDialog->setup(_filename.toStdString())) {
            cerr << "Problems with a2dVariableDialog setup\n";
            winTitle.append("(could not set up a2dVariable Dialog)");
            setWindowTitle(winTitle);
            return;
    }
    else {
        if (_doc) delete(_doc);
        _doc = new Document(_projDir+_engCalDirRoot, this);
        _doc->setFilename(_filename.toStdString());
        try {
            _doc->parseFile();
        }
        catch (nidas::util::InvalidParameterException &e) {
            cerr<<"caught Exception InvalidParam: " << e.toString() << "\n";
            _errorMessage->setText(QString::fromStdString
                     ("Caught nidas InvalidParameterException: "
                      + e.toString()));
            _errorMessage->exec();
            return;
         }

//       Aircraft XML files should have only one site
         vector <std::string> siteNames;
         siteNames = _doc->getSiteNames();
         if (siteNames.size() > 1) {
            if (siteNames[0]=="GV_N677F" ||
                siteNames[0]=="C130_N130AR") {
                   cerr<<"XML is for aircraft but has multiple sites\n";
                   _errorMessage->setText(_filename + QString(
                     ":: ERROR: XML is for aircraft but has multiple sites"));
                   _errorMessage->exec();
                   return;
            }
         }

//       Without Engineering Calibrations directory we'd be guessing
//       at calfile names
         if (!_doc->engCalDirExists()) {
             QString engCalDir = _doc->getEngCalDir();
             _errorMessage->setText("Could not open Engineering Cal dir:" +
                              engCalDir +
                            "\n ERROR: Can't check on Cal files. " +
                            "\n Would be guessing names - fix problem.");
             _errorMessage->exec();
             return;
         }

         try {
cerr<<"printSiteNames\n";
            _doc->printSiteNames();

            QWidget *oldCentral = centralWidget();
            if (oldCentral) {
                cerr << "got an old central widget\n";
                cerr << "NAME: " << oldCentral->objectName().toStdString() << "\n";
                cerr << "INFO::\n";
                oldCentral->dumpObjectInfo();
                cerr << "\n\nTREE:\n";
                oldCentral->dumpObjectTree();
                cerr << "\n\n";

                    /* qt docs say call setCentralWidget() only once,
                       but we need to dump the old one to make for a new file
                     */
                setCentralWidget(0);
                show();
                }

            mainSplitter = new QSplitter(this);
            mainSplitter->setObjectName(QString("the horizontal splitter!!!"));

            buildSensorCatalog();
            dsmComboDialog->setDocument(_doc);
            //sampleComboDialog->setDocument(_doc);
            a2dVariableComboDialog->setDocument(_doc);
            variableComboDialog->setDocument(_doc);
            setupModelView(mainSplitter);

            setCentralWidget(mainSplitter);

            show(); // XXX

            winTitle.append(_filename);
            setWindowTitle(winTitle);

        }
        catch (const CancelProcessingException & cpe) {
            // stop processing, show blank window
            _errorMessage->setText(QString::fromStdString
                     ("Caught CancelProcessingException" + cpe.toString()));
            _errorMessage->exec();
            return;

            QStatusBar *sb = statusBar();
            if (sb) sb->showMessage(QString::fromLatin1(cpe.what()));
        }
        catch(...) {
            // stop processing, show blank window
            _errorMessage->setText(QString::fromStdString
                     ("Caught Unknown Exception"));
            _errorMessage->exec();

            exceptionHandler->handle("Project configuration file");
            return;
        }
    }

    QList<int> sizes = mainSplitter->sizes();
    sizes[0] = 300;
    sizes[1] = 700;
    mainSplitter->setSizes(sizes);

    tableview->resizeColumnsToContents ();
    _fileOpen = true;
    show();
    return;
}

void ConfigWindow::show()
{
  resize(1400,600);

  QMainWindow::show();
}

/**
 * @brief Function to allow editing the project name
 *
 * @warning If called before a config file has been loaded, lets user
 *          know they need to create/select a config file.
 */
void ConfigWindow::editProjName()
{
cerr<<"In ConfigWindow::editProjName.  \n";
    string projName;
    if (_doc) {
      projName = _doc->getProjectName();
      cerr<<"In ConfigWindow::editProjName.  projName = " << projName << "\n";
      bool ok;
      QString text = QInputDialog::getText(this, tr("Edit Project Name"),
                                        tr("Project Name:"), QLineEdit::Normal,
                                        QString::fromStdString(projName), &ok);
      cerr<< "after call to QInputDialog::getText\n";
      if (ok && !text.isEmpty()) {
         writeProjectName(text);
      }
    } else {
      _errorMessage->setText(QString::fromStdString
                    ("Error: Create/select a configuration file first."));
      _errorMessage->exec();
    }

    return;
}

void ConfigWindow::writeProjectName(QString projName)
{
    _doc->setProjectName(projName.toStdString());

    // Now put the project name into a file in the Project Directory
    //      (needed by nimbus)
    std::string dir =  _doc->getDirectory();
    std::string projDir(dir);
    size_t found;
    found = projDir.rfind('/');
    if (found!=string::npos)
      projDir.erase(found);
    else {
      string error;
      error = "Could not find Project Directory (parent of " +
           dir + " )" + "\nUnable to write ProjectName file." +
           "This will cause problems with nimbus.";
      _errorMessage->setText(QString::fromStdString(error));
      _errorMessage->exec();
      return;
    }
    std::string projFName;
    projFName = projDir + "/ProjectName";
    ofstream projFile(projFName.c_str());
    if (projFile) {
      projFile.close();
      if (remove(projFName.c_str()) != 0) {
        string error;
        error = "Could not remove ProjectName file:" + projFName +
             + "\nUnable to write ProjectName file." +
             "This will cause problems with nimbus.";
        _errorMessage->setText(QString::fromStdString(error));
        _errorMessage->exec();
        return;
      }
    }
    projFile.open(projFName.c_str(), ios::out);
    if (projFile.is_open()) {
      projFile << projName.toStdString().c_str() << "\n";
      projFile.close();
    }
    else {
        string error;
        error = "Could not open file:" + projFName +
             + "\nUnable to write ProjectName file." +
             "This will cause problems with nimbus.";
        _errorMessage->setText(QString::fromStdString(error));
        _errorMessage->exec();
        return;
    }
}

/**
 * QT oddity wrt argument passing forces this hack
 */
void ConfigWindow::saveOldFile()
{
    saveFile("");
}

/**
 * \brief Save a configuration file
 *
 * This interface is a little confusing.  The filename that will be saved
 * is found in Document.  If an origFile is passed in, that argument is used
 * to save a copy of the previous file in .confedit directory.
 *   TODO: There's got to be a less confusing way of doing this.
 */
bool ConfigWindow::saveFile(string origFile)
{
    cerr << __func__ << endl;
    if (_filename == _projDir+_c130Default || _filename == _projDir+_gvDefault)
    {
      _errorMessage->setText(
             "Attempting to save default aircraft file - please use Save As");
      _errorMessage->exec();
      return false;
    }
    if (_doc) { // confirm user has loaded a config file
      if (!saveFileCopy(origFile)) {
        _errorMessage->setText("FAILED to write copy of file.\n No backups");
        _errorMessage->exec();
      }
      if (!_doc->writeDocument()) {
        _errorMessage->setText("FAILED TO WRITE FILE! Check permissions");
        _errorMessage->exec();
        return false;
      }
    } else {
      _errorMessage->setText(QString::fromStdString
                    ("Error: Create/select a configuration file first."));
      _errorMessage->exec();
      return false;
    }
cerr << "  Missing Cal Files:\n";
vector<QString> missingEngCalFiles = _doc->getMissingEngCalFiles();
for (size_t i=0; i<missingEngCalFiles.size(); i++) {
  cerr << missingEngCalFiles[i].toStdString();
  cerr << "\n";
}
    // Now clean up xercesc's bizzare comment formatting
    std::string syscmd;
    std::string filename = _doc->getFilename();
    std::string tmpfilename = filename + ".tmp";
    // remove all blank lines
    syscmd = "sed '/^ *$/d' " + filename + " > " + tmpfilename;
    system(syscmd.c_str());
    syscmd.clear();
    // move all start xml comments to first column
    syscmd = "sed 's/^ *<\\!/<\\!/' " + tmpfilename + " > " + filename;
    system(syscmd.c_str());
    syscmd.clear();
    // insert a blank line ahead of xml comments
    syscmd = "sed '/^ *<\\!/{x;p;x;}' " + filename + " > " + tmpfilename;
    system(syscmd.c_str());
    syscmd.clear();
    // final step in cleanup
    syscmd = "mv -f " + tmpfilename + " " + filename;
    system(syscmd.c_str());

    _doc->setIsChanged(false);
    _doc->setIsChangedBig(false);

    return true;
}

bool ConfigWindow::saveAsFile()
{
    QString qfilename;
    QString _caption;
    std::string curFileName;
    if (_doc) { // confirm user has loaded a config file
      curFileName=_doc->getFilename();
    } else {
      _errorMessage->setText(QString::fromStdString
                    ("Error: Create/select a configuration file first."));
      _errorMessage->exec();
      return false;
    }

    string filename(_doc->getDirectory());
    filename.append("/default.xml");

    qfilename = QFileDialog::getSaveFileName(
                0,
                _caption,
                QString::fromStdString(filename),
                "Config Files (*.xml)");

    cerr << "saveAs dialog returns " << qfilename.toStdString() << endl;

    if (qfilename.isNull() || qfilename.isEmpty()) {
        cerr << "qfilename null/empty ; not saving" << endl;
        _errorMessage->setText("No Filename chosen - not saving");
        _errorMessage->exec();
        return(false);
    }

    if (!qfilename.endsWith(".xml"))
      qfilename.append(".xml");

    _doc->setFilename(qfilename.toStdString().c_str());
    _filename=qfilename;

    if (saveFile(curFileName)) {
      QString winTitle("configedit:  ");
      winTitle.append(_filename);
      setWindowTitle(winTitle);
      return true;
    } else {
      _doc->setFilename(curFileName);
      _filename=QString::fromStdString(curFileName);
      return false;
    }
}

bool ConfigWindow::saveFileCopy(string origFile)
{
  std::string saveFileName = _doc->getFilename();
  size_t fn = saveFileName.rfind("/");
  std::string saveFname = saveFileName.substr(fn+1);
  std::string saveDir = saveFileName.substr(0, fn+1);
  std::string copyDir = saveDir + ".confedit";
  std::string copyFname;
  std::string copyFile;
  std::string fromFile;

  // Make copy directory if it doesn't already exist
  umask(0);
  struct stat st;
  if (stat(copyDir.c_str(), &st) != 0) { // create copy dir
    if (mkdir(copyDir.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == -1) {
      cerr << "Could not create directory " << copyDir << " to save copies\n";
      return false;
    }
  }

  // set up the copy filename
  time_t now;
  char dateTime[64];
  dateTime[0] = '\0';
  now = time(NULL);
  if (now != -1)
    strftime(dateTime, 64, "%Y%m%d-%H%M%S", gmtime(&now));
  else
    cerr << "couldn't get timestamp for copy filename\n";
  copyFname = saveFname+"."+std::string(dateTime);
  copyFile = copyDir + "/" +copyFname;

  if (origFile.length() == 0)
    fromFile = saveFileName;
  else
    fromFile = origFile;

  ifstream src(fromFile.c_str(), ifstream::in);
  if (!src) {
    // see if a .xml was added by configwindow
    size_t found;
    found = fromFile.rfind(".xml");
    if (found!=string::npos) {
      string::iterator it;
      it = fromFile.begin()+found;
      fromFile.erase(it,fromFile.end());
    }
    else
      return false;

    src.open(fromFile.c_str(), ifstream::in);
    if (!src) {
      cerr << "Could not open source file : " << fromFile << "\n";
      return false;
    }
  }
  ofstream dest(copyFile.c_str(), ifstream::out);
  if (!dest) {
    cerr << "Could not open destination file: " << copyFile << "\n";
    return false;
  }

  dest << src.rdbuf();
  if (!dest)
  {
     cerr << "Error while copying from: \n" << fromFile <<
             "\n to: \n" << copyFile << "\n";
     return false;
  }

  cerr << "copied from: \n" << fromFile <<
          "\n to: \n" << copyFile << "\n";

  return true;
}

void ConfigWindow::setupModelView(QSplitter *splitter)
{
  model = new NidasModel(Project::getInstance(), _doc->getDomDocument(), this);

  treeview = new QTreeView(splitter);
  treeview->setModel(model);
  treeview->header()->hide();

  tableview = new QTableView(splitter);
  tableview->setModel( model );
  tableview->setSelectionModel( treeview->selectionModel() );  /* common selection model */
  tableview->setSelectionBehavior( QAbstractItemView::SelectRows );
  tableview->setSelectionMode( QAbstractItemView::SingleSelection );

  //connect(treeview, SIGNAL(pressed(const QModelIndex &)), this, SLOT(changeToIndex(const QModelIndex &)));
  connect(treeview->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(changeToIndex(const QItemSelection &)));

  treeview->setCurrentIndex(treeview->rootIndex().child(0,0));

  splitter->addWidget(treeview);
  splitter->addWidget(tableview);
}


/*!
 * \brief Display and setup the correct actions for the index in \a selections.
 *
 * This version is for selectionModel's selectionChanged signal.
 * We use only the first element of \a selections,
 * expecting that the view(s) allow only one selection at a time.
 */
void ConfigWindow::changeToIndex(const QItemSelection & selections)
{
  QModelIndexList il = selections.indexes();
  if (il.size()) {
    changeToIndex(il.at(0));
    tableview->resizeColumnsToContents ();
  }
  else throw InternalProcessingException("selectionChanged signal provided no"
                                         " selections");
}


/*!
 * \brief Display and setup the correct actions for the current \a index.
 *
 * Set the table view's root index to the parent of \a index
 * and tell the model same so it returns correct headerData.
 * En/dis-able appropriate actions, e.g. add or delete choices...
 */
void ConfigWindow::changeToIndex(const QModelIndex & index)
{
  tableview->setRootIndex(index.parent());
  tableview->scrollTo(index);

  model->setCurrentRootIndex(index.parent());


  NidasItem *parentItem = model->getItem(index.parent());
  NidasItem *item = model->getItem(index);

//parentItem->setupyouractions(ahelper);
  //ahelper->addSensor(true);

    tableview->setSortingEnabled(true);
    tableview->sortByColumn(0, Qt::AscendingOrder);
    tableview->sortByColumn(1, Qt::AscendingOrder);

  if (dynamic_cast<SiteItem*>(parentItem)) {
    dsmMenu->setEnabled(true);
    a2dVariableMenu->setEnabled(false);
    variableMenu->setEnabled(false);
  } else dsmMenu->setEnabled(false);

  if (dynamic_cast<DSMItem*>(parentItem)) {
    sensorMenu->setEnabled(true);
    if (dynamic_cast<DSC_A2DSensorItem*>(item)) { // ANALOG_DMMAT
      a2dVariableMenu->setEnabled(true);
      editA2DVariableAction->setEnabled(false);
      deleteA2DVariableAction->setEnabled(false);
    } else
      a2dVariableMenu->setEnabled(false);
    variableMenu->setEnabled(false);
  } else sensorMenu->setEnabled(false);

  if (dynamic_cast<SensorItem*>(parentItem)) {
    a2dVariableMenu->setEnabled(false);
    variableMenu->setEnabled(true);
  }

  if (dynamic_cast<A2DSensorItem*>(parentItem)) { // ANALOG_NCAR
    SensorItem* sensorItem = dynamic_cast<SensorItem*>(parentItem);
    cerr << "A2D SensorItem: " << sensorItem->getBaseName().toStdString() << "\n";

    a2dVariableMenu->setEnabled(true);
    editA2DVariableAction->setEnabled(true);
    deleteA2DVariableAction->setEnabled(true);
    variableMenu->setEnabled(false);
    tableview->setSortingEnabled(true);
    tableview->sortByColumn(0, Qt::AscendingOrder);
  }

  if (dynamic_cast<DSC_A2DSensorItem*>(parentItem)) { // ANALOG_DMMAT
    SensorItem* sensorItem = dynamic_cast<SensorItem*>(parentItem);
    cerr << "DSC SensorItem: " << sensorItem->getBaseName().toStdString() << "\n";

    a2dVariableMenu->setEnabled(true);
    editA2DVariableAction->setEnabled(true);
    deleteA2DVariableAction->setEnabled(true);
    variableMenu->setEnabled(false);
    tableview->setSortingEnabled(true);
    tableview->sortByColumn(0, Qt::AscendingOrder);
  }

  // fiddle with context-dependent menus/toolbars
  /*
  XXX
  ask model: what are you
  (dis)able menu/toolbars

  NidasItem *item = model->data(index)
  qt::install(item->menus());
  */
}


/**
 * Construct the Sensor Catalog drop-down
 *
 * Pulls the devicename and suffix from the sensor line as well.
 */
void ConfigWindow::buildSensorCatalog()
{
Project *project = Project::getInstance();

    if(!project->getSensorCatalog()) {
        cerr<<"Configuration file doesn't contain a Sensor catalog!!"<<endl;
        return;
    }

    cerr<<"Putting together sensor Catalog"<<endl;

    sensorComboDialog->SensorBox->clear();
    sensorComboDialog->SensorBox->addItem("ANALOG_DMMAT");
    sensorComboDialog->SensorBox->addItem("ANALOG_NCAR");
    sensorComboDialog->clearSfxMap();
    sensorComboDialog->clearDevMap();

    xercesc::DOMElement* sensorElem;
    map<string,xercesc::DOMElement*>::const_iterator mi;
    const map<std::string,xercesc::DOMElement*>& scMap = 
                                        project->getSensorCatalog()->getMap();
    for (mi = scMap.begin(); mi != scMap.end(); mi++) {
        cerr<<"   - adding sensor:"<<(*mi).first<<endl;
        sensorComboDialog->SensorBox->addItem(QString::fromStdString(mi->first));

        sensorElem = mi->second;
        XDOMElement xnode((DOMElement *) sensorElem);

        const std::string& sSfxName = xnode.getAttributeValue("suffix");
        sensorComboDialog->addSensorSfx(QString::fromStdString(mi->first),
                                        QString::fromStdString(sSfxName));

        const std::string& sDevName = xnode.getAttributeValue("devicename");
        sensorComboDialog->addSensorDev(QString::fromStdString(mi->first),
                                        QString::fromStdString(sDevName));
    }

    sensorComboDialog->setDocument(_doc);
    return;
}

/**
 * Check to see if user would like current file saved
 */
bool ConfigWindow::askSaveFileAndContinue()
{
  QMessageBox msgBox;
  int ret = 0;

  if (_doc->isChanged() || _doc->isChangedBig()) {
    if (_doc->isChangedBig()) {
      QString msg("You have *significantly* modified the configuration.\n");
      msg.append("Suggest save to a new file, especially if mid project.\n");
      msg.append("Would you like to save to a new file?");
      msgBox.setText(msg);
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Save |
                                 QMessageBox::Discard | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Yes);
      ret = msgBox.exec();

    } else if (_doc->isChanged()) {
      msgBox.setText("You have modified the configuration.");
      msgBox.setInformativeText("Do you want to save your changes?");
      msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard
                                | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Save);
      ret = msgBox.exec();
    }

    switch (ret) {
      case QMessageBox::Yes:
        saveAsFile();
        return true;
        break;
      case QMessageBox::Save:
        saveFile(_doc->getFilename());
        return true;
        break;
      case QMessageBox::Discard:
        return true;
        break;
      case QMessageBox::Cancel:
        return false;
        break;
      default:
        // should never be reached
        return false;
        break;
    }
  }
  return true;
}
