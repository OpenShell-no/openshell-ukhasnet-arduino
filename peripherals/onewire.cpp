double getDSTemp() {
    dstemp.requestTemperatures();

    double temp = dstemp.getTempC(dsaddr);

    return temp;
}
