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

#ifndef _SENSOR_ITEM_H
#define _SENSOR_ITEM_H

#include "NidasItem.h"
#include <nidas/core/DSMSensor.h>
#include <nidas/dynld/raf/DSMAnalogSensor.h>
#include <nidas/core/SensorCatalog.h>
#include <nidas/core/CalFile.h>

using namespace nidas::core;
using namespace nidas::dynld::raf;


class SensorItem : public NidasItem
{

public:
    SensorItem(DSMSensor *sensor, int row, NidasModel *theModel,
               NidasItem *parent = 0) ;
    SensorItem(DSMAnalogSensor *sensor, int row, NidasModel *theModel,
               NidasItem *parent = 0) ;

    ~SensorItem();

    NidasItem * child(int i);
    void refreshChildItems();

    void fromDOM();

    bool removeChild(NidasItem *item);

    std::string devicename() { return this->_sensor->getDeviceName(); }

    const QVariant & childLabel(int column) const {
          if (column == 0) return NidasItem::_Variable_Label;
          if (column == 1) return NidasItem::_Rate_Label;
          if (column == 2) return NidasItem::_CalCoef_Label;
          if (column == 3) return NidasItem::_CalCoefSrc_Label;
          if (column == 4) return NidasItem::_CalDate_Label;
          if (column == 5) return NidasItem::_Sample_Label;
          return NidasItem::_Unknown_Label;
    }

    int childColumnCount() const {return 6;}

    QString dataField(int column);

    xercesc::DOMNode* getDOMNode() {
        if (domNode)
          return domNode;
        else return domNode=findDOMNode();
        }

    QString getBaseName();
    QString getDevice() { return QString::fromStdString(_sensor->getDeviceName()); }

// at some point this should be protected.
//protected:
        // get/convert to the underlying model pointers
    DSMSensor *getDSMSensor() const { return _sensor; }
    //xercesc::DOMNode * findSampleDOMNode(SampleTag * sampleTag);
    xercesc::DOMNode * findSampleDOMNode(unsigned int sampleId);

    // Subclass needs to set this, otherwise it's null string
    virtual std::string getSerialNumberString() { return(std::string()); }

protected:
    QString viewName();
    xercesc::DOMNode *findDOMNode();
    DSMSensor * _sensor;

private:

};

#endif
