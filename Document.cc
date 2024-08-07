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


#include "Document.h"
#include "configwindow.h"
#include "exceptions/InternalProcessingException.h"
#include <nidas/util/InvalidParameterException.h>

#include <sys/param.h>
#include <libgen.h>
#include <dirent.h>

#include <xercesc/util/XMLUniDefs.hpp>

#if XERCES_VERSION_MAJOR < 3
#include <xercesc/dom/DOMWriter.hpp>
#else
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
//#include <xercesc/dom/DOMLSSerializer.hpp>
#endif

#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>

#include <nidas/core/XMLParser.h>
#include <nidas/core/DSMSensor.h>

#include <iostream>
#include <set>
#include <vector>

using namespace std;
using namespace xercesc;
using namespace nidas::core;



static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
static const XMLCh gNull[] = { chNull };



const char *Document::getDirectory() const
{
    static char buf[MAXPATHLEN];

    strncpy(buf,filename->c_str(),MAXPATHLEN-1);
    return dirname(buf);
}



bool Document::writeDocument()
{
    LocalFileFormatTarget *target;

    try {
        cerr<<__func__ << " : filename = "<<filename->c_str()<<"\n";
        target = new LocalFileFormatTarget(filename->c_str());
        if (!target) {
            cerr << "target is null" << endl;
            return false;
            }
    } catch (...) {
        cerr << "LocalFileFormatTarget new exception" << endl;
        return false;
    }

    writeDOM(target,domdoc);
    delete target;
    return true;
}



#if XERCES_VERSION_MAJOR < 3
bool Document::writeDOM( XMLFormatTarget * const target, const DOMNode * node )
{
    cerr << __func__ << " called\n";
    DOMImplementation *domimpl;
    DOMImplementationLS *lsimpl;
    DOMWriter *myWriter;

    try {
        domimpl = XMLImplementation::getImplementation();
        //DOMImplementation *domimpl = DOMImplementationRegistry::getDOMImplementation(gLS);
    } catch (...) {
        cerr << "  getImplementation exception" << endl;
        return(false);
    }

    if (!domimpl) {
        cerr << "  xml implementation is null" << endl;
        return(false);
    }

    try {
        lsimpl =
        // (DOMImplementationLS*)domimpl;
         (domimpl->hasFeature(gLS,gNull)) ? (DOMImplementationLS*)domimpl : 0;
    } catch (...) {
        cerr << "  hasFeature/cast exception" << endl;
        return(false);
    }

    if (!lsimpl) {
        cerr << "  dom implementation is null" << endl;
        return(false);
    }

    try {
        myWriter = lsimpl->createDOMWriter();
        if (!myWriter) {
            cerr << "  writer is null" << endl;
            return(false);
            }
    } catch (...) {
        cerr << "  createDOMWriter exception" << endl;
        return(false);
    }

        if (myWriter->canSetFeature(XMLUni::fgDOMWRTValidation, true))
            myWriter->setFeature(XMLUni::fgDOMWRTValidation, true);
        if (myWriter->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
            myWriter->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
        else
            cerr << "  set of Writer format pretty print failed...\n";

        myWriter->setErrorHandler(&errorHandler);

    try {
        if (!myWriter->writeNode(target,*node)) {
            cerr << "  writeNode returns false" << endl;
            return false;
        }
    } catch (...) {
        cerr << "  writeNode exception" << endl;
        return(false);
    }

    target->flush();
    myWriter->release();

    return(true);
}
#else
bool Document::writeDOM( XMLFormatTarget * const target, const DOMNode * node )
{
    cerr << __func__ << " called\n";
    DOMImplementation *domimpl = 0;
    DOMImplementationLS *lsimpl = 0;
    DOMLSSerializer *mySerializer = 0;

    try {
        domimpl = XMLImplementation::getImplementation();
    } catch (...) {
        cerr << "  getImplementation exception" << endl;
        return(false);
    }

    if (!domimpl) {
        cerr << "  xml implementation is null" << endl;
        return(false);
    }

    lsimpl = (DOMImplementationLS*)domimpl;
    try {
        mySerializer = lsimpl->createLSSerializer();
        if (!mySerializer) {
            cerr << "  Serializer is null" << endl;
            return(false);
            }
    } catch (...) {
        cerr << "  createDOMSerializer exception" << endl;
        return(false);
    }

    if (mySerializer->getDomConfig()->canSetParameter(
                XMLUni::fgDOMWRTDiscardDefaultContent, true))
        mySerializer->getDomConfig()->setParameter(
                XMLUni::fgDOMWRTDiscardDefaultContent, true);

    if (mySerializer->getDomConfig()->canSetParameter(
                XMLUni::fgDOMWRTFormatPrettyPrint, true))
        mySerializer->getDomConfig()->setParameter(
                XMLUni::fgDOMWRTFormatPrettyPrint, true);
    else
        cerr << "  set of Serializer format pretty print failed...\n";

    mySerializer->getDomConfig()->setParameter
                     (XMLUni::fgDOMErrorHandler, &errorHandler);

    DOMLSOutput* theOutput = ((DOMImplementationLS*)domimpl)->createLSOutput();
    theOutput->setByteStream(target);
    try {
        if (!mySerializer->write(node,theOutput)) {
            cerr << "  writeNode returns false" << endl;
            return false;
        }
    } catch (...) {
        cerr << "  writeNode exception" << endl;
        return(false);
    }

    target->flush();
    theOutput->release();
    mySerializer->release();

    return(true);
}

#endif



void Document::parseFile()
{
    cerr << "Document::parseFile()" << endl;
    if (!filename) return;

    XMLParser * parser = new XMLParser();

    // turn on validation
    parser->setDOMValidation(true);
    parser->setDOMValidateIfSchema(true);
    parser->setDOMNamespaces(true);
    parser->setXercesSchema(true);
    parser->setXercesSchemaFullChecking(true);
    parser->setDOMDatatypeNormalization(false);
    parser->setXercesHandleMultipleImports(true);
    parser->setXercesDoXInclude(true);
    parser->setXercesUserAdoptsDOMDocument(true);

    cerr << "parsing: " << *filename << endl;
    // build Document Object Model (DOM) tree
    domdoc = parser->parse(*filename);
    cerr << "parsed" << endl;
    delete parser;

    _project = new Project(); // start anew

    // build Project tree
    _project->fromDOMElement(domdoc->getDocumentElement());

    vector <std::string> siteNames;
    siteNames=getSiteNames();
    _engCalDir = _engCalDirRoot + QString::fromStdString(siteNames[0])
                     + QString::fromStdString("/");

    cerr<<"Engineering cal dir = ";
    cerr<<_engCalDir.toStdString();
    cerr<<"\n";

    DIR *dir;
    char *temp_dir = (char*) malloc(_engCalDir.size()+1);

    strcpy(temp_dir, _engCalDir.toStdString().c_str());

    if ((dir = opendir(temp_dir)) == 0) {
       _engCalDirExists = false;
       return;
    } else {
       _engCalDirExists = true;
    }

    // Read filenames and keep those that are .dat (Engineering cal files)
    // Put files with "_" in them in the front of the list so that they
    // are preferentially found when looking for engineering cal files.
    struct dirent *entry;
    std::vector<QString>::iterator it;
    cerr<<"Found Engineering CalFiles: ";
    while ( (entry = readdir(dir)) )
        if (strstr(entry->d_name, ".dat")) {
            if (strstr(entry->d_name, "_")) {
                it = _engCalFiles.begin();
                _engCalFiles.insert(it,QString(entry->d_name));
            } else {
                _engCalFiles.push_back(QString(entry->d_name));
            }
            cerr<<entry->d_name<<" ";
        }
    cerr<<"\n";

    free(temp_dir);
}

/**
 * @return The project name.
 */
string Document::getProjectName() const
{
    return (_project->getName());
}

void Document::setProjectName(string projectName)
{
  NidasModel *model = _configWindow->getModel();
  ProjectItem * projectItem = dynamic_cast<ProjectItem*>(model->getRootItem());
  if (!projectItem)
    throw InternalProcessingException("Model root index is not a Project.");

  // Set the project name in the DOM tree
  xercesc::DOMNode * projectNode = projectItem->getDOMNode();
  if (!projectNode)  {
    throw InternalProcessingException("null Project DOM node");
  }

  if (projectNode->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)
    throw InternalProcessingException("Project DOM Node is not an Element Node!");
  xercesc::DOMElement * projectElement = ((xercesc::DOMElement*) projectNode);

  //xercesc::DOMElement * projectElement = dynamic_cast<DOMElement*>(projectNode);
  //if (!projectElement)
  //  throw InternalProcessingException("Project DOM Node is not an Element Node!");

  projectElement->removeAttribute((const XMLCh*)XMLStringConverter("name"));
  projectElement->setAttribute((const XMLCh*)XMLStringConverter("name"),
                              (const XMLCh*)XMLStringConverter(projectName));

  // Now set the project name in the nidas tree
  //  NOTE: need to do this after changing the DOM attribute as ProjectItem
  //  uses old name in setting itself up.
  _project->setName(projectName);

  return;
}

/*!
 * \brief Find a sensor in Project's DOM sensor catalog
 *        based on its XML attribute "ID" (i.e. the Name)
 *
 * \return DOMElement pointer from the Sensor Catalog or NULL
 */
const xercesc::DOMElement* Document::findSensor(const std::string & sensorIdName)
{

    if (sensorIdName.empty()) return(NULL);
    if(!_project->getSensorCatalog()) {
        cerr<<"Configuration file doesn't contain a catalog!!"<<endl;
        return(0);
    }
    map<string,xercesc::DOMElement*>::const_iterator mi;
    const map<std::string,xercesc::DOMElement*>& scMap = _project->getSensorCatalog()->getMap();
    for (mi = scMap.begin(); mi != scMap.end(); mi++) {
        if (mi->first == sensorIdName) {
           return mi->second;
        }
    }
    return(NULL);
}

void Document::updateSensor(const std::string & sensorIdName,
                            const std::string & device,
                            const std::string & lcId,
                            const std::string & sfx,
                            const std::string & a2dTempSfx,
                            const std::string & a2dSNFname,
                            const std::string & pmsSN,
                            const std::string & resltn,
                            QModelIndexList indexList)
{
cerr<<"entering Document::updateSensor\n";

  // Gather together all the elements we'll need to update the Sensor
  // in both the DOM model and the Nidas Model
  NidasModel *model = _configWindow->getModel();
  DSMItem* dsmItem = dynamic_cast<DSMItem*>(model->getCurrentRootItem());
  if (!dsmItem)
    throw InternalProcessingException("Current root index is not a DSM.");

  DSMConfig *dsmConfig = dsmItem->getDSMConfig();
  if (!dsmConfig)
    throw InternalProcessingException("null DSMConfig");

  NidasItem *item = 0;
  if (indexList.size() > 0)  {
    for (int i=0; i<indexList.size(); i++) {
      QModelIndex index = indexList[i];
      // the NidasItem for the selected row resides in column 0
      if (index.column() != 0) continue;
      if (!index.isValid()) continue;
      item = model->getItem(index);
    }
  }

  if (!item) throw InternalProcessingException("null DSMConfig");
  SensorItem* sItem = dynamic_cast<SensorItem*>(item);
  if (!sItem) throw InternalProcessingException("Sensor Item not selected");
  // Confirm we got an A2D sensor item
  A2DSensorItem *a2dSensorItem = dynamic_cast<A2DSensorItem*>(sItem);
  DSC_A2DSensorItem *dscA2dSensorItem = dynamic_cast<DSC_A2DSensorItem*>(sItem);
  PMSSensorItem* pmsSensorItem = dynamic_cast<PMSSensorItem*>(sItem);

  // Get the Sensor, save all the current values and then
  //  update to the new values
  DSMSensor* sensor = sItem->getDSMSensor();
  std::string currDevName = sensor->getDeviceName();
  unsigned int currSensorId = sensor->getSensorId();
  std::string currSuffix = sensor->getSuffix();
  QString currA2DTempSfx;
  std::string currA2DCalFname;
  if (a2dSensorItem) {
    currA2DTempSfx = a2dSensorItem->getA2DTempSuffix();
  }

  if (a2dSensorItem||dscA2dSensorItem) {
    const map<string,CalFile*>& cfs = sensor->getCalFiles();
    if (!cfs.empty()) {
        currA2DCalFname = cfs.begin()->second->getFile();
        cerr<< "Current calfile name: " << currA2DCalFname <<"\n";
    }
  }

  // Start by updating the sensor DOM
  updateSensorDOM(sItem, device, lcId, sfx);

  // If we've got an NCAR analog sensor then we need to update it's temp suffix
  if (a2dSensorItem) {
    a2dSensorItem->updateDOMA2DTempSfx(currA2DTempSfx, a2dTempSfx);
  }
  // If we've got an analog sensor of any kind, then we need to update its
  // calibration file name
  if (a2dSensorItem) {
    cerr<< "calling updateDOMCalFile("<<a2dSNFname<<")\n";
    a2dSensorItem->updateDOMCalFile(a2dSNFname);
  }
  if (dscA2dSensorItem) {
    cerr<< "calling DSC updateDOMCalFile("<<a2dSNFname<<")\n";
    dscA2dSensorItem->updateDOMCalFile(a2dSNFname);
  }

  // If we've got a PMS sensor then we need to update it's serial number
  if (pmsSensorItem)
    pmsSensorItem->updateDOMPMSParams(pmsSN, resltn);

  // Now we need to validate that all is right with the updated sensor
  // information - and if not change it all back to the original state
  try {
    sItem->fromDOM();

    // make sure new sensor works well with old (e.g. var names and suffix)
    //Site* site = const_cast <Site *> (dsmConfig->getSite());
    //site->validate();

  } catch (nidas::util::InvalidParameterException &e) {

    stringstream strS;
    strS<<currSensorId;
    updateSensorDOM(sItem, currDevName, strS.str(), currSuffix);
    if (a2dSensorItem) {
      a2dSensorItem->updateDOMA2DTempSfx(QString::fromStdString(a2dTempSfx),
                                         currA2DTempSfx.toStdString());
    }
    if (a2dSensorItem) {
      a2dSensorItem->updateDOMCalFile(currA2DCalFname);
    }
    if (dscA2dSensorItem) {
      dscA2dSensorItem->updateDOMCalFile(currA2DCalFname);
    }
    sItem->fromDOM();

    throw(e); // notify GUI
  } catch (InternalProcessingException const &) {
    stringstream strS;
    strS<<currSensorId;
    this->updateSensorDOM(sItem, currDevName, strS.str(), currSuffix);
    if (a2dSensorItem) {
      a2dSensorItem->updateDOMA2DTempSfx(QString::fromStdString(a2dTempSfx),
                                         currA2DTempSfx.toStdString());
    }
    if (a2dSensorItem) {
      a2dSensorItem->updateDOMCalFile(currA2DCalFname);
    }
    if (dscA2dSensorItem) {
      dscA2dSensorItem->updateDOMCalFile(currA2DCalFname);
    }
    sItem->fromDOM();
    throw; // notify GUI
  }

  // Looks like the new values all pass the mustard...
  std::cerr << "Finished updating sensor values - all seems ok\n";
  printSiteNames();
}

void Document::updateSensorDOM(SensorItem * sItem, const std::string & device,
                               const std::string & lcId, const std::string & sfx)
{
  // get the DOM node for this Sensor
  xercesc::DOMNode *sensorNode = sItem->getDOMNode();
  if (!sensorNode) {
    throw InternalProcessingException("null sensor DOM node");
  }

  // get the DOM element for this Sensor
  if (sensorNode->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)
    throw InternalProcessingException("Sensor DOM Node is not an Element Node!");
  xercesc::DOMElement* sensorElem = ((xercesc::DOMElement*) sensorNode);

  // setup the DOM element from user input
  sensorElem->removeAttribute((const XMLCh*)XMLStringConverter("devicename"));
  sensorElem->setAttribute((const XMLCh*)XMLStringConverter("devicename"),
                           (const XMLCh*)XMLStringConverter(device));
  sensorElem->removeAttribute((const XMLCh*)XMLStringConverter("id"));
  sensorElem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                     (const XMLCh*)XMLStringConverter(lcId));
  if ((sensorElem->getAttributeNode((const XMLCh*)XMLStringConverter("suffix")) != NULL))
    sensorElem->removeAttribute((const XMLCh*)XMLStringConverter("suffix"));

  if (!sfx.empty()) {
    sensorElem->setAttribute((const XMLCh*)XMLStringConverter("suffix"),
                             (const XMLCh*)XMLStringConverter(sfx));
  }
}


