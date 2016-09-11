#ifdef UNDEFINED__

#include <onewire.h>

double getDSTemp() {
    dstemp.requestTemperatures();

    double temp = dstemp.getTempC(dsaddr);

    return temp;
}

#endif
