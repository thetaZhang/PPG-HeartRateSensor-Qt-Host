#ifndef BLEDEVICECONTROLLER_H
#define BLEDEVICECONTROLLER_H

#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QObject>


class BLEDeviceController : public QObject
{
    Q_OBJECT
public:
    enum DEVICE_STATUS{
        disconnect,
        connected
    };
    explicit BLEDeviceController(const QBluetoothDeviceInfo &deviceInfo);
    void connectToDevice();
    void disconnectFromDevice();
    void readCharacteristic(const QString &serviceUuidStr, const QString &characteristicUuidStr);
    void writeCharacteristic(const QString &serviceUuidStr, const QString &characteristicUuidStr, const QByteArray &value);
    void enableCharacteristicNotify(const QString &serviceUuidStr, const QString &characteristicUuidStr, const bool &enable);

private slots:
    void on_deviceConnected();
    void on_errorReceived(QLowEnergyController::Error error);
    void on_serviceDiscoveredFinished();
    void on_serviceStateChanged(QLowEnergyService::ServiceState s);
    void on_characteristicRead(const QLowEnergyCharacteristic &info, const QByteArray &value);
    void on_characteristicNotify(const QLowEnergyCharacteristic &info, const QByteArray &value);

signals:
    void characteristicReadSignal(QLowEnergyCharacteristic info, QByteArray value);
    void characteristicNotifySignal(QLowEnergyCharacteristic info,QByteArray value);
    void isConnectedSignal(int count);

private:
    QLowEnergyController *m_controller;
    DEVICE_STATUS m_status;
    QList<QLowEnergyService*> m_services;
};

#endif // BLEDEVICECONTROLLER_H