void Document::addSensor(const std::string & sensorIdName,
                         const std::string & device,
                         const std::string & lcId,
                         const std::string & sfx,
                         const std::string & a2dTempSfx,
                         const std::string & a2dSNFname,
                         const std::string & pmsSN,
                         const std::string & resltn)
{
cerr << "entering Document::addSensor about to make call to "
     << "_configWindow->getModel()\n  configwindow address = "
     << _configWindow << "\n";
  NidasModel *model = _configWindow->getModel();
  DSMItem* dsmItem = dynamic_cast<DSMItem*>(model->getCurrentRootItem());
  if (!dsmItem)
    throw InternalProcessingException("Current root index is not a DSM.");

  DSMConfig *dsmConfig = dsmItem->getDSMConfig();
  if (!dsmConfig)
    throw InternalProcessingException("null DSMConfig");

// gets XML tag name for the selected sensor
  const XMLCh * tagName = 0;
  XMLStringConverter xmlSensor("sensor");
  if (sensorIdName == "ANALOG_NCAR"||sensorIdName =="ANALOG_DMMAT") {
    tagName = (const XMLCh *) xmlSensor;
    cerr << "Analog Tag Name is " <<  (std::string)XMLStringConverter(tagName) << endl;
  } else { // look for the sensor ID in the catalog
    const DOMElement * sensorCatElement;
    sensorCatElement = findSensor(sensorIdName);
    if (sensorCatElement == NULL) {
        cerr << "Null sensor DOMElement found for sensor " << sensorIdName << endl;
        throw InternalProcessingException("null sensor DOMElement");
        }
    tagName = sensorCatElement->getTagName();
  }

// get the DOM node for this DSM
  xercesc::DOMNode *dsmNode = dsmItem->getDOMNode();
  if (!dsmNode) {
    throw InternalProcessingException("null dsm DOM node");
  }
  cerr << "past getDSMNode()\n";

    // create a new DOM element
  xercesc::DOMElement* elem = 0;
  try {
     elem = dsmNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         tagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new sensor element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

    // setup the new DOM element from user input
  if (sensorIdName == "ANALOG_NCAR") {
    elem->setAttribute((const XMLCh*)XMLStringConverter("class"),
                       (const XMLCh*)XMLStringConverter("raf.DSMAnalogSensor"));
  } else if (sensorIdName == "ANALOG_DMMAT") {
    elem->setAttribute((const XMLCh*)XMLStringConverter("class"),
                       (const XMLCh*)XMLStringConverter("DSC_A2DSensor"));
  }else {
    elem->setAttribute((const XMLCh*)XMLStringConverter("IDREF"),
                       (const XMLCh*)XMLStringConverter(sensorIdName));
  }
  elem->setAttribute((const XMLCh*)XMLStringConverter("devicename"),
                     (const XMLCh*)XMLStringConverter(device));
  elem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                     (const XMLCh*)XMLStringConverter(lcId));
  if (!sfx.empty())
    elem->setAttribute((const XMLCh*)XMLStringConverter("suffix"),
                       (const XMLCh*)XMLStringConverter(sfx));

  // If we've got an analog sensor then we need to set up a calibration file,
  // a rate, a sample and variable for it
  if (sensorIdName ==  "ANALOG_NCAR") {
    addA2DCalFile(elem, dsmNode, a2dSNFname, sensorIdName);
    addA2DRate(elem, dsmNode, a2dSNFname);
    addSampAndVar(elem, dsmNode, a2dTempSfx); // add A2DTEMP var
  }

  if (sensorIdName ==  "ANALOG_DMMAT") {
    addA2DCalFile(elem, dsmNode, a2dSNFname, sensorIdName);
    addA2DRate(elem, dsmNode, a2dSNFname);
  }

  // If we've got a PMS sensor then we need to set up it's serial number
  if (sensorIdName == "CDP" ||
      sensorIdName == "Fast2DC" ||
      sensorIdName == "S100" ||
      sensorIdName == "S200" ||
      sensorIdName == "S300" ||
      sensorIdName == "TwoDP" ||
      sensorIdName == "UHSAS") {
    addPMSParms(elem, dsmNode, pmsSN, resltn);
  }

// add sensor to nidas project

    // adapted from nidas::core::DSMConfig::fromDOMElement()
    // should be factored out of that method into a public method of DSMConfig

    DSMSensor* sensor = dsmConfig->sensorFromDOMElement(elem);
    if (sensor == NULL)
      throw InternalProcessingException("null sensor(FromDOMElement)");

    // check if this is a new DSMSensor for this DSMConfig.
    const std::list<DSMSensor*>& sensors = dsmConfig->getSensors();
    list<DSMSensor*>::const_iterator si = std::find(sensors.begin(),sensors.end(),sensor);
    if (si == sensors.end())
         dsmConfig->addSensor(sensor);
    else throw InternalProcessingException ("Found duplicate sensor unexpectedly");

    try {
        dsmConfig->validate();
        sensor->validate();

        // make sure new sensor works well with old (e.g. var names and suffix)
        Site* site = const_cast <Site *> (dsmConfig->getSite());
        site->validate();

    } catch (nidas::util::InvalidParameterException &e) {
        dsmConfig->removeSensor(sensor); // validation failed so get it out of nidas Project tree
        throw(e); // notify GUI
    }


    // add sensor to DOM
  try {
    dsmNode->appendChild(elem);
  } catch (DOMException &e) {
     dsmConfig->removeSensor(sensor); // keep nidas Project tree in sync with DOM
     throw InternalProcessingException("add sensor to dsm element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

    // update Qt model
    // XXX returns bool
  model->appendChild(dsmItem);

   printSiteNames();
}

vector <std::string> Document::getSiteNames()
{
    vector <std::string> sites;
    std::string siteName;

    for (SiteIterator si = _project->getSiteIterator(); si.hasNext(); ) {
        Site * site = si.next();
        siteName = site->getName();
        sites.push_back(siteName);
    }
    return sites;
}

void Document::printSiteNames()
{

cerr << "printSiteNames " << endl;

    for (SiteIterator si = _project->getSiteIterator(); si.hasNext(); ) {
        Site * site = si.next();

        cerr << "Site: Name = " << site->getName() << "; Number = " << site->getNumber();
        cerr << "; Suffix = " << site->getSuffix() << "; Project = " << site->getProject()->getName();
        cerr << endl;
        }
}

unsigned int Document::validateDsmInfo(Site *site, const std::string & dsmName, const std::string & dsmId)
{

  // Check that id is legit
  unsigned int iDsmId;
  if (dsmId.length() > 0) {
    istringstream ist(dsmId);
    ist >> iDsmId;
    if (ist.fail()) throw n_u::InvalidParameterException(
        string("dsm") + ": " + dsmName," id",dsmId);
  }

  // Make sure that the dsmName and dsmId are unique for this Site
  set<int> dsm_ids;
  set<string> dsm_names;
  int j;
  const list<DSMConfig*>& dsms = site->getDSMConfigs();
  list<DSMConfig*>::const_iterator it;
  for (it = dsms.begin(),j=0; it != dsms.end(); ++it, j++) {
    DSMConfig * dsm = *it;
    // The following two if clauses would be really bizzarre, but lets check anyway
    // TODO: these should actually get the DMSItem and delete it.
    //   since this situation should never arise, lets not worry for now and just delete the dsm.
    if (!dsm_ids.insert(dsm->getId()).second) {
      ostringstream ost;
      ost << dsm->getId();
      delete dsm;
      throw InternalProcessingException("Existing dsm id"+ost.str()+
              "is not unique: This should NOT happen!");
    //  throw n_u::InvalidParameterException("dsm id",
    //          ost.str(),"is not unique: This should NOT happen!");
    }
    if (!dsm_names.insert(dsm->getName()).second) {
      const string& dsmname = dsm->getName();
      delete dsm;
      throw InternalProcessingException("dsm name"+
              dsmname+"is not unique: This should NOT happen!");
      //throw n_u::InvalidParameterException("dsm name",
      //        dsmname,"is not unique: This should NOT happen!");
    }
  }

  if (!dsm_ids.insert(iDsmId).second) {
    throw n_u::InvalidParameterException("dsm id", dsmId,
          "is not unique");
  }
  if (!dsm_names.insert(dsmName).second) {
    throw n_u::InvalidParameterException("dsm name",
          dsmName,"is not unique");
  }

  return iDsmId;
}

void Document::addA2DRate(xercesc::DOMElement *sensorElem,
                          xercesc::DOMNode *dsmNode,
                          const std::string & a2dSNFname)
{
  const XMLCh * paramTagName = 0;
  XMLStringConverter xmlSamp("parameter");
  paramTagName = (const XMLCh *) xmlSamp;

  // Create a new DOM element for the param element.
  xercesc::DOMElement* paramElem = 0;
  try {
    paramElem = dsmNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         paramTagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm sample element: " +
                              (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the rate parameter node attributes
  paramElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                            (const XMLCh*)XMLStringConverter("rate"));
  paramElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                            (const XMLCh*)XMLStringConverter("500"));
  paramElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                            (const XMLCh*)XMLStringConverter("int"));

  sensorElem->appendChild(paramElem);

  return;
}

void Document::addPMSParms(xercesc::DOMElement *sensorElem,
                          xercesc::DOMNode *dsmNode,
                          const std::string & pmsSN,
                          const std::string & pmsResltn)
{
  const XMLCh * pmsSNTagName = 0;
  XMLStringConverter xmlSamp("parameter");
  pmsSNTagName = (const XMLCh *) xmlSamp;

  // Create a new DOM element for the psm SN parameter element.
  xercesc::DOMElement* pmsSNElem = 0;
  try {
    pmsSNElem = dsmNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         pmsSNTagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new PMS Ser Num element: "
                             + (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the PMS Serial Number parameter node attributes
  pmsSNElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                            (const XMLCh*)XMLStringConverter("SerialNumber"));
  pmsSNElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                            (const XMLCh*)XMLStringConverter(pmsSN));
  pmsSNElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                            (const XMLCh*)XMLStringConverter("string"));

  sensorElem->appendChild(pmsSNElem);

  // Only add RESOLUTION param if we've actually got a resolution defined
  if (pmsResltn.size() > 0) {
    const XMLCh * paramTagName = 0;
    XMLStringConverter xmlSamp("parameter");
    paramTagName = (const XMLCh *) xmlSamp;

    // Create a new DOM element for the param element.
    xercesc::DOMElement* paramElem = 0;
    try {
      paramElem = dsmNode->getOwnerDocument()->createElementNS(
           DOMable::getNamespaceURI(),
           paramTagName);
    } catch (DOMException &e) {
       cerr << "Node->getOwnerDocument()->createElementNS() threw exception\n";
       throw InternalProcessingException("dsm create new dsm sample element: "+
                              (std::string)XMLStringConverter(e.getMessage()));
    }

    // set up the rate parameter node attributes
    paramElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                              (const XMLCh*)XMLStringConverter("RESOLUTION"));
    paramElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                              (const XMLCh*)XMLStringConverter("int"));
    paramElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                              (const XMLCh*)XMLStringConverter(pmsResltn));

    sensorElem->appendChild(paramElem);
  }

  return;
}

void Document::addMissingEngCalFile(QString filename)
{
  bool inList = false;

  for (vector <QString>::iterator it = _missingEngCalFiles.begin();
          it != _missingEngCalFiles.end(); it++)
  {
    if (*it == filename) {
      inList = true;
      break;
    }
  }

  if (!inList) _missingEngCalFiles.push_back(filename);

  return;
}

