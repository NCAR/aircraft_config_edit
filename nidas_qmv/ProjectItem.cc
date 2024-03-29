/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*- */
/* vim: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 ********************************************************************
 ** NIDAS: NCAR In-situ Data Acquistion Software
 **
 ** 2010, Copyright University Corporation for Atmospheric Research
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

#include "ProjectItem.h"
#include "SensorItem.h"
#include "NidasModel.h"

#include <iostream>
#include <fstream>

#include <exceptions/InternalProcessingException.h>

using namespace xercesc;
using namespace std;


ProjectItem::ProjectItem(Project *project, int row, NidasModel *theModel, NidasItem *parent) // :
{
    _project = project;
    domNode = 0;

    // Record the item's location within its parent.
    rowNumber = row;
    parentItem = parent;
    model = theModel;
}

/* Should never be called - we don't give access to the Project in the UI */
ProjectItem::~ProjectItem()
{
std::cerr << "call to ~ProjectItem() \n";
try {
      _project->destroyInstance();
} catch (...) {
    // ugh!?!
    cerr << "Caught Exception in ~Projectitem() \n";
}
}

NidasItem *ProjectItem::child(int i)
{
    if ((i>=0) && (i<childItems.size()))
        return childItems[i];

    int j;

    SiteIterator it;
std::cerr << "Starting to fillout child of Project  - i = " << i << "\n";
    for (j=0, it = _project->getSiteIterator(); it.hasNext(); j++) {
std::cerr << "getting next site\n";
        Site* site = it.next();
        if (j<i) continue; // skip old cached items (after it.next())
        NidasItem *childItem = new SiteItem(site, j, model, this);
        childItems.append( childItem);
    }

    // we tried to build children but still can't find requested row i
    // probably (always?) when i==0 and this item has no children
    if ((i<0) || (i>=childItems.size())) return 0;

    // we built children, return child i from it
    return childItems[i];

}


/// find the DOM node which defines this Project
DOMNode *ProjectItem::findDOMNode() 
{
DOMDocument *domdoc = model->getDOMDocument();
if (!domdoc) return 0;

  DOMNodeList * ProjectNodes = domdoc->getElementsByTagName((const XMLCh*)XMLStringConverter("project"));

  DOMNode * ProjectNode = 0;
  for (XMLSize_t i = 0; i < ProjectNodes->getLength(); i++) 
  {
     XDOMElement xnode((DOMElement *)ProjectNodes->item(i));
     const string& sProjectName = xnode.getAttributeValue("name");
     string projectName = _project->getName();
     if (sProjectName == projectName) { 
       cerr<<"getProjectNode - Found ProjectNode with name:" << sProjectName << endl;
       ProjectNode = ProjectNodes->item(i);
       break;
     }
  }

  domNode = ProjectNode;
  return ProjectNode;

}

QString ProjectItem::dataField(int column)
{
  if (column == 0) return name();

  return QString();
}

/*!
 * \brief remove the site \a item from this Project's Nidas and DOM trees
 *
 * current implementation confused between returning bool and throwing exceptions
 * due to refactoring from Document
 *
 */
bool ProjectItem::removeChild(NidasItem *item)
{
cerr << "ProjectItem::removeChild\n";
Site *site = dynamic_cast<Site*>(item);
string deleteSite = site->getName();
cerr << " deleting site " << deleteSite << "\n";

  //Project *project = this->getProject();
    // get the DOM node for this Project
  xercesc::DOMNode *projectNode = this->getDOMNode();
  if (!projectNode) {
    throw InternalProcessingException("null project DOM node");
  }

    // delete the site from this Projects DOM node
  xercesc::DOMNode* child;
  xercesc::DOMNodeList* projectChildren = projectNode->getChildNodes();
  XMLSize_t numChildren, index;
  numChildren = projectChildren->getLength();
  for (index = 0; index < numChildren; index++ )
  {
      if (!(child = projectChildren->item(index))) continue;
      if (child->getNodeType() != xercesc::DOMNode::ELEMENT_NODE) continue;
      nidas::core::XDOMElement xchild((xercesc::DOMElement*) child);

      const string& elname = xchild.getNodeName();
      if (elname == "site") 
      {
        const string & name = xchild.getAttributeValue("name");
        cerr << "found node with name " << elname  << " and name attribute " << name << endl;

          if (name == deleteSite) 
          {
             xercesc::DOMNode* removableChld = projectNode->removeChild(child);
             removableChld->release();
          }
      }
  }

/* Cannot delete a site from the project
    // delete site from nidas model (Project tree)
    for (SiteIterator si = project->getSiteIterator(); si.hasNext(); ) {
      Site* site = si.next();
      cerr << "found site with name " << site->getName() << "\n";
      if (site->getname() == deleteSite) {
         cerr << "  calling removeSite()\n";
         project->removeSite(site); // do not delete, leave that for ~SiteItem()
         break; // Nidas has only 1 object per site, regardless of how many XML has
         }
    }
*/

  return true;
}

QString ProjectItem::name()
{
    cerr << "name() called from ProjectItem class \n";
    return(QString::fromStdString(_project->getName()));
}
