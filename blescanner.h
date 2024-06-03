#ifndef BLESCANNER_H
#define BLESCANNER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>

class BLEScanner : public QObject
{
    Q_OBJECT
public:
    explicit BLEScanner(QObject *parent = nullptr);

public slots:
    void startScan();
    void stopScan();

signals:
    void deviceDiscoveredSignal(const QBluetoothDeviceInfo &info);
    void scanFinishedSiganl();

private:
    QBluetoothDeviceDiscoveryAgent *m_BLEDeviceDiscoveryAgent;  // 蓝牙设备发现控制器

};

#endif // BLESCANNER_H