void Document::addA2DCalFile(xercesc::DOMElement *sensorElem,
                          xercesc::DOMNode *dsmNode,
                          const std::string & a2dSNFname,
                          const std::string & sensorIdName)
{
  const XMLCh * calfileTagName = 0;
  XMLStringConverter xmlSamp("calfile");
  calfileTagName = (const XMLCh *) xmlSamp;

  // Create a new DOM element for the calfile element.
  xercesc::DOMElement* calfileElem = 0;
  try {
    calfileElem = dsmNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         calfileTagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm sample element: "
                             + (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the calfile node attributes
  if (sensorIdName ==  "ANALOG_DMMAT") {
  calfileElem->setAttribute((const XMLCh*)XMLStringConverter("path"),
                            (const XMLCh*)XMLStringConverter
                            ("${PROJ_DIR}/Configuration/cal_files/A2D/DMMAT"));
  }
  if (sensorIdName ==  "ANALOG_NCAR") {
  calfileElem->setAttribute((const XMLCh*)XMLStringConverter("path"),
                            (const XMLCh*)XMLStringConverter
                            ("${PROJ_DIR}/Configuration/cal_files/A2D/"));
  }
  calfileElem->setAttribute((const XMLCh*)XMLStringConverter("file"),
                            (const XMLCh*)XMLStringConverter(a2dSNFname));

  sensorElem->appendChild(calfileElem);

  return;
}

void Document::addSampAndVar(xercesc::DOMElement *sensorElem,
                             xercesc::DOMNode *dsmNode,
                             const std::string & a2dTempSfx)
{
  const XMLCh * sampTagName = 0;
  XMLStringConverter xmlSamp("sample");
  sampTagName = (const XMLCh *) xmlSamp;

  // Create a new DOM element for the sample node
  xercesc::DOMElement* sampElem = 0;
  try {
    sampElem = dsmNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         sampTagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm sample element: " +
                              (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the sample node attributes
  sampElem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                         (const XMLCh*)XMLStringConverter("1"));
  sampElem->setAttribute((const XMLCh*)XMLStringConverter("rate"),
                         (const XMLCh*)XMLStringConverter("1"));

  // The sample Element needs an A2D Temperature parameter and variable element
  xercesc::DOMElement* a2dTempParmElem = createA2DTempParmElement(dsmNode);
  xercesc::DOMElement* a2dTempVarElem = createA2DTempVarElement(dsmNode,
                                                                a2dTempSfx);

  // Now add the fully qualified sample to the sensor node
  sampElem->appendChild(a2dTempParmElem);
  sampElem->appendChild(a2dTempVarElem);
  sensorElem->appendChild(sampElem);

  return;
}

void Document::updateSampAndVar(xercesc::DOMElement *sensorElem,
                                xercesc::DOMNode *dsmNode,
                                const std::string & a2dTempSfx)
{
  const XMLCh * sampTagName = 0;
  XMLStringConverter xmlSamp("sample");
  sampTagName = (const XMLCh *) xmlSamp;

  // Create a new DOM element for the sample node
  xercesc::DOMElement* sampElem = 0;
  try {
    sampElem = dsmNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         sampTagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm sample element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the sample node attributes
  sampElem->setAttribute((const XMLCh*)XMLStringConverter("id"), (const XMLCh*)XMLStringConverter("1"));
  sampElem->setAttribute((const XMLCh*)XMLStringConverter("rate"), (const XMLCh*)XMLStringConverter("1"));

  // The sample Element needs an A2D Temperature parameter and variable element
  xercesc::DOMElement* a2dTempParmElem = createA2DTempParmElement(dsmNode);
  xercesc::DOMElement* a2dTempVarElem = createA2DTempVarElement(dsmNode, a2dTempSfx);

  // Now add the fully qualified sample to the sensor node
  sampElem->appendChild(a2dTempParmElem);
  sampElem->appendChild(a2dTempVarElem);
  sensorElem->appendChild(sampElem);

  return;
}

xercesc::DOMElement* Document::createA2DTempParmElement(xercesc::DOMNode *seniorNode)
{
  // tag for parameter is "parameter"
  const XMLCh * parmTagName = 0;
  XMLStringConverter xmlParm("parameter");
  parmTagName = (const XMLCh *) xmlParm;

  // Create a new DOM element for the variable node
  xercesc::DOMElement* parmElem = 0;
  try {
    parmElem = seniorNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "Document::createA2DTempParmElement: seniorNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new variableelement:  " +
                            (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the variable node attributes
  parmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                        (const XMLCh*)XMLStringConverter("temperature"));
  parmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                        (const XMLCh*)XMLStringConverter("true"));
  parmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                        (const XMLCh*)XMLStringConverter("bool"));

  return parmElem;
}

xercesc::DOMElement* Document::createA2DTempVarElement(xercesc::DOMNode *seniorNode, const std::string & a2dTempSfx)
{
  // tag for variable is "variable"
  const XMLCh * varTagName = 0;
  XMLStringConverter xmlVar("variable");
  varTagName = (const XMLCh *) xmlVar;

  // Create a new DOM element for the variable node
  xercesc::DOMElement* varElem = 0;
  try {
    varElem = seniorNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         varTagName);
  } catch (DOMException &e) {
     cerr << "Document::createA2DTempVarElement: seniorNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new variableelement:  " +
                            (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the variable node attributes
  varElem->setAttribute((const XMLCh*)XMLStringConverter("longname"),
                        (const XMLCh*)XMLStringConverter("A2DTemperature"));
  varElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                        (const XMLCh*)XMLStringConverter("A2DTEMP" + a2dTempSfx));
  varElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                        (const XMLCh*)XMLStringConverter("deg_C"));

  return varElem;
}

xercesc::DOMElement* Document::createDsmOutputElem(xercesc::DOMNode *siteNode)
{
  const XMLCh * outTagName = 0;
  XMLStringConverter xmlOut("output");
  outTagName = (const XMLCh *) xmlOut;

  // Create a new DOM element for the output node
  xercesc::DOMElement* outElem = 0;
  try {
    outElem = siteNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         outTagName);
  } catch (DOMException &e) {
     cerr << "siteNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm output element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the output node attributes
  outElem->setAttribute((const XMLCh*)XMLStringConverter("class"), (const XMLCh*)XMLStringConverter("RawSampleOutputStream"));

  // The Output node needs a socket node
  const XMLCh * sockTagName = 0;
  XMLStringConverter xmlSock("socket");
  sockTagName = (const XMLCh *) xmlSock;

  // Create a new DOM element for the socket node
  xercesc::DOMElement* sockElem = 0;
  try {
    sockElem = siteNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         sockTagName);
  } catch (DOMException &e) {
     cerr << "siteNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new socket element:  " + (std::string)XMLStringConverter(e.getMessage()));
  }
  sockElem->setAttribute((const XMLCh*)XMLStringConverter("type"), (const XMLCh*)XMLStringConverter("mcrequest"));

  // Create the dsm->output->socket hierarchy in preparation for inserting it into the DOM tree
  outElem->appendChild(sockElem);

  return outElem;
}

xercesc::DOMElement* Document::createDsmServOutElem(xercesc::DOMNode *siteNode)
{
  const XMLCh * outTagName = 0;
  XMLStringConverter xmlOut("output");
  outTagName = (const XMLCh *) xmlOut;

  // Create a new DOM element for the output node
  xercesc::DOMElement* outElem = 0;
  try {
    outElem = siteNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         outTagName);
  } catch (DOMException &e) {
     cerr << "siteNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm output element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the output node attributes
  outElem->setAttribute((const XMLCh*)XMLStringConverter("class"), (const XMLCh*)XMLStringConverter("RawSampleOutputStream"));

  // The Output node needs a socket node
  const XMLCh * sockTagName = 0;
  XMLStringConverter xmlSock("socket");
  sockTagName = (const XMLCh *) xmlSock;

  // Create a new DOM element for the socket node
  xercesc::DOMElement* sockElem = 0;
  try {
    sockElem = siteNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         sockTagName);
  } catch (DOMException &e) {
     cerr << "siteNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new socket element:  " + (std::string)XMLStringConverter(e.getMessage()));
  }
  sockElem->setAttribute((const XMLCh*)XMLStringConverter("port"), (const XMLCh*)XMLStringConverter("3000"));
  sockElem->setAttribute((const XMLCh*)XMLStringConverter("type"), (const XMLCh*)XMLStringConverter("server"));

  // Create the dsm->output->socket hierarchy in preparation for inserting it into the DOM tree
  outElem->appendChild(sockElem);

  return outElem;
}

unsigned int Document::validateSampleInfo(DSMSensor *sensor, const std::string & sampleId)
{

  // Check that id is legit
  unsigned int iSampleId;
  if (sampleId.length() > 0) {
    istringstream ist(sampleId);
    ist >> iSampleId;
    if (ist.fail()) throw n_u::InvalidParameterException(
        string("sample Id") + ": " + sampleId);
  }

  // Make sure that the sampleId is unique for this Site
  set<int> sample_ids;
  int j;
  SampleTagIterator it;
  for (j=0, it = sensor->getSampleTagIterator(); it.hasNext(); j++) {
    SampleTag * sample = const_cast<SampleTag*>(it.next()); // XXX cast from const
    // The following clause would be really bizzarre, but lets check anyway
    // TODO: these should actually get the VariableItems with duplicate sample id and delete them.
    //   since this situation should never arise, lets not worry for now and just delete the sample.
    if (!sample_ids.insert(sample->getId()).second) {
      ostringstream ost;
      ost << sample->getId();
      delete sample;
      throw InternalProcessingException("Existing sample id"+ost.str()+
              "is not unique: This should NOT happen!");
    }
  }

  if (!sample_ids.insert(iSampleId).second) {
    throw n_u::InvalidParameterException("sample id", sampleId,
          "is not unique");
  }

  return iSampleId;
}

xercesc::DOMElement* Document::createSampleElement(xercesc::DOMNode *sensorNode,
                         const std::string & sampleId,
                         const std::string & sampleRate,
                         const std::string & sampleFilter)
{
  // XML tagname for Samples is "sample"
  const XMLCh * tagName = 0;
  XMLStringConverter xmlSample("sample");
  tagName = (const XMLCh *) xmlSample;

  // create a new DOM element for the Sample
  xercesc::DOMElement* sampleElem = 0;
  try {
     sampleElem = sensorNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         tagName);
  } catch (DOMException &e) {
     cerr << "sensorNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("sample create new sample element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

  // setup the new Sample DOM element from user input
cerr << "  setting samp element attribs: id = " << sampleId
     << "  rate = " << sampleRate << "\n";
  sampleElem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                           (const XMLCh*)XMLStringConverter(sampleId));
  sampleElem->setAttribute((const XMLCh*)XMLStringConverter("rate"),
                           (const XMLCh*)XMLStringConverter(sampleRate));

  int sRate = atoi(sampleRate.c_str());
  int nPts = 500/sRate;
  stringstream strS;
  strS<<nPts;

  if (nPts != 1) {

    // create parameter elements for boxcar filtering
    XMLStringConverter xmlParameter("parameter");
    tagName = (const XMLCh *) xmlParameter;
    xercesc::DOMElement* paramElems[] = {0,0};
    for (int i=0; i<2; i++) {
      try {
        paramElems[i] = sensorNode->getOwnerDocument()->createElementNS(
             DOMable::getNamespaceURI(), tagName);
      } catch (DOMException &e) {
        cerr << "createElementNS() threw exception creating boxcar param\n";
        throw InternalProcessingException(
                  "Sample: create new parameter element: " +
                  (std::string)XMLStringConverter(e.getMessage()));
      }
    }

    // fill in parameter info
    paramElems[0]->setAttribute((const XMLCh*)XMLStringConverter("name"),
                                (const XMLCh*)XMLStringConverter("filter"));
    paramElems[0]->setAttribute((const XMLCh*)XMLStringConverter("value"),
                                (const XMLCh*)XMLStringConverter("boxcar"));
    paramElems[0]->setAttribute((const XMLCh*)XMLStringConverter("type"),
                                (const XMLCh*)XMLStringConverter("string"));
    paramElems[1]->setAttribute((const XMLCh*)XMLStringConverter("name"),
                                (const XMLCh*)XMLStringConverter("numpoints"));
    paramElems[1]->setAttribute((const XMLCh*)XMLStringConverter("value"),
                                (const XMLCh*)XMLStringConverter(strS.str()));
    paramElems[1]->setAttribute((const XMLCh*)XMLStringConverter("type"),
                                (const XMLCh*)XMLStringConverter("int"));

    sampleElem->appendChild(paramElems[0]);
    sampleElem->appendChild(paramElems[1]);
  }

  return sampleElem;
}

void Document::addDSM(const std::string & dsmName, const std::string & dsmId,
                         const std::string & dsmLocation)
//               throw (nidas::util::InvalidParameterException, InternalProcessingException)
{
cerr<<"entering Document::addDSM about to make call to _configWindow->getModel()"  <<"\n"
      "dsmName = "<<dsmName<<" id= "<<dsmId<<" location= " <<dsmLocation<<"\n"
      "configwindow address = "<< _configWindow <<"\n";
  NidasModel *model = _configWindow->getModel();
  SiteItem * siteItem = dynamic_cast<SiteItem*>(model->getCurrentRootItem());
  if (!siteItem)
    throw InternalProcessingException("Current root index is not a Site.");

  Site *site = siteItem->getSite();
  if (!site)
    throw InternalProcessingException("null Site");

  unsigned int iDsmId;
  iDsmId = validateDsmInfo(site, dsmName,dsmId);

// get the DOM node for this Site
  xercesc::DOMNode *siteNode = siteItem->getDOMNode();
  if (!siteNode) {
    throw InternalProcessingException("null site DOM node");
  }
  cerr << "past getSiteNode()\n";

// XML tagname for DSMs is "dsm"
  const XMLCh * tagName = 0;
  XMLStringConverter xmlDSM("dsm");
  tagName = (const XMLCh *) xmlDSM;

    // create a new DOM element for the DSM
  xercesc::DOMElement* dsmElem = 0;
  try {
     dsmElem = siteNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         tagName);
  } catch (DOMException &e) {
     cerr << "siteNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

  // setup the new DSM DOM element from user input
  //  TODO: are the three "fixed" attributes ok?  e.g. derivedData only needed for certain sensors.
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                        (const XMLCh*)XMLStringConverter(dsmName));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                        (const XMLCh*)XMLStringConverter(dsmId));
  if (!dsmLocation.empty()) dsmElem->setAttribute((const XMLCh*)XMLStringConverter("location"),
                                                  (const XMLCh*)XMLStringConverter(dsmLocation));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("rserialPort"),
                        (const XMLCh*)XMLStringConverter("30002"));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("statusAddr"),
                        (const XMLCh*)XMLStringConverter("sock::30001"));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("derivedData"),
                        (const XMLCh*)XMLStringConverter("sock::7071"));

  // The DSM needs an IRIG card sensor type
  const XMLCh * sensorTagName = 0;
  XMLStringConverter xmlSensor("sensor");
  sensorTagName =  (const XMLCh *) xmlSensor;

  // Create a new DOM element for the sensor node
  xercesc::DOMElement* sensorElem = 0;
  try {
    sensorElem = siteNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         sensorTagName);
  } catch (DOMException &e) {
     cerr << "siteNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm sensor element: " +
                                      (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the sensor node attributes
  sensorElem->setAttribute((const XMLCh*)XMLStringConverter("IDREF"),
                           (const XMLCh*)XMLStringConverter("IRIG"));
  sensorElem->setAttribute((const XMLCh*)XMLStringConverter("devicename"),
                           (const XMLCh*)XMLStringConverter("/dev/irig0"));
  sensorElem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                           (const XMLCh*)XMLStringConverter("100"));

  string suffix = dsmName;
  size_t found = suffix.find("dsm");
  if (found != string::npos)
    suffix.replace(found,3,"");
  suffix.insert(0,"_");
  sensorElem->setAttribute((const XMLCh*)XMLStringConverter("suffix"),
                           (const XMLCh*)XMLStringConverter(suffix));

  dsmElem->appendChild(sensorElem);
cerr<< "appended sensor element to dsmElem \n";

  // The DSM node needs an output node w/mcrequest
  xercesc::DOMElement* outElem = createDsmOutputElem(siteNode);
  dsmElem->appendChild(outElem);

  // The DSM node needs an output node w/server port
  // HMM - not sure where I came up with that idea - seems this is
  // type of output is only needed on rare occasion
  //  better to stick with just mcrequest type outputs
  //xercesc::DOMElement* servOutElem = createDsmServOutElem(siteNode);
  //dsmElem->appendChild(servOutElem);

// add dsm to nidas project

    // adapted from nidas::core::Site::fromDOMElement()
    // should be factored out of that method into a public method of Site

    DSMConfig* dsm = new DSMConfig();
    dsm->setSite(site);
    dsm->setName(dsmName);
//    dsm_sample_id_t nidasId;
//    nidasId = convert from string to dsm_sample_id_t;k
//    dsm->setId(nidasId);
    dsm->setId(iDsmId);
    dsm->setLocation(dsmLocation);
    try {
                dsm->fromDOMElement((xercesc::DOMElement*)dsmElem);
    }
    catch(const n_u::InvalidParameterException& e) {
        delete dsm;
        throw;
    }

    site->addDSMConfig(dsm);

cerr<<"added DSMConfig to the site\n";

//    DSMSensor* sensor = dsm->sensorFromDOMElement(dsmElem);
//    if (sensor == NULL)
//      throw InternalProcessingException("null sensor(FromDOMElement)");


    // add dsm to DOM
  try {
    siteNode->appendChild(dsmElem);
  } catch (DOMException &e) {
     site->removeDSMConfig(dsm);  // keep nidas Project tree in sync with DOM
     throw InternalProcessingException("add dsm to site element: " + (std::string)XMLStringConverter(e.getMessage()));
  }

cerr<<"added dsm node to the DOM\n";

    // update Qt model
    // XXX returns bool
  model->appendChild(siteItem);

//   printSiteNames();
}

