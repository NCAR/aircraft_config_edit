#include "DeviceValidator.h"
#include <iostream>
#include <fstream>


//std::pair<std::string,
//          DeviceValidator::_DeviceDefinition> 
   DeviceValidator::_DeviceDefinition DeviceValidator::_Definitions[] = {
{"Analog", "/dev/ncar_a2d", 0, 9},
{"ADC-GV", "/dev/arinc", 0, 9},
{"Butanol_CN_Counter", "/dev/ttyS", 0, 9},
{"CCN", "/dev/ttyS", 0, 9},
{"CDP", "/dev/ttyS", 0, 9},
{"CFDC", "/dev/ttyS", 0, 9},
{"CMIGITS3", "/dev/ttyS", 0, 9},
{"COMR", "/dev/ttyS", 0, 9},
{"CORAW", "/dev/ttyS", 0, 9},
{"D_GPS", "/dev/ttyS", 0, 9},
{"DewPointer", "/dev/ttyS", 0, 9},
{"Fast2DC", "/dev/usbtwod", 0, 9},
{"Fast2DC", "/dev/usbtwod_64_", 0, 9},
{"GPS-GV", "/dev/arinc", 0, 9},
{"Garmin_GPS", "/dev/ttyS", 0, 9},
{"HoneywellPPT", "/dev/ttyS", 0, 9},
{"IRIG", "/dev/irig", 0, 9},
{"IRS-C130", "/dev/arinc", 0, 9},
{"IRS-GV", "/dev/arinc", 0, 9},
{"LAMS", "/dev/lams", 0, 9},
{"Mensor_6100", "/dev/ttyS", 0, 9},
{"NOAA_CSD_O3", "/dev/ttyS", 0, 9},
{"Novatel_GPS", "/dev/ttyS", 0, 9},
{"OphirIII", "/dev/ttyS", 0, 9},
{"Paro_DigiQuartz_1000", "/dev/ttyS", 0, 9},
{"QCLS", "/dev/ttyS", 0, 9},
{"S100", "/dev/ttyS", 0, 9},
{"S200", "/dev/ttyS", 0, 9},
{"S300", "/dev/ttyS", 0, 9},
{"SP2", "/dev/ttyS", 0, 9},
{"TDLH2O", "/dev/ttyS", 0, 9},
{"TwoDP", "/dev/usbtwod_32_", 0, 9},
{"TwoD_House", "/dev/ttyS", 0, 9},
{"UHSAS", "/dev/ttyS", 0, 9},
{"UHSAS_CU", "/dev/ttyS", 0, 9},
{"VCSEL", "/dev/ttyS", 0, 9},
{"Water_CN_Counter", "/dev/ttyS", 0, 9},
};

DeviceValidator * DeviceValidator::_instance = NULL;

DeviceValidator::DeviceValidator() //: 
       //_devMap(_Definitions, _Definitions + sizeof(_Definitions) / sizeof(_DeviceDefinition)) 
{

  int n =  sizeof(_Definitions) / sizeof(_DeviceDefinition);
  for (int i = 0; i < n;  i++) {
    //_devMap.insert ( _devMap.rbegin(), std::pair<std::string, _DeviceDefinition>(_Definitions[i].sensorName, _Definitions[i]));
    _devMap.insert ( std::pair<std::string, _DeviceDefinition>(_Definitions[i].sensorName, _Definitions[i]));
  }
  for (std::map<std::string, _DeviceDefinition>::iterator it = _devMap.begin(); it != _devMap.end(); it++) {

      std::cerr << it->first << std::endl;
//    cerr << it->first << " " << (_DeviceDefinition)it->second.devicePrefix << " " << (_DeviceDefinition)it->second.min << " " << (_DeviceDefinition)it->second.max << endl;

  }

}
