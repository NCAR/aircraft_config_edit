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

#ifndef _DSC_A2DSENSOR_ITEM_H
#define _DSC_A2DSENSOR_ITEM_H

#include "SensorItem.h"
#include <nidas/dynld/DSC_A2DSensor.h>
#include <nidas/core/SensorCatalog.h>
#include <nidas/core/CalFile.h>

using namespace nidas::core;
using namespace nidas::dynld;
//using namespace nidas::dynld::raf;

class DSC_A2DSensorItem : public SensorItem
{

public:
    DSC_A2DSensorItem(DSC_A2DSensor *sensor, int row, NidasModel *theModel,
                  NidasItem *parent) ;

    NidasItem * child(int i);
    void refreshChildItems();

 //   std::string devicename() { return this->dataField(1).toStdString(); }

    const QVariant & childLabel(int column) const {
          if (column == 0) return NidasItem::_Variable_Label;
          if (column == 1) return NidasItem::_Channel_Label;
          if (column == 2) return NidasItem::_Rate_Label;
          if (column == 3) return NidasItem::_Volt_Label;
          if (column == 4) return NidasItem::_CalCoef_Label;
          if (column == 5) return NidasItem::_CalCoefSrc_Label;
          if (column == 6) return NidasItem::_CalDate_Label;
          if (column == 7) return NidasItem::_Sample_Label;
          return NidasItem::_Unknown_Label;
    }

    int childColumnCount() const {return 8;}

    bool removeChild(NidasItem *item);

    void updateDOMCalFile(const std::string & calFileName);

    std::string getCalFileName();

    std::string getSerialNumberString();

// at some point this should be protected.
//protected:
        // get/convert to the underlying model pointers
    DSC_A2DSensor *getDSC_A2DSensor() const { return dynamic_cast<DSC_A2DSensor*>(_sensor); }

private:
    //DSC_A2DSensor * _sensor;

};

#endif