void Document::updateDSM(const std::string & dsmName,
                         const std::string & dsmId,
                         const std::string & dsmLocation,
                         QModelIndexList indexList)
{
cerr<<"entering Document::updateDSM\n";

  // Gather together all the elements we'll need to update the Sensor
  // in both the document object model (DOM) and the Nidas Model
  NidasModel *model = _configWindow->getModel();

  NidasItem *item = 0;
  if (indexList.size() > 0)  {
    for (int i=0; i<indexList.size(); i++) {
      QModelIndex index = indexList[i];
      // the NidasItem for the selected row resides in column 0
      if (index.column() != 0) continue;
      if (!index.isValid()) continue;
      item = model->getItem(index);
    }
  }

  if (!item) throw InternalProcessingException("null DSMConfig");
  DSMItem* dsmItem = dynamic_cast<DSMItem*>(item);
  if (!dsmItem) throw InternalProcessingException("DSM Item not selected.");

  // Get the DSM and save all the current values, then update
  // to the new values
  DSMConfig* dsm = dsmItem->getDSMConfig();
  std::string currDSMName = dsm->getName();
  dsm_sample_id_t currDSMId = dsm->getId();
  std::string currLocation = dsm->getLocation();

  // Now update the DSM DOM
  updateDSMDOM(dsmItem, dsmName, dsmId, dsmLocation);

  // Now we need to validate that all is right with the updated dsm
  // in the nidas world and if not, change it all back.
  try {
cerr<<" Getting and validating site.\n";
    dsmItem->fromDOM();
    Site* site = const_cast<Site *>(dsmItem->getDSMConfig()->getSite());
    site->validate();
  } catch (nidas::util::InvalidParameterException &e) {
    stringstream strS;
    strS<<currDSMId;
    updateDSMDOM(dsmItem, currDSMName, strS.str(), currLocation);
    dsmItem->fromDOM();
    throw(e); // notify GUI
  } catch (InternalProcessingException const &) {
    stringstream strS;
    strS<<currDSMId;
    this->updateDSMDOM(dsmItem, currDSMName, strS.str(), currLocation);
    dsmItem->fromDOM();
    throw; // notify GUI
  }
}

void Document::updateDSMDOM(DSMItem* dsmItem,
                            const std::string & dsmName,
                            const std::string & dsmId,
                            const std::string & dsmLocation)
{
  // get DOM node for this DSM
  xercesc::DOMNode *dsmNode = dsmItem->getDOMNode();
  if (!dsmNode)
    throw InternalProcessingException("null DSM DOM node!");

  // get the DOM Element for this DSM
  if (dsmNode->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)
    throw InternalProcessingException("DSM DOM Node is not an Element Node!");
  xercesc::DOMElement* dsmElem = ((xercesc::DOMElement*) dsmNode);

  // insert new values into the DOM element

  dsmElem->removeAttribute((const XMLCh*)XMLStringConverter("name"));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                        (const XMLCh*)XMLStringConverter(dsmName));
  dsmElem->removeAttribute((const XMLCh*)XMLStringConverter("id"));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("id"),
                        (const XMLCh*)XMLStringConverter(dsmId));
  dsmElem->removeAttribute((const XMLCh*)XMLStringConverter("location"));
  dsmElem->setAttribute((const XMLCh*)XMLStringConverter("location"),
                        (const XMLCh*)XMLStringConverter(dsmLocation));
cerr<<"updateDSMDOM - name:" << dsmName << " ID:"<<dsmId<<" loc:"<<dsmLocation<< "\n";

  return;
}

unsigned int Document::getNextSensorId()
{
cerr<< "in getNextSensorId" << endl;
  unsigned int maxSensorId = 0;

  NidasModel *model = _configWindow->getModel();

  DSMItem * dsmItem = dynamic_cast<DSMItem*>(model->getCurrentRootItem());
  if (!dsmItem) {
    throw InternalProcessingException("Current root index is not a DSM.");
    cerr<<" dsmItem = NULL" << endl;
    return 0;
    }
cerr<< "after call to model->getCurrentRootItem" << endl;

  //DSMConfig *dsmConfig = (DSMConfig *) dsmItem;
  DSMConfig *dsmConfig = dsmItem->getDSMConfig();
  if (dsmConfig == NULL) {
    cerr << "dsmConfig is null!" <<endl;
    return 0;
    }

cerr << "dsmConfig name : " << dsmConfig->getName() << endl;
  const std::list<DSMSensor*>& sensors = dsmConfig->getSensors();
cerr<< "after call to dsmConfig->getSensors" << endl;

  for (list<DSMSensor*>::const_iterator si = sensors.begin();si != sensors.end(); si++)
  {
if (*si == 0) cerr << "si is zero" << endl;
cerr<<" si is: " << (*si)->getName() << endl;
    if ((*si)->getSensorId() > maxSensorId) maxSensorId = (*si)->getSensorId();
cerr<< "after call to getSensorId" << endl;
  }

    maxSensorId += 200;
cerr<< "returning maxSensorId " << maxSensorId << endl;
    return maxSensorId;
}

std::list <int> Document::getAvailableA2DChannels()
{
cerr<< "in getAvailableA2DChannels" << endl;
  std::list<int> availableChannels;
  for (int i = 0; i < 8; i++) availableChannels.push_back(i);
  std::list<int>::iterator aci;

  NidasModel *model = _configWindow->getModel();

  SensorItem * sensorItem = dynamic_cast<SensorItem*>(model->getCurrentRootItem());
  if (!sensorItem) {
    // New ANALOG_DMMAT sensors are added without any default variables
    // necessitating the ability to add a variable while a DMMAT sensor is
    // selected. Check for that case here.
    DSMItem * dsmItem = dynamic_cast<DSMItem*>(model->getCurrentRootItem());
    if (dsmItem) { // parent is a DSM item
        // Get selected indices and make sure it's only one
        QTableView *tableview = _configWindow->getTableView();
        QModelIndexList indexList = tableview->selectionModel()->selectedIndexes();
        for (int i=0; i<indexList.size(); i++) {
            QModelIndex index = indexList[i];
            // the NidasItem for the selected row resides in column 0
            if (index.column() !=0) continue;
            NidasItem *item = model->getItem(index);
            SensorItem* _Item = dynamic_cast<SensorItem*>(item);
            if (_Item)
              cerr << _Item->devicename() << " is a sensor item\n";
        }
        // get child of DSM Item and set as currentRootIndex
        model->setCurrentRootIndex(indexList[0]);
        sensorItem = dynamic_cast<SensorItem*>(model->getCurrentRootItem());
        if (!sensorItem) {
            throw InternalProcessingException("Parent of VariableItem is not a SensorItem!");
        } else {
            cerr << "Current root item " << sensorItem->devicename();
            cerr << " is a sensor item\n";
        }
    } else {
      throw InternalProcessingException("Parent of VariableItem is not a SensorItem!");
    }
    return availableChannels;
  }

  DSMSensor *sensor = sensorItem->getDSMSensor();
  if (sensor == NULL) {
    cerr << "dsmSensor is null!\n";
    return availableChannels;
  }

  for (SampleTagIterator sti = sensor->getSampleTagIterator(); sti.hasNext(); ) {
    const SampleTag* tag = sti.next();
    for (VariableIterator vi = tag->getVariableIterator(); vi.hasNext(); )
    {
      const Variable* var = vi.next();
      int curChan = var->getA2dChannel();
      for (aci = availableChannels.begin(); aci!= availableChannels.end(); ++aci)
        if (*aci == curChan) {aci=availableChannels.erase(aci);break;}
    }
  }

  cerr << "Available channels are: ";
  for (aci = availableChannels.begin(); aci!= availableChannels.end(); ++aci)
    cerr<< " " << *aci;
  cerr << "\n";

    return availableChannels;
}

/*
 * Provide a DSM ID that would seem to make sense.  Assume that it is not a
 * wing based DSM (by convention id's start with _MIN_WING_DSM_ID for their
 * nidas ID) and that it needs the "next" number available that is not
 * (i.e. is less than) a wing dsm id.
 *
 */
unsigned int Document::getNextDSMId()
{
cerr<< "in getNextDSMId" << endl;
  unsigned int maxDSMId = 0;

  NidasModel *model = _configWindow->getModel();

  SiteItem * siteItem = dynamic_cast<SiteItem*>(model->getCurrentRootItem());
  if (!siteItem) {
    throw InternalProcessingException("Current root index is not a Site.");
    cerr<<" siteItem = NULL" << endl;
    return 0;
    }
cerr<< "after call to model->getCurrentRootItem" << endl;

  //DSMConfig *dsmConfig = (DSMConfig *) dsmItem;
  Site *site = siteItem->getSite();
  if (site == NULL) {
    cerr << "Site is null!" <<endl;
    return 0;
    }

cerr << "Site name : " << site->getName() << endl;
  const std::list<DSMConfig*>& dsmConfigs = site->getDSMConfigs();
cerr<< "after call to site->getDSMConfigs" << endl;

  for (list<DSMConfig*>::const_iterator di = dsmConfigs.begin();di != dsmConfigs.end(); di++)
  {
if (*di == 0) cerr << "di is zero" << endl;
cerr<<" di is: " << (*di)->getName() << endl;
    if (((*di)->getId() > maxDSMId) && ((*di)->getId() < _MIN_WING_DSM_ID))
      maxDSMId = (*di)->getId();
cerr<< "after call to getDSMId" << endl;
  }

    maxDSMId += 1;
cerr<< "returning maxDSMId" << maxDSMId << endl;
    return maxDSMId;
}

