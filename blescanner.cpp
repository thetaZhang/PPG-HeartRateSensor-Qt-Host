#include "blescanner.h"
#include <iostream>

BLEScanner::BLEScanner(QObject *parent)
    : QObject{parent}
{
    m_BLEDeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_BLEDeviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BLEScanner::deviceDiscoveredSignal);
    connect(m_BLEDeviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BLEScanner::scanFinishedSiganl);
}

void BLEScanner::startScan(){
    m_BLEDeviceDiscoveryAgent->start();
}

void BLEScanner::stopScan(){
    m_BLEDeviceDiscoveryAgent->stop();
}