void Document::updateVariable(VariableItem * varItem,
                              const std::string & varName,
                              const std::string & varLongName,
                              const std::string & varSR,
                              const std::string & varUnits,
                              vector <std::string> cals,
                              bool useCalfile)
{
  cerr<<"Document::updateVariable\n";
  NidasModel *model = _configWindow->getModel();
  SensorItem * sensorItem = dynamic_cast<SensorItem*>
                                        (model->getCurrentRootItem());
  if (!sensorItem)
    throw InternalProcessingException("Current root index is not a SensorItem");

  DOMNode *sensorDOMNode = sensorItem->getDOMNode();
  DSMSensor* sensor;
  sensor = dynamic_cast<DSMSensor*>(sensorItem->getDSMSensor());
  if (!sensor)
    throw InternalProcessingException
                    ("Current root nidasItem  is not a DSMSensor.");
  cerr << "  got sensor item \n";

  // We need to make a copy of the whole of the sample
  // element either from the catalog or from the previous instantiation
  // of the sample element in the sensor element.  So see if it's from
  // a previous instantiation within the sensor element and if so, copy that
  // instantiation and update the variable accordingly.  If not then it must be
  // an IDREF from the sensorcatalog, get a copy of that sample element
  // modify its variable and put it in place
  // otherwise then something's really wrong!

  DOMNode *sampleNode = 0;
  DOMNode *origSampleNode = 0;
  DOMElement *origSampleElem = 0;
  DOMNode *newSampleNode = 0;
  DOMElement *newSampleElem = 0;
  DOMNode * variableNode = 0;

  cerr<<" Testing varItem - name = "<<varItem->name().toStdString()<<"\n";
  cerr<<"  about to findSampleDOMNode for sampleID: "
      << varItem->getSampleId() << "\n";
  sampleNode = sensorItem->findSampleDOMNode(varItem->getSampleId());
  if (sampleNode != 0) {  // sample is defined in <sensor>
    cerr<<"  found sample node defined in <site> xml - looking for var\n";

    newSampleNode = sampleNode->cloneNode(true); // complete copy
    origSampleNode = sampleNode;

  } else {  // Need to get newSampleNode from the catalog

    cerr<<"  sample node not defined in <site> xml, looking in catalog.\n";

    QString siBName = sensorItem->getBaseName();

    if(!_project->getSensorCatalog())
       throw InternalProcessingException(
                 "Document::updateVariable - can't find sensor catalog!");

    map<string,xercesc::DOMElement*>::const_iterator mi;
    const map<std::string,xercesc::DOMElement*>& scMap =
                                    _project->getSensorCatalog()->getMap();

    // Find the sensor in the sensor catalog
    for (mi = scMap.begin(); mi != scMap.end(); mi++) {
      if (mi->first == siBName.toStdString()) {
        cerr << "  found sensor node in catalog\n";

        // find Sample node in the sensor node from the catalog
        DOMNodeList * sampleNodes = mi->second->getChildNodes();
        for (XMLSize_t i = 0; i < sampleNodes->getLength(); i++) {
          DOMNode * sensorChild = sampleNodes->item(i);
          if ( ((string)XMLStringConverter(sensorChild->getNodeName())).
               find("sample")== string::npos ) continue;  // not a sample item

          XDOMElement xnode((DOMElement *)sampleNodes->item(i));
          const string& sSampleId = xnode.getAttributeValue("id");
          // We need to interpret sample id as does sampletag - i.e. accounting
          // for octal and hexidecimal numbers
          istringstream ist(sSampleId);
          unsigned int val;
          ist.unsetf(ios::dec);
          ist >> val;
          if (ist.fail())
            throw InternalProcessingException
		   ("Document::updateVariable - can't interpret sample id");

          if (val == varItem->getSampleId()) {
            cerr<<"  about to clone the sample node in the catalog\n";
            sampleNode = sampleNodes->item(i);
            newSampleNode = sampleNodes->item(i)->cloneNode(true);
            cerr<<"   cloned the node\n";
            break;
          }
        }
      }
    }
  }
  origSampleElem = ((xercesc::DOMElement*) sampleNode);

  if (!newSampleNode) {
    std::cerr << "  clone of sample node appears to have failed!\n";
    throw InternalProcessingException(
       "Document::updateVariable - clone of sample node seems to have failed");
  }

  // set the sample rate based on user input
  newSampleElem = (DOMElement*) newSampleNode;
  newSampleElem->removeAttribute((const XMLCh*)XMLStringConverter("rate"));
  newSampleElem->setAttribute((const XMLCh*)XMLStringConverter("rate"),
                              (const XMLCh*)XMLStringConverter(varSR));

  // Find the variable in the copy of the samplenode
  DOMNodeList * variableNodes = newSampleNode->getChildNodes();
  if (variableNodes == 0) {
    std::cerr << "  found no children nodes in copied sample node \n";
    std::string err = "Document::updateVariable:\n";
    err.append(" - sample node has no children - something is very wrong!\n");
    throw InternalProcessingException(err);
  }
  std::string variableName = varItem->getBaseName();
  cerr<< "after call to varItem->getBaseName - name = " ;
  cerr<< variableName <<"\n";
  for (XMLSize_t i = 0; i < variableNodes->getLength(); i++)
  {
     DOMNode * sampleChild = variableNodes->item(i);
     if (((string)XMLStringConverter(sampleChild->getNodeName())).
            find("variable") == string::npos ) continue;

     XDOMElement xnode((DOMElement *)variableNodes->item(i));
     const std::string& sVariableName = xnode.getAttributeValue("name");
     if (sVariableName.c_str() == variableName) {
       variableNode = variableNodes->item(i);
       cerr << "  Found variable node in sample copy!\n";
       break;
     }
  }

  // Look through variable node children, find and eliminate calibrations
  DOMNodeList * varChildNodes = variableNode->getChildNodes();
  DOMNode * varCalChild = 0;
  for (XMLSize_t i = 0; i < varChildNodes->getLength(); i++)
  {
    DOMNode * varChild = varChildNodes->item(i);
    if (((string)XMLStringConverter(varChild->getNodeName())).find("poly")
        == string::npos &&
        ((string)XMLStringConverter(varChild->getNodeName())).find("linear")
        == string::npos)
      continue;

    cerr << "  Found a calibration node - setting up to remove it\n";
    varCalChild = varChild;
  }

  // found a poly or linear node - remove it
  if (varCalChild) {
    cerr<< "Found a calibration node for: "<<variableName<<"\n";
    cerr<< "    - Removing it\n";
    variableNode->removeChild(varCalChild);
    //  DOMNode * rmVarChild = variableNode->removeChild(varCalChild);
  }

  // Update values of variablenode in samplenode copy based on user input
  DOMElement * varElem = ((xercesc::DOMElement*) variableNode);
  varElem->removeAttribute((const XMLCh*)XMLStringConverter("name"));
  varElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                        (const XMLCh*)XMLStringConverter(varName));
  varElem->removeAttribute((const XMLCh*)XMLStringConverter("longname"));
  varElem->setAttribute((const XMLCh*)XMLStringConverter("longname"),
                        (const XMLCh*)XMLStringConverter(varLongName));
  varElem->removeAttribute((const XMLCh*)XMLStringConverter("units"));
  varElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                        (const XMLCh*)XMLStringConverter(varUnits));
  cerr<< " updated variable copy\n";

  // Now add calibration info if it has been specified.
  if (cals.size() && useCalfile) {
    addCalibElem(cals, varUnits, sampleNode, varElem);
    cerr<<" added XML calibration info to variable copy\n";
  } else if (useCalfile) {
    Site* site = const_cast<Site *> (sensor->getSite());
    std::string siteName = site->getName();
    addVarCalFileElem(varName + string(".dat"), varUnits, siteName,
                      sampleNode, varElem);
    cerr<<" added calfile calibration info to variable copy\n";
  }

  // Check for SampleTag having problems with new XML
  SampleTag* origSampTag = varItem->getSampleTag();
  try {
    origSampTag->fromDOMElement((xercesc::DOMElement*)newSampleElem);
  }
    catch(const n_u::InvalidParameterException& e) {
    origSampTag->fromDOMElement((xercesc::DOMElement*)origSampleElem);
    throw e;
  }

  // If original sample was defined in the xml, remove it
  if (origSampleNode) sensorDOMNode->removeChild(origSampleNode);

  // add sample to Sensor DOM - get the varItem parent which should be the
  // SensorItem then get it's Dom
  try {
    sensorDOMNode->appendChild(newSampleNode);
    DOMElement *sensorElem = (DOMElement*) sensorDOMNode;
    sensor->fromDOMElement((DOMElement*)sensorElem);
  } catch (DOMException &e) {
     origSampTag->fromDOMElement((xercesc::DOMElement*)origSampleElem);
     throw InternalProcessingException("add var to sensor element: " +
                     (std::string)XMLStringConverter(e.getMessage()));
  }

  // Make sure all is right with the sample and variable
  // Note - this will require getting the site and doing a validate variables.
  try {
    Site* site = const_cast <Site *> (sensor->getSite());
    cerr << "  doing site validation\n";
    site->validate();

  } catch (nidas::util::InvalidParameterException &e) {
    cerr << "Caught invalidparameter exception\n";
    // Return DOM to prior state
    sensorDOMNode->removeChild(newSampleNode);
    if (origSampleNode) sensorDOMNode->appendChild(origSampleNode);
    origSampTag->fromDOMElement((xercesc::DOMElement*)origSampleElem);
    throw(e); // notify GUI
  } catch ( ... ) {
    cerr << "Caught unexpected error\n";
    // Return DOM to prior state
    sensorDOMNode->removeChild(newSampleNode);
    if (origSampleNode) sensorDOMNode->appendChild(origSampleNode);
    origSampTag->fromDOMElement((xercesc::DOMElement*)origSampleElem);
    throw InternalProcessingException("Caught unexpected error trying to add A2D Variable to model.");
  }

  cerr<<"added sample node to the DOM - now updating varItem\n";
  varItem->clearVarItem();
  sensorItem->fromDOM();
  varItem->fromDOM();

    // update Qt model
    // XXX returns bool

  //model->appendChild(sensorItem);

  return;
}

void Document::addA2DVariable(const std::string & a2dVarNamePfx,
                              const std::string & a2dVarNameSfx,
                              const std::string & a2dVarLongName,
                              const std::string & a2dVarVolts,
                              const std::string & a2dVarChannel,
                              const std::string & a2dVarSR,
                              const std::string & a2dVarUnits,
                              vector <std::string> cals)
{
cerr<<"in Document::addA2DVariable about to call _configWindow->getModel()"  <<"\n";
  NidasModel *model = _configWindow->getModel();
cerr<<"got model \n";
  SensorItem * sensorItem = dynamic_cast<SensorItem*>(model->getCurrentRootItem());
  if (!sensorItem)
    throw InternalProcessingException("Current root index is not an A2D SensorItem.");

  // Use devicename() to determine which type of analog sensor we are dealing with
  if (sensorItem->devicename() == "/dev/ncar_a2d0") { // ANALOG_NCAR
    addNCARVariable(a2dVarNamePfx, a2dVarNameSfx, a2dVarLongName, a2dVarVolts,
                    a2dVarChannel, a2dVarSR, a2dVarUnits, cals);
  } else if (sensorItem->devicename() == "/dev/dmmat_a2d0") { // ANALOG_DMMAT
    addDSCVariable(a2dVarNamePfx, a2dVarNameSfx, a2dVarLongName, a2dVarVolts,
                    a2dVarChannel, a2dVarSR, a2dVarUnits, cals);
  }
cerr << "Leaving Document::addA2DVariable\n";

  return;
}

void Document::addNCARVariable(const std::string & a2dVarNamePfx,
                              const std::string & a2dVarNameSfx,
                              const std::string & a2dVarLongName,
                              const std::string & a2dVarVolts,
                              const std::string & a2dVarChannel,
                              const std::string & a2dVarSR,
                              const std::string & a2dVarUnits,
                              vector <std::string> cals)
{
cerr<<"entering Document::addA2DVariable about to make call to _configWindow->getModel()"  <<"\n";
  NidasModel *model = _configWindow->getModel();
cerr<<"got model \n";
  SensorItem * sensorItem = dynamic_cast<SensorItem*>(model->getCurrentRootItem());
  if (!sensorItem)
    throw InternalProcessingException("Current root index is not an A2D SensorItem.");

  DOMNode * sensorNode = sensorItem->getDOMNode();
  A2DVariableItem *a2dvItem;
  A2DVariableInfo *a2dvInfo;
  vector<A2DVariableInfo*> varInfoList;
  vector<A2DVariableInfo*> varInfoList2;

//
// Next we add the variable described above to the vector (inserting
// ordered based on channel number)
  a2dvInfo = new A2DVariableInfo;
  a2dvInfo->a2dVarNamePfx = a2dVarNamePfx;
  a2dvInfo->a2dVarNameSfx = a2dVarNameSfx;
  a2dvInfo->a2dVarLongName = a2dVarLongName;
  a2dvInfo->a2dVarVolts = a2dVarVolts;
  a2dvInfo->a2dVarChannel = a2dVarChannel;
  a2dvInfo->a2dVarSR = a2dVarSR;
  a2dvInfo->a2dVarUnits = a2dVarUnits;
  a2dvInfo->cals = cals;
  // Because of the way nidas stores a2dVarUnits at the end of the cals
  a2dvInfo->cals.push_back(a2dVarUnits);
  varInfoList.push_back(a2dvInfo);
cerr << "put together struct for new variable and put it in the list\n";

  QModelIndexList qmIdxList;
cerr<<"Find existing A2D Variable Items (non-A2DTemp) and add them to list:\n";
// Step through all the child elements in the sensorItem:
  for (int i = 0; i<sensorItem->childCount(); i++) {
//  Gather key elements of children
    a2dvItem = dynamic_cast<A2DVariableItem*>(sensorItem->child(i));
    if (a2dvItem) {
      a2dvInfo = new A2DVariableInfo;
    } else {
      throw InternalProcessingException("Child of A2D Sensor is not A2D Variable.");
    }
// If we've got an A2DTEMP variable we need to skip it
    if (a2dvItem->variableName().compare(0,7,"A2DTEMP") != 0) {
      a2dvInfo->a2dVarNamePfx = a2dvItem->getVarNamePfx();
      a2dvInfo->a2dVarNameSfx = a2dvItem->getVarNameSfx();
cerr<<"  - A2DvItem pfx:"<<a2dvItem->getVarNamePfx();
cerr<<"  sfx:"<<a2dvItem->getVarNameSfx()<<"\n";
      a2dvInfo->a2dVarLongName = a2dvItem->getLongName().toStdString();
      if (a2dvItem->getGain() == 1 && a2dvItem->getBipolar() == 1)
        a2dvInfo->a2dVarVolts = "-10 to 10 Volts";
      else if (a2dvItem->getGain() == 2 && a2dvItem->getBipolar() == 0)
        a2dvInfo->a2dVarVolts = "  0 to 10 Volts";
      else if (a2dvItem->getGain() == 2 && a2dvItem->getBipolar() == 1)
        a2dvInfo->a2dVarVolts = " -5 to  5 Volts";
      else if (a2dvItem->getGain() == 4 && a2dvItem->getBipolar() == 0)
        a2dvInfo->a2dVarVolts = "  0 to  5 Volts";
      else {
        throw InternalProcessingException
                      ("Unsupported Gain and Bipolar Values");
      }
      a2dvInfo->a2dVarChannel = std::to_string(a2dvItem->getA2DChannel());
      a2dvInfo->a2dVarSR = std::to_string((int) a2dvItem->getRate());
      a2dvInfo->a2dVarUnits = a2dvItem->getUnits();
      a2dvInfo->cals = a2dvItem->getCalibrationInfo();

//
//   If we put them into the vector ordered based solely on channel number
//   then call the insertA2DVariable then they will be inserted into the DOM
//   first based on SR and second based on channel number
//   Ah - but we really want a secondary sort on SR so that when a lower
//   channel number is eliminated we don't have a reshuffling of SRs and
//   by association sample numbers.

      bool inserted = false;
      vector<A2DVariableInfo*>::iterator it;

      it = varInfoList.begin();
      if (atoi(a2dvInfo->a2dVarChannel.c_str()) <
          atoi((*it)->a2dVarChannel.c_str())) {
        varInfoList.insert(it, a2dvInfo);
        inserted = true;
      }
      if (varInfoList.size() == 1) {
        varInfoList.push_back(a2dvInfo);
        inserted = true;
      }
      if (!inserted) {
        for (it = varInfoList.begin()+1; it < varInfoList.end(); it++) {
          if (atoi(a2dvInfo->a2dVarChannel.c_str()) >
              atoi((*(it-1))->a2dVarChannel.c_str()) &&
              atoi(a2dvInfo->a2dVarChannel.c_str()) <
              atoi((*it)->a2dVarChannel.c_str())) {
            varInfoList.insert(it, a2dvInfo);
            inserted = true;
            break;
          }
        }
      }
      if (!inserted) varInfoList.push_back(a2dvInfo);

//  Now remove the variable from the model
//      sensorItem->removeChild(sensorItem->child(i));
//  Now get the model index for this item and add it to the list to be removed
      qmIdxList.push_back(a2dvItem->createIndex());

    } // else we skip the A2D Temperature variable
  }

// Now perform a secondary sort based on sample rate
  varInfoList2.push_back(varInfoList[0]);
  for (size_t i = 1; i < varInfoList.size(); i++) {
    bool inserted = false;
    vector<A2DVariableInfo*>::iterator it;

    it = varInfoList2.begin();
    if (atoi(varInfoList[i]->a2dVarSR.c_str()) <
        atoi((*it)->a2dVarSR.c_str())) {
      varInfoList2.insert(it, varInfoList[i]);
      inserted = true;
    }
    if (varInfoList2.size() == 1) {
      varInfoList2.push_back(varInfoList[i]);
      inserted = true;
    }
    if (!inserted) {
      for (it = varInfoList2.begin()+1; it < varInfoList2.end(); it++) {
        if (atoi(varInfoList[i]->a2dVarSR.c_str()) >=
            atoi((*(it-1))->a2dVarSR.c_str()) &&
            atoi(varInfoList[i]->a2dVarSR.c_str()) <
            atoi((*it)->a2dVarSR.c_str())) {
          varInfoList2.insert(it, varInfoList[i]);
          inserted = true;
          break;
        }
      }
    }
    if (!inserted) varInfoList2.push_back(varInfoList[i]);
  }

// Now remove all of the indexes which should eliminate all aspects of the
// model data for each A2DVariable we've collected together.
  model->removeIndexes(qmIdxList);

//
//  Next we loop on the vector and call the following code for
//  each variable - call it insertA2DVariable
//    and include sensorItem*, sensorNode in the interface
//
  InternalProcessingException* intProcEx = 0;
  bool gotIntProcEx = false;
  nidas::util::InvalidParameterException* InvParmEx = 0;
  bool gotInvParmEx = false;
  bool gotUnspEx = false;
  for (size_t ii = 0; ii < varInfoList2.size(); ii++) {

    // cals last "value" may be a unit indication
    //    - if so, change it to null string
    if (varInfoList2[ii]->cals.size()) {
      if (!isNum(varInfoList2[ii]->cals[varInfoList2[ii]->cals.size()-1])) {
        varInfoList2[ii]->a2dVarUnits =
                       varInfoList2[ii]->cals[varInfoList2[ii]->cals.size()-1];
        varInfoList2[ii]->cals[varInfoList2[ii]->cals.size()-1] = "";
      }
    }

    try {
      insertA2DVariable(model, sensorItem, sensorNode,
                      varInfoList2[ii]->a2dVarNamePfx,
                      varInfoList2[ii]->a2dVarNameSfx,
                      varInfoList2[ii]->a2dVarLongName,
                      varInfoList2[ii]->a2dVarVolts,
                      varInfoList2[ii]->a2dVarChannel,
                      varInfoList2[ii]->a2dVarSR,
                      varInfoList2[ii]->a2dVarUnits,
                      varInfoList2[ii]->cals);
    } catch (InternalProcessingException &e) {
      gotIntProcEx = true;
      intProcEx = e.clone();
    } catch (nidas::util::InvalidParameterException &e) {
      gotInvParmEx = true;
      InvParmEx = new nidas::util::InvalidParameterException(e.toString());
    } catch (...) {
      gotUnspEx = true;
    }
  }

  if (gotIntProcEx) throw(*intProcEx);
  if (gotInvParmEx) throw(*InvParmEx);
  if (gotUnspEx) throw;

  return;
}

void Document::addDSCVariable(const std::string & a2dVarNamePfx,
                              const std::string & a2dVarNameSfx,
                              const std::string & a2dVarLongName,
                              const std::string & a2dVarVolts,
                              const std::string & a2dVarChannel,
                              const std::string & a2dVarSR,
                              const std::string & a2dVarUnits,
                              vector <std::string> cals)
{
cerr<<"entering Document::addDSCVariable about to make call to _configWindow->getModel()"  <<"\n";


  // Attempting to insert a DSC_A2D var causes two separate errors:
  // - if select add while highlighting DMMAT sensor, on save get duplicate
  //   sample id error. See commit where parent is reset to handle this case.
  //   It is likely related to this error.
  // - if highlight a variable, get out of sync error
  // Temporarily catch trying to add a DMMAT var and don't allow it.
  QMessageBox msgBox;
  QString msg("Adding a variable on a DMMAT card not implemented yet");
  msgBox.setText(msg);
  msgBox.exec();
  return;


  NidasModel *model = _configWindow->getModel();
cerr<<"got model \n";
  SensorItem * sensorItem = dynamic_cast<SensorItem*>(model->getCurrentRootItem());
  if (!sensorItem)
    throw InternalProcessingException("Current root index is not an A2D SensorItem.");

  DOMNode * sensorNode = sensorItem->getDOMNode();
  DSC_A2DVariableItem *a2dvItem;
  DSC_A2DVariableInfo *a2dvInfo;
  vector<DSC_A2DVariableInfo*> varInfoList;
  vector<DSC_A2DVariableInfo*> varInfoList2;

//
// Next we add the variable described above to the vector (inserting
// ordered based on channel number)
  a2dvInfo = new DSC_A2DVariableInfo;
  a2dvInfo->a2dVarNamePfx = a2dVarNamePfx;
  a2dvInfo->a2dVarNameSfx = a2dVarNameSfx;
  a2dvInfo->a2dVarLongName = a2dVarLongName;
  a2dvInfo->a2dVarVolts = a2dVarVolts;
  a2dvInfo->a2dVarChannel = a2dVarChannel;
  a2dvInfo->a2dVarSR = a2dVarSR;
  a2dvInfo->a2dVarUnits = a2dVarUnits;
  a2dvInfo->cals = cals;
  // Because of the way nidas stores a2dVarUnits at the end of the cals
  a2dvInfo->cals.push_back(a2dVarUnits);
  varInfoList.push_back(a2dvInfo);
cerr << "put together struct for new variable and put it in the list\n";

  QModelIndexList qmIdxList;
cerr<<"Find existing A2D Variable Items (non-A2DTemp) and add them to list:\n";
// Step through all the child elements in the sensorItem:
  for (int i = 0; i<sensorItem->childCount(); i++) {
//  Gather key elements of children
    a2dvItem = dynamic_cast<DSC_A2DVariableItem*>(sensorItem->child(i));
    if (a2dvItem) {
      a2dvInfo = new DSC_A2DVariableInfo;
    } else {
      throw InternalProcessingException("Child of A2D Sensor is not A2D Variable.");
    }
// If we've got an A2DTEMP variable we need to skip it - does not apply
// to DMMAT
//    if (a2dvItem->variableName().compare(0,7,"A2DTEMP") != 0) {
      a2dvInfo->a2dVarNamePfx = a2dvItem->getVarNamePfx();
      a2dvInfo->a2dVarNameSfx = a2dvItem->getVarNameSfx();
cerr<<"  - A2DvItem pfx:"<<a2dvItem->getVarNamePfx();
cerr<<"  sfx:"<<a2dvItem->getVarNameSfx()<<"\n";
      a2dvInfo->a2dVarLongName = a2dvItem->getLongName().toStdString();
      if (a2dvItem->getGain() == 1 && a2dvItem->getBipolar() == 1)
        a2dvInfo->a2dVarVolts = "-10 to 10 Volts";
      else if (a2dvItem->getGain() == 2 && a2dvItem->getBipolar() == 0)
        a2dvInfo->a2dVarVolts = "  0 to 10 Volts";
      else if (a2dvItem->getGain() == 2 && a2dvItem->getBipolar() == 1)
        a2dvInfo->a2dVarVolts = " -5 to  5 Volts";
      else if (a2dvItem->getGain() == 4 && a2dvItem->getBipolar() == 0)
        a2dvInfo->a2dVarVolts = "  0 to  5 Volts";
      else {
        throw InternalProcessingException
                      ("Unsupported Gain and Bipolar Values");
      }
      a2dvInfo->a2dVarChannel = std::to_string(a2dvItem->getA2DChannel());
      a2dvInfo->a2dVarSR = std::to_string((int) a2dvItem->getRate());
      a2dvInfo->a2dVarUnits = a2dvItem->getUnits();
      a2dvInfo->cals = a2dvItem->getCalibrationInfo();

//
//   If we put them into the vector ordered based solely on channel number
//   then call the insertA2DVariable then they will be inserted into the DOM
//   first based on SR and second based on channel number
//   Ah - but we really want a secondary sort on SR so that when a lower
//   channel number is eliminated we don't have a reshuffling of SRs and
//   by association sample numbers.

      bool inserted = false;
      vector<DSC_A2DVariableInfo*>::iterator it;

      it = varInfoList.begin();
      if (atoi(a2dvInfo->a2dVarChannel.c_str()) <
          atoi((*it)->a2dVarChannel.c_str())) {
        varInfoList.insert(it, a2dvInfo);
        inserted = true;
      }
      if (varInfoList.size() == 1) {
        varInfoList.push_back(a2dvInfo);
        inserted = true;
      }
      if (!inserted) {
        for (it = varInfoList.begin()+1; it < varInfoList.end(); it++) {
          if (atoi(a2dvInfo->a2dVarChannel.c_str()) >
              atoi((*(it-1))->a2dVarChannel.c_str()) &&
              atoi(a2dvInfo->a2dVarChannel.c_str()) <
              atoi((*it)->a2dVarChannel.c_str())) {
            varInfoList.insert(it, a2dvInfo);
            inserted = true;
            break;
          }
        }
      }
      if (!inserted) varInfoList.push_back(a2dvInfo);

//  Now remove the variable from the model
//      sensorItem->removeChild(sensorItem->child(i));
//  Now get the model index for this item and add it to the list to be removed
      qmIdxList.push_back(a2dvItem->createIndex());

//    } // else we skip the A2D Temperature variable
  }

// Now perform a secondary sort based on sample rate
  varInfoList2.push_back(varInfoList[0]);
  for (size_t i = 1; i < varInfoList.size(); i++) {
    bool inserted = false;
    vector<DSC_A2DVariableInfo*>::iterator it;

    it = varInfoList2.begin();
    if (atoi(varInfoList[i]->a2dVarSR.c_str()) <
        atoi((*it)->a2dVarSR.c_str())) {
      varInfoList2.insert(it, varInfoList[i]);
      inserted = true;
    }
    if (varInfoList2.size() == 1) {
      varInfoList2.push_back(varInfoList[i]);
      inserted = true;
    }
    if (!inserted) {
      for (it = varInfoList2.begin()+1; it < varInfoList2.end(); it++) {
        if (atoi(varInfoList[i]->a2dVarSR.c_str()) >=
            atoi((*(it-1))->a2dVarSR.c_str()) &&
            atoi(varInfoList[i]->a2dVarSR.c_str()) <
            atoi((*it)->a2dVarSR.c_str())) {
          varInfoList2.insert(it, varInfoList[i]);
          inserted = true;
          break;
        }
      }
    }
    if (!inserted) varInfoList2.push_back(varInfoList[i]);
  }

// Now remove all of the indexes which should eliminate all aspects of the
// model data for each DSCVariable we've collected together.
  model->removeIndexes(qmIdxList);

//
//  Next we loop on the vector and call the following code for
//  each variable - call it insertA2DVariable
//    and include sensorItem*, sensorNode in the interface
//
  InternalProcessingException* intProcEx = 0;
  bool gotIntProcEx = false;
  nidas::util::InvalidParameterException* InvParmEx = 0;
  bool gotInvParmEx = false;
  bool gotUnspEx = false;

  for (size_t ii = 0; ii < varInfoList2.size(); ii++) {

    // cals last "value" may be a unit indication
    //    - if so, change it to null string
    if (varInfoList2[ii]->cals.size()) {
      if (!isNum(varInfoList2[ii]->cals[varInfoList2[ii]->cals.size()-1])) {
        varInfoList2[ii]->a2dVarUnits =
                       varInfoList2[ii]->cals[varInfoList2[ii]->cals.size()-1];
        varInfoList2[ii]->cals[varInfoList2[ii]->cals.size()-1] = "";
      }
    }

    try {
      insertDSC_A2DVariable(model, sensorItem, sensorNode,
                      varInfoList2[ii]->a2dVarNamePfx,
                      varInfoList2[ii]->a2dVarNameSfx,
                      varInfoList2[ii]->a2dVarLongName,
                      varInfoList2[ii]->a2dVarVolts,
                      varInfoList2[ii]->a2dVarChannel,
                      varInfoList2[ii]->a2dVarSR,
                      varInfoList2[ii]->a2dVarUnits,
                      varInfoList2[ii]->cals);
    } catch (InternalProcessingException &e) {
      gotIntProcEx = true;
      intProcEx = e.clone();
    } catch (nidas::util::InvalidParameterException &e) {
      gotInvParmEx = true;
      InvParmEx = new nidas::util::InvalidParameterException(e.toString());
    } catch (...) {
      gotUnspEx = true;
    }
  }

  if (gotIntProcEx) throw(*intProcEx);
  if (gotInvParmEx) throw(*InvParmEx);
  if (gotUnspEx) throw;

cerr << "Leaving Document::addDSCVariable\n";
  return;
}

bool Document::isNum(std::string str)
{
  // Determine if a string is numeric
  std::istringstream strStream(str);
  double inpValue = 0.0;
  if (strStream >> inpValue)
    return true;
  else
    return false;
}

void Document::insertA2DVariable(NidasModel            *model,
                                 SensorItem            *sensorItem,
                                 DOMNode               *sensorNode,
                                 const std::string     &a2dVarNamePfx,
                                 const std::string     &a2dVarNameSfx,
                                 const std::string     &a2dVarLongName,
                                 const std::string     &a2dVarVolts,
                                 const std::string     &a2dVarChannel,
                                 const std::string     &a2dVarSR,
                                 const std::string     &a2dVarUnits,
                                 vector <std::string>  cals)
{
cerr << "got A2Dsensor item \n";

DSMAnalogSensor* analogSensor;
analogSensor = dynamic_cast<DSMAnalogSensor*>(sensorItem->getDSMSensor());
if (!analogSensor)
  throw InternalProcessingException("Current root nidas element is not an AnalogSensor.");


std::cerr<<"insertA2DVariable: \n   VarPfx:"<< a2dVarNamePfx<<"  ";
std::cerr<<"VarSfx: "<<a2dVarNameSfx<<" \n  Units:"<<a2dVarUnits<<"\n   Cals:";
for (size_t i=0; i<cals.size(); i++) std::cerr<<cals[i];
std::cerr<<"\n";

// Find or create the SampleTag that will house this variable
  SampleTag *sampleTag2Add2=0;
  unsigned int iSampRate;
  if (a2dVarSR.length() > 0) {
    istringstream ist(a2dVarSR);
    ist >> iSampRate;
    if (ist.fail()) throw n_u::InvalidParameterException(
        string("sample rate:") + a2dVarSR);
  }
  set<unsigned int> sampleIds;

// We want a sampleTag with the same sample rate as requested, but if the
// SampleTag found is A2D temperature, we don't want it.
  for (int i=0; i< sensorItem->childCount(); i++) {
    A2DVariableItem* variableItem =
            dynamic_cast<A2DVariableItem*>(sensorItem->child(i));
    if (!variableItem)
      throw InternalProcessingException
            ("Found child of A2DSensorItem that's not an A2DVariableItem!");
    SampleTag* sampleTag = variableItem->getSampleTag();
    sampleIds.insert(sampleTag->getSampleId());
    if (sampleTag->getRate() == iSampRate)
      if  ( !sampleTag->getParameter("temperature")) sampleTag2Add2 = sampleTag;
  }

set<unsigned int>::iterator it;
cerr << "Sample IDs found: ";
for (it=sampleIds.begin(); it!=sampleIds.end(); it++)
    cerr << " " << *it;
cerr << "\n";

  bool createdNewSamp = false;
  xercesc::DOMNode *sampleNode = 0;
  if (!sampleTag2Add2) {
    // We need a unique sample Id
    unsigned int sampleId=0;
    for (unsigned int i = 1; i<99; i++) {
      pair<set<unsigned int>::iterator,bool> ret;
      ret = sampleIds.insert(i);
      if (ret.second == true) {
        sampleId=i;
        break;
      }
    }
    char sSampleId[10];
    sprintf(sSampleId,"%d",sampleId);
    DOMElement* newSampleElem = 0;

    newSampleElem = createSampleElement(sensorNode,
                                        string(sSampleId),a2dVarSR,string(""));

//cerr << "prior to fromdom newSampleElem = " << newSampleElem << "\n";
    createdNewSamp = true;

    try {
      sampleTag2Add2 = new SampleTag();
      //sampleTag2Add2->setSampleId(sampleId);
      //sampleTag2Add2->setRate(iSampRate);
      //DSMSensor* sensor = sensorItem->getDSMSensor();
      sampleTag2Add2->setSensorId(analogSensor->getSensorId());
      sampleTag2Add2->setDSMId(analogSensor->getDSMId());
      sampleTag2Add2->fromDOMElement((xercesc::DOMElement*)newSampleElem);
      analogSensor->addSampleTag(sampleTag2Add2);

//cerr << "after fromdom newSampleElem = " << newSampleElem << "\n";
//cerr<<"added SampleTag to the Sensor\n";

      // add sample to DOM
      try {
        sampleNode = sensorNode->appendChild(newSampleElem);
      } catch (DOMException &e) {
        analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
        throw InternalProcessingException("add sample to dsm element: " +
                                      (std::string)XMLStringConverter(e.getMessage()));
      }
    }
    catch(const n_u::InvalidParameterException& e) {
        analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
        throw;
    }

  }
//cerr << "got sampleTag\n";

//  Getting the sampleNode - if we created a newSampleElem above then we just
//  need to cast it as a DOMNode, but if not, we need to step through sample
//  nodes of this sensor until we find the one with the right ID
  if (!createdNewSamp)  {
    sampleNode = sensorItem->findSampleDOMNode(sampleTag2Add2->getSampleId());
  }

  if (!sampleNode) {
    if (createdNewSamp)  {
        analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
        sampleNode = sensorNode->removeChild(sampleNode);
    }
    throw InternalProcessingException("null sample DOM node");
  }
//cerr << "past getSampleNode()\n";

// Now add the new variable to the sample get the DOM node for this SampleTag
// XML tagname for A2DVariables is "variable"
  const XMLCh * tagName = 0;
  XMLStringConverter xmlA2DVariable("variable");
  tagName = (const XMLCh *) xmlA2DVariable;

    // create a new DOM element for the A2DVariable
  xercesc::DOMElement* a2dVarElem = 0;
  try {
     a2dVarElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         tagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new a2dVar element: " +
                              (std::string)XMLStringConverter(e.getMessage()));
  }

  // setup the new A2DVariable DOM element from user input
//cerr << "setting variable element attribs: name = " << a2dVarName << "\n";
  std::string a2dVarName = a2dVarNamePfx;
  if (a2dVarNameSfx.size() > 0) {
    a2dVarName.append("_");
    a2dVarName.append(a2dVarNameSfx);
  }
  a2dVarElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter(a2dVarName));
  a2dVarElem->setAttribute((const XMLCh*)XMLStringConverter("longname"),
                           (const XMLCh*)XMLStringConverter(a2dVarLongName));
  a2dVarElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                           (const XMLCh*)XMLStringConverter("V"));

  // Now we need parameters for channel, gain and bipolar
  const XMLCh * parmTagName = 0;
  XMLStringConverter xmlParm("parameter");
  parmTagName = (const XMLCh *) xmlParm;

  // create a new DOM element for the Channel parameter
  xercesc::DOMElement* chanParmElem = 0;
  try {
    chanParmElem  = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new channel element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }
  chanParmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter("channel"));
  chanParmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                           (const XMLCh*)XMLStringConverter("int"));
  chanParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter(a2dVarChannel));

  // create new DOM elements for the gain and bipolar parameters
  xercesc::DOMElement* gainParmElem = 0;
  try {
    gainParmElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new gain element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }
  gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter("gain"));
  gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                           (const XMLCh*)XMLStringConverter("float"));

  xercesc::DOMElement* biPolarParmElem = 0;
  try {
    biPolarParmElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new biPolar element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }
  biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter("bipolar"));
  biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                           (const XMLCh*)XMLStringConverter("bool"));

  // Now set gain and BiPolar according to the user's selection
cerr<<"a2dVarVolts = " << a2dVarVolts <<"\n";
  if (a2dVarVolts =="  0 to  5 Volts") {
    gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("4"));
    biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("false"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 4, 0);
  } else
  if (a2dVarVolts == " -5 to  5 Volts") {
      gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("2"));
      biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("true"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 2, 1);
  } else
  if (a2dVarVolts == "  0 to 10 Volts") {
    gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("2"));
    biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("false"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 2, 0);
  } else
  if (a2dVarVolts == "-10 to 10 Volts") {
    gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("1"));
    biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("true"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 1, 1);
  } else {
     if (createdNewSamp)  {
         // keep nidas Project tree in sync with DOM
         analogSensor->removeSampleTag(sampleTag2Add2);
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException
                ("Voltage choice not found in Document if/else block!");
  }

  a2dVarElem->appendChild(chanParmElem);
  a2dVarElem->appendChild(gainParmElem);
  a2dVarElem->appendChild(biPolarParmElem);

  // Now for the Calibration element.  If it was previously defined in the
  // XML, leave that alone.  Otherwise, look for a calfile for the variable
  // and if none found, go ahead and put one in.
  if (cals.size() && *cals.begin() == "XML:") {
    cals.erase(cals.begin());
    addCalibElem(cals, a2dVarUnits, sampleNode, a2dVarElem);
  } else {
    // If it's not XML defined try to find a specific Calibration file for
    // this Variable
    Site* site = const_cast<Site *> (analogSensor->getSite());
    std::string siteName = site->getName();
    QString varPfxFileName = QString::fromStdString(a2dVarNamePfx);
    varPfxFileName.append(".dat");
    QString varFileName = QString::fromStdString(a2dVarName);
    varFileName.append(".dat");
    bool foundCalFile = false;
    for (std::vector<QString>::iterator qit=_engCalFiles.begin();
         qit!=_engCalFiles.end(); qit++) {
//cerr<<(*qit).toStdString()<<"\n";
      if (!foundCalFile) {
        if ((*qit) == varFileName) {
          foundCalFile = true;
          addVarCalFileElem(a2dVarName + string(".dat"), a2dVarUnits,
                            siteName, sampleNode, a2dVarElem);
cerr<<"Found engineering cal file: "<<a2dVarName<<".dat\n";
        }
        if (*qit == varPfxFileName && !foundCalFile) {
          foundCalFile = true;
          addVarCalFileElem(a2dVarNamePfx + string(".dat"), a2dVarUnits,
                            siteName, sampleNode, a2dVarElem);
cerr<<"Found engineering cal file: "<<a2dVarNamePfx<<".dat\n";
        }
      }
    }
    if (!foundCalFile) {
      addMissingEngCalFile(QString::fromStdString(a2dVarName));
cerr<<"Found neither "<<varPfxFileName.toStdString()<<" nor "<<varFileName.toStdString()<<" in Cal Dir\n";
      addVarCalFileElem(a2dVarName + string(".dat"), a2dVarUnits, siteName,
                            sampleNode, a2dVarElem);
    }
  }

    // add a2dVar to nidas project by doing a fromDOM
    Variable* a2dVar = new Variable();
    Site* site = const_cast <Site *> (analogSensor->getSite());
    a2dVar->setSite(site);
    a2dVar->setSampleTag(sampleTag2Add2);
cerr << "Calling fromDOM \n";
    try {
                a2dVar->fromDOMElement((xercesc::DOMElement*)a2dVarElem);
    }
    catch(const n_u::InvalidParameterException& e) {
        delete a2dVar;
        if (createdNewSamp)  {
            // keep nidas Project tree in sync with DOM
            analogSensor->removeSampleTag(sampleTag2Add2);
            sampleNode = sensorNode->removeChild(sampleNode);
        }
        throw(e);
    }
//cerr << "setting a2d Channel for new variable to value"
//     << a2dVarChannel.c_str() << "\n";
    a2dVar->setA2dChannel(atoi(a2dVarChannel.c_str()));

  // Make sure nidas is OK with the new variable
  try {
//cerr << "adding variable to sample tag\n";
    sampleTag2Add2->addVariable(a2dVar);
    Site* site = const_cast <Site *> (sampleTag2Add2->getSite());
//cerr << "doing site validation\n";
    site->validate();

  } catch (nidas::util::InvalidParameterException &e) {
    cerr << "Caught invalidparameter exception\n";
    // validation failed so get it out of nidas Project tree
    sampleTag2Add2->removeVariable(a2dVar);
    if (createdNewSamp)  {
        // keep nidas Project tree in sync with DOM
        analogSensor->removeSampleTag(sampleTag2Add2);
        try {
          sampleNode = sensorNode->removeChild(sampleNode);
        }
        catch (xercesc::DOMException &e){
            cerr<<"domexeption: " <<
              (std::string)XMLStringConverter(e.getMessage()) <<"\n";
        }
    }
    //delete a2dVar;
    throw(e); // notify GUI
  } catch ( ... ) {
    // validation failed so get it out of nidas Project tree and DOM tree
    sampleTag2Add2->removeVariable(a2dVar);
    if (createdNewSamp)  {
        analogSensor->removeSampleTag(sampleTag2Add2);
        sampleNode = sensorNode->removeChild(sampleNode);
    }
    //delete a2dVar;
    throw InternalProcessingException
            ("Caught unexpected error trying to add A2D Variable to model.");
  }

  // add a2dVar to DOM
  try {
    sampleNode->appendChild(a2dVarElem);
  } catch (DOMException &e) {
     // validation failed so get it out of nidas Project tree and DOM tree
     sampleNode->removeChild(a2dVarElem);
     sampleTag2Add2->removeVariable(a2dVar);
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("add a2dVar to dsm element: " +
                     (std::string)XMLStringConverter(e.getMessage()));
  }

cerr<<"added a2dVar node to the DOM\n";

    // update Qt model
    // XXX returns bool
  model->appendChild(sensorItem);

//   printSiteNames();
}

void Document::insertDSC_A2DVariable(NidasModel            *model,
                                 SensorItem            *sensorItem,
                                 DOMNode               *sensorNode,
                                 const std::string     &a2dVarNamePfx,
                                 const std::string     &a2dVarNameSfx,
                                 const std::string     &a2dVarLongName,
                                 const std::string     &a2dVarVolts,
                                 const std::string     &a2dVarChannel,
                                 const std::string     &a2dVarSR,
                                 const std::string     &a2dVarUnits,
                                 vector <std::string>  cals)
{
cerr << "got DSC_A2Dsensor item \n";
cerr << "In Document::insertDSC_A2DVariable\n";

DSC_A2DSensor* analogSensor;
analogSensor = dynamic_cast<DSC_A2DSensor*>(sensorItem->getDSMSensor());
if (!analogSensor)
  throw InternalProcessingException("Current root nidas element is not an AnalogSensor.");


std::cerr<<"insertDSC_A2DVariable: \n   VarPfx:"<< a2dVarNamePfx<<"  ";
std::cerr<<"VarSfx: "<<a2dVarNameSfx<<" \n  Units:"<<a2dVarUnits<<"\n   Cals:";
for (size_t i=0; i<cals.size(); i++) std::cerr<<cals[i];
std::cerr<<"\n";

// Find or create the SampleTag that will house this variable
  SampleTag *sampleTag2Add2=0;
  unsigned int iSampRate;
  if (a2dVarSR.length() > 0) {
    istringstream ist(a2dVarSR);
    ist >> iSampRate;
    if (ist.fail()) throw n_u::InvalidParameterException(
        string("sample rate:") + a2dVarSR);
  }
  set<unsigned int> sampleIds;

// We want a sampleTag with the same sample rate as requested
  for (int i=0; i< sensorItem->childCount(); i++) {
    DSC_A2DVariableItem* variableItem =
            dynamic_cast<DSC_A2DVariableItem*>(sensorItem->child(i));
    if (!variableItem)
      throw InternalProcessingException
            ("Found child of DSC_A2DSensorItem that's not an DSC_A2DVariableItem!");
    SampleTag* sampleTag = variableItem->getSampleTag();
    sampleIds.insert(sampleTag->getSampleId());
    if (sampleTag->getRate() == iSampRate)
      sampleTag2Add2 = sampleTag;
  }

set<unsigned int>::iterator it;
cerr << "Sample IDs found: ";
for (it=sampleIds.begin(); it!=sampleIds.end(); it++)
    cerr << " " << *it;
cerr << "\n";

  bool createdNewSamp = false;
  xercesc::DOMNode *sampleNode = 0;
  if (!sampleTag2Add2) {
    // We need a unique sample Id
    unsigned int sampleId=0;
    for (unsigned int i = 1; i<99; i++) {
      pair<set<unsigned int>::iterator,bool> ret;
      ret = sampleIds.insert(i);
      if (ret.second == true) {
        sampleId=i;
        break;
      }
    }
    char sSampleId[10];
    sprintf(sSampleId,"%d",sampleId);
    DOMElement* newSampleElem = 0;

    newSampleElem = createSampleElement(sensorNode,
                                        string(sSampleId),a2dVarSR,string(""));

//cerr << "prior to fromdom newSampleElem = " << newSampleElem << "\n";
    createdNewSamp = true;

    try {
      sampleTag2Add2 = new SampleTag();
      //sampleTag2Add2->setSampleId(sampleId);
      //sampleTag2Add2->setRate(iSampRate);
      //DSMSensor* sensor = sensorItem->getDSMSensor();
      sampleTag2Add2->setSensorId(analogSensor->getSensorId());
      sampleTag2Add2->setDSMId(analogSensor->getDSMId());
      sampleTag2Add2->fromDOMElement((xercesc::DOMElement*)newSampleElem);
      analogSensor->addSampleTag(sampleTag2Add2);

//cerr << "after fromdom newSampleElem = " << newSampleElem << "\n";
cerr <<"added SampleTag to the Sensor\n";

      // add sample to DOM
      try {
        sampleNode = sensorNode->appendChild(newSampleElem);
      } catch (DOMException &e) {
        analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
        throw InternalProcessingException("add sample to dsm element: " +
                                      (std::string)XMLStringConverter(e.getMessage()));
      }
    }
    catch(const n_u::InvalidParameterException& e) {
        analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
        throw;
    }

  }
cerr << "got sampleTag\n";

//  Getting the sampleNode - if we created a newSampleElem above then we just
//  need to cast it as a DOMNode, but if not, we need to step through sample
//  nodes of this sensor until we find the one with the right ID
  if (!createdNewSamp)  {
    sampleNode = sensorItem->findSampleDOMNode(sampleTag2Add2->getSampleId());
  }

  if (!sampleNode) {
    if (createdNewSamp)  {
        analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
        sampleNode = sensorNode->removeChild(sampleNode);
    }
    throw InternalProcessingException("null sample DOM node");
  }
//cerr << "past getSampleNode()\n";

// Now add the new variable to the sample get the DOM node for this SampleTag
// XML tagname for DSC_A2DVariables is "variable"
  const XMLCh * tagName = 0;
  XMLStringConverter xmlA2DVariable("variable");
  tagName = (const XMLCh *) xmlA2DVariable;

    // create a new DOM element for the DSC_A2DVariable
  xercesc::DOMElement* a2dVarElem = 0;
  try {
     a2dVarElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         tagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new a2dVar element: " +
                              (std::string)XMLStringConverter(e.getMessage()));
  }

  // setup the new DSC_A2DVariable DOM element from user input
  std::string a2dVarName = a2dVarNamePfx;
cerr << "setting variable element attribs: name = " << a2dVarName << "\n";
  if (a2dVarNameSfx.size() > 0) {
    a2dVarName.append("_");
    a2dVarName.append(a2dVarNameSfx);
  }
  a2dVarElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter(a2dVarName));
  a2dVarElem->setAttribute((const XMLCh*)XMLStringConverter("longname"),
                           (const XMLCh*)XMLStringConverter(a2dVarLongName));
  a2dVarElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                           (const XMLCh*)XMLStringConverter("V"));

  // Now we need parameters for channel, gain and bipolar
  const XMLCh * parmTagName = 0;
  XMLStringConverter xmlParm("parameter");
  parmTagName = (const XMLCh *) xmlParm;

  // create a new DOM element for the Channel parameter
  xercesc::DOMElement* chanParmElem = 0;
  try {
    chanParmElem  = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new channel element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }
  chanParmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter("channel"));
  chanParmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                           (const XMLCh*)XMLStringConverter("int"));
  chanParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter(a2dVarChannel));

  // create new DOM elements for the gain and bipolar parameters
  xercesc::DOMElement* gainParmElem = 0;
  try {
    gainParmElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new gain element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }
  gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter("gain"));
  gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                           (const XMLCh*)XMLStringConverter("float"));

  xercesc::DOMElement* biPolarParmElem = 0;
  try {
    biPolarParmElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         parmTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);  // keep nidas Project tree in sync with DOM
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("a2dVar create new biPolar element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }
  biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("name"),
                           (const XMLCh*)XMLStringConverter("bipolar"));
  biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("type"),
                           (const XMLCh*)XMLStringConverter("bool"));

  // Now set gain and BiPolar according to the user's selection
cerr<<"a2dVarVolts = " << a2dVarVolts <<"\n";
  if (a2dVarVolts =="  0 to  5 Volts") {
    gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("4"));
    biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("false"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 4, 0);
  } else
  if (a2dVarVolts == " -5 to  5 Volts") {
      gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("2"));
      biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("true"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 2, 1);
  } else
  if (a2dVarVolts == "  0 to 10 Volts") {
    gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("2"));
    biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("false"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 2, 0);
  } else
  if (a2dVarVolts == "-10 to 10 Volts") {
    gainParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("1"));
    biPolarParmElem->setAttribute((const XMLCh*)XMLStringConverter("value"),
                           (const XMLCh*)XMLStringConverter("true"));
    analogSensor->setGainBipolar(atoi(a2dVarChannel.c_str()), 1, 1);
  } else {
     if (createdNewSamp)  {
         // keep nidas Project tree in sync with DOM
         analogSensor->removeSampleTag(sampleTag2Add2);
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException
                ("Voltage choice not found in Document if/else block!");
  }

  a2dVarElem->appendChild(chanParmElem);
  a2dVarElem->appendChild(gainParmElem);
  a2dVarElem->appendChild(biPolarParmElem);

  // Now for the Calibration element.  If it was previously defined in the
  // XML, leave that alone.  Otherwise, look for a calfile for the variable
  // and if none found, go ahead and put one in.
  if (cals.size() && *cals.begin() == "XML:") {
    cals.erase(cals.begin());
    addCalibElem(cals, a2dVarUnits, sampleNode, a2dVarElem);
  } else {
    // If it's not XML defined try to find a specific Calibration file for
    // this Variable
    Site* site = const_cast<Site *> (analogSensor->getSite());
    std::string siteName = site->getName();
    QString varPfxFileName = QString::fromStdString(a2dVarNamePfx);
    varPfxFileName.append(".dat");
    QString varFileName = QString::fromStdString(a2dVarName);
    varFileName.append(".dat");
    bool foundCalFile = false;
    for (std::vector<QString>::iterator qit=_engCalFiles.begin();
         qit!=_engCalFiles.end(); qit++) {
//cerr<<(*qit).toStdString()<<"\n";
      if (!foundCalFile) {
        if ((*qit) == varFileName) {
          foundCalFile = true;
          addVarCalFileElem(a2dVarName + string(".dat"), a2dVarUnits,
                            siteName, sampleNode, a2dVarElem);
cerr<<"Found engineering cal file: "<<a2dVarName<<".dat\n";
        }
        if (*qit == varPfxFileName && !foundCalFile) {
          foundCalFile = true;
          addVarCalFileElem(a2dVarNamePfx + string(".dat"), a2dVarUnits,
                            siteName, sampleNode, a2dVarElem);
cerr<<"Found engineering cal file: "<<a2dVarNamePfx<<".dat\n";
        }
      }
    }
    if (!foundCalFile) {
      addMissingEngCalFile(QString::fromStdString(a2dVarName));
cerr<<"Found neither "<<varPfxFileName.toStdString()<<" nor "<<varFileName.toStdString()<<" in Cal Dir\n";
      addVarCalFileElem(a2dVarName + string(".dat"), a2dVarUnits, siteName,
                            sampleNode, a2dVarElem);
    }
  }

    // add a2dVar to nidas project by doing a fromDOM
    Variable* a2dVar = new Variable();
    Site* site = const_cast <Site *> (analogSensor->getSite());
    a2dVar->setSite(site);
    a2dVar->setSampleTag(sampleTag2Add2);
cerr << "Calling fromDOM \n";
    try {
                a2dVar->fromDOMElement((xercesc::DOMElement*)a2dVarElem);
    }
    catch(const n_u::InvalidParameterException& e) {
        delete a2dVar;
        if (createdNewSamp)  {
            // keep nidas Project tree in sync with DOM
            analogSensor->removeSampleTag(sampleTag2Add2);
            sampleNode = sensorNode->removeChild(sampleNode);
        }
        throw(e);
    }
cerr << "setting a2d Channel for new variable to value"
     << a2dVarChannel.c_str() << "\n";
    a2dVar->setA2dChannel(atoi(a2dVarChannel.c_str()));

  // Make sure nidas is OK with the new variable
  try {
cerr << "adding variable to sample tag\n";
    sampleTag2Add2->addVariable(a2dVar);
    Site* site = const_cast <Site *> (sampleTag2Add2->getSite());
cerr << "doing site validation\n";
    site->validate();

  } catch (nidas::util::InvalidParameterException &e) {
    // validation failed so get it out of nidas Project tree
    sampleTag2Add2->removeVariable(a2dVar);
    if (createdNewSamp)  {
        // keep nidas Project tree in sync with DOM
        analogSensor->removeSampleTag(sampleTag2Add2);
        try {
          sampleNode = sensorNode->removeChild(sampleNode);
        }
        catch (xercesc::DOMException &e){
            cerr<<"domexeption: " <<
              (std::string)XMLStringConverter(e.getMessage()) <<"\n";
        }
    }
    //delete a2dVar;
    throw(e); // notify GUI
  } catch ( ... ) {
    // validation failed so get it out of nidas Project tree and DOM tree
    sampleTag2Add2->removeVariable(a2dVar);
    if (createdNewSamp)  {
        analogSensor->removeSampleTag(sampleTag2Add2);
        sampleNode = sensorNode->removeChild(sampleNode);
    }
    //delete a2dVar;
    throw InternalProcessingException
            ("Caught unexpected error trying to add DSC_A2D Variable to model.");
  }

  // add a2dVar to DOM
  try {
    sampleNode->appendChild(a2dVarElem);
  } catch (DOMException &e) {
     // validation failed so get it out of nidas Project tree and DOM tree
     sampleNode->removeChild(a2dVarElem);
     sampleTag2Add2->removeVariable(a2dVar);
     if (createdNewSamp)  {
         analogSensor->removeSampleTag(sampleTag2Add2);
         sampleNode = sensorNode->removeChild(sampleNode);
     }
     throw InternalProcessingException("add a2dVar to dsm element: " +
                     (std::string)XMLStringConverter(e.getMessage()));
  }

cerr<<"added a2dVar node to the DOM\n";

    // update Qt model
    // XXX returns bool
  model->appendChild(sensorItem);

//   printSiteNames();
cerr << "Leaving Document::insertDSC_A2DVariable\n";
}

void Document::addCalibElem(std::vector <std::string> cals,
                            const std::string & VarUnits,
                            xercesc::DOMNode *sampleNode,
                            xercesc::DOMElement *varElem)
{
  if (cals[2].size()) {  // poly cal
    // We need a poly node
    const XMLCh * polyTagName = 0;
    XMLStringConverter xmlPoly("poly");
    polyTagName = (const XMLCh *) xmlPoly;

    // create a new DOM element for the poly node
    xercesc::DOMElement* polyElem = 0;
    try {
      polyElem  = sampleNode->getOwnerDocument()->createElementNS(
           DOMable::getNamespaceURI(),
           polyTagName);
    } catch (DOMException &e) {
       cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
       throw InternalProcessingException("a2dVar create new poly calibration element: " +
                               (std::string)XMLStringConverter(e.getMessage()));
    }

    // set up the poly node attributes
    std::string polyStr = cals[0];
    for (size_t i = 1; i < cals.size(); i++)
      if (cals[i].size()) polyStr += (" " + cals[i]);

    polyElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                             (const XMLCh*)XMLStringConverter(VarUnits));
    polyElem->setAttribute((const XMLCh*)XMLStringConverter("coefs"),
                             (const XMLCh*)XMLStringConverter(polyStr));

    varElem->appendChild(polyElem);

  } else {   // slope & offset cal
    // We need a linear node
    const XMLCh * linearTagName = 0;
    XMLStringConverter xmlLinear("linear");
    linearTagName = (const XMLCh *) xmlLinear;

    // create a new DOM element for the linear node
    xercesc::DOMElement* linearElem = 0;
    try {
      linearElem = sampleNode->getOwnerDocument()->createElementNS(
           DOMable::getNamespaceURI(),
           linearTagName);
    } catch (DOMException &e) {
       cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
       throw InternalProcessingException("a2dVar create new linear calibration element: " +
                               (std::string)XMLStringConverter(e.getMessage()));
    }

    // set up the linear node attributes
    linearElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                             (const XMLCh*)XMLStringConverter(VarUnits));
    linearElem->setAttribute((const XMLCh*)XMLStringConverter("intercept"),
                             (const XMLCh*)XMLStringConverter(cals[0]));
    linearElem->setAttribute((const XMLCh*)XMLStringConverter("slope"),
                             (const XMLCh*)XMLStringConverter(cals[1]));

    varElem->appendChild(linearElem);
  }
} // add CalibElem

void Document::addVarCalFileElem(std::string varCalFileName,
                              const std::string & varUnits,
                              const std::string & siteName,
                              xercesc::DOMNode *sampleNode,
                              xercesc::DOMElement *varElem)
{
cerr<<"\nIn addVarCalFile:\n";
//cerr<<"   Calfilename: "<<varCalFileName;
//cerr<<"\n   Units: "<<varUnits;
//cerr<<"\n   Site: "<<siteName<<"\n";
  // We need a poly node
  const XMLCh * polyTagName = 0;
  XMLStringConverter xmlPoly("poly");
  polyTagName = (const XMLCh *) xmlPoly;

  // create a new DOM element for the poly node
  xercesc::DOMElement* polyElem = 0;
  try {
    polyElem  = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         polyTagName);
  } catch (DOMException &e) {
     cerr << "sampleNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("a2dVar create new poly calibration element: " +
                             (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the poly node attributes
  //std::string polyStr = cals[0];
  //for (size_t i = 1; i < cals.size(); i++)
    //if (cals[i].size()) polyStr += (" " + cals[i]);

  polyElem->setAttribute((const XMLCh*)XMLStringConverter("units"),
                           (const XMLCh*)XMLStringConverter(varUnits));

  // We need a calfile node
  const XMLCh * calfileTagName = 0;
  XMLStringConverter xmlCalFile("calfile");
  calfileTagName = (const XMLCh *) xmlCalFile;

  // Create a new DOM element for the calfile element.
  xercesc::DOMElement* calfileElem = 0;
  try {
    calfileElem = sampleNode->getOwnerDocument()->createElementNS(
         DOMable::getNamespaceURI(),
         calfileTagName);
  } catch (DOMException &e) {
     cerr << "dsmNode->getOwnerDocument()->createElementNS() threw exception\n";
     throw InternalProcessingException("dsm create new dsm sample element: "
                             + (std::string)XMLStringConverter(e.getMessage()));
  }

  // set up the calfile node attributes
  std::string tmpCalDir = "${TMP_PROJ_DIR}/Configuration/cal_files/Engineering/";
  tmpCalDir.append(siteName);
  std::string engCalDir = "${PROJ_DIR}/Configuration/cal_files/Engineering/";
  engCalDir.append(siteName);
  std::string engCalPath = tmpCalDir + ":" + engCalDir;
  calfileElem->setAttribute((const XMLCh*)XMLStringConverter("path"),
                           (const XMLCh*)XMLStringConverter (engCalPath));
  calfileElem->setAttribute((const XMLCh*)XMLStringConverter("file"),
                           (const XMLCh*)XMLStringConverter(varCalFileName));

  polyElem->appendChild(calfileElem);
  varElem->appendChild(polyElem);

} // addVarCalFileElem
