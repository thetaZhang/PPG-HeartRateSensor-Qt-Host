
#include "bledevicecontroller.h"
#include <iostream>

BLEDeviceController::BLEDeviceController(const QBluetoothDeviceInfo &deviceInfo)
    : QObject{nullptr}
{
    m_controller = QLowEnergyController::createCentral(deviceInfo, this);
    m_status = disconnect;
    connect(m_controller, &QLowEnergyController::connected, this, &BLEDeviceController::on_deviceConnected);
    connect(m_controller, &QLowEnergyController::errorOccurred, this, &BLEDeviceController::on_errorReceived);
}

void BLEDeviceController::connectToDevice(){
    m_controller->connectToDevice();
}

void BLEDeviceController::disconnectFromDevice(){
    if(m_status == connected)
        m_controller->disconnectFromDevice();
}

void BLEDeviceController::on_deviceConnected(){
    qDebug() << "device connected.";
    m_status = connected;
    // 开始发现服务，绑定服务发现完成回调函数
    connect(m_controller, &QLowEnergyController::discoveryFinished, this, &BLEDeviceController::on_serviceDiscoveredFinished);
    m_controller->discoverServices();
}

void BLEDeviceController::on_errorReceived(QLowEnergyController::Error error){
    qDebug() << "QLowEnergyController Error Code:" << error;
}

void BLEDeviceController::on_serviceDiscoveredFinished(){
    qDebug() << "Service discovery finished.";

    foreach (const QBluetoothUuid &serviceUuid, m_controller->services()) {
        QLowEnergyService *service = m_controller->createServiceObject(serviceUuid, this);
        if (service) {                                                                          // 成功创建service
            m_services.append(service);
            qDebug() << "Service discovered:" <<serviceUuid.toString();
            connect(service, &QLowEnergyService::stateChanged, this, &BLEDeviceController::on_serviceStateChanged);         // 服务状态改变回调函数
            connect(service, &QLowEnergyService::characteristicRead, this, &BLEDeviceController::on_characteristicRead); // 绑定读特征值回调函数
            service->discoverDetails(); // 开始发现服务中的特性
        }
    }

}

void BLEDeviceController::on_serviceStateChanged(QLowEnergyService::ServiceState s){
    if (s == QLowEnergyService::RemoteServiceDiscovered) {
        // 服务的所有特性已经被发现
        const QLowEnergyService *service = qobject_cast<QLowEnergyService *>(sender());
        const QList<QLowEnergyCharacteristic> characteristics = service->characteristics();
        foreach (const QLowEnergyCharacteristic &ch, characteristics) {
            static int count=0;
            // 这里可以根据需要对每个特性进行操作，例如读取特性值
            qDebug() << "Discovered characteristic:" << ch.uuid().toString();
            count++;
            emit isConnectedSignal(count);

        }

    }
}

void BLEDeviceController::readCharacteristic(const QString &serviceUuidStr, const QString &characteristicUuidStr){
    // 将 QString 转换为 QBluetoothUuid
    QBluetoothUuid serviceUuid(serviceUuidStr);
    QBluetoothUuid characteristicUuid(characteristicUuidStr);
    // 查找并设置当前服务
    QLowEnergyService *service = nullptr;
    foreach(auto discoveredService, m_services) {
        if(discoveredService->serviceUuid() == serviceUuid) {
            service = discoveredService;
            break;
        }
    }

    if (service == nullptr) {
        qDebug() << "Service not found for UUID:" << serviceUuidStr;
        return;
    }

    // 查找特征
    QLowEnergyCharacteristic characteristic = service->characteristic(characteristicUuid);
    if (!characteristic.isValid()) {
        qDebug() << "Characteristic not found for UUID:" << characteristicUuidStr;
        return;
    }

    // 读取特征
    service->readCharacteristic(characteristic);

}

void BLEDeviceController::writeCharacteristic(const QString &serviceUuidStr, const QString &characteristicUuidStr, const QByteArray &value){
    // 将 QString 转换为 QBluetoothUuid
    QBluetoothUuid serviceUuid(serviceUuidStr);
    QBluetoothUuid characteristicUuid(characteristicUuidStr);
    // 查找并设置当前服务
    QLowEnergyService *service = nullptr;
    foreach(auto discoveredService, m_services) {
        if(discoveredService->serviceUuid() == serviceUuid) {
            service = discoveredService;
            break;
        }
    }

    if (service == nullptr) {
        qDebug() << "Service not found for UUID:" << serviceUuidStr;
        return;
    }

    // 查找特征
    QLowEnergyCharacteristic characteristic = service->characteristic(characteristicUuid);
    if (!characteristic.isValid()) {
        qDebug() << "Characteristic not found for UUID:" << characteristicUuidStr;
        return;
    }

    // 写入特征值
    service->writeCharacteristic(characteristic, value, QLowEnergyService::WriteWithoutResponse);
//    service->writeCharacteristic(characteristic, value, QLowEnergyService::WriteWithResponse);
}

void BLEDeviceController::enableCharacteristicNotify(const QString &serviceUuidStr, const QString &characteristicUuidStr, const bool &enable){
    // 将 QString 转换为 QBluetoothUuid
    QBluetoothUuid serviceUuid(serviceUuidStr);
    QBluetoothUuid characteristicUuid(characteristicUuidStr);
    QBluetoothUuid cccdUuid(QBluetoothUuid::DescriptorType(0x2902));

    // 查找并设置当前服务
    QLowEnergyService *service = nullptr;
    foreach(auto discoveredService, m_services) {
        if(discoveredService->serviceUuid() == serviceUuid) {
            service = discoveredService;
            break;
        }
    }

    if (service == nullptr) {
        qDebug() << "Service not found for UUID:" << serviceUuidStr;
        return;
    }

    // 查找特征
    QLowEnergyCharacteristic characteristic = service->characteristic(characteristicUuid);
    if (!characteristic.isValid()) {
        qDebug() << "Characteristic not found for UUID:" << characteristicUuidStr;
        return;
    }

    // 获取特性的客户端特性配置描述符（CCCD）
    QLowEnergyDescriptor cccd = characteristic.descriptor(cccdUuid);
    if (!cccd.isValid()) {
        qWarning() << "CCCD not found for characteristic:" << characteristicUuidStr;
        return;
    }

    if(enable){
        QByteArray enableNotificationValue(2,0);
        enableNotificationValue[0] = 0x01;
        service->writeDescriptor(cccd, enableNotificationValue);
        QObject::connect(service, &QLowEnergyService::characteristicChanged, this, &BLEDeviceController::on_characteristicNotify);       // 绑定Notify回调信号
    } else{
        QByteArray enableNotificationValue(2,0);
        service->writeDescriptor(cccd, enableNotificationValue);
        QObject::disconnect(service, &QLowEnergyService::characteristicChanged, this, &BLEDeviceController::on_characteristicNotify);    // 解除绑定Notify回调信号
    }

}

void BLEDeviceController::on_characteristicRead(const QLowEnergyCharacteristic &info, const QByteArray &value){
    qDebug() << "Characteristic Read from: " << info.uuid().toString() << "Value:" << value;
    emit characteristicReadSignal(info, value);
}

void BLEDeviceController::on_characteristicNotify(const QLowEnergyCharacteristic &info, const QByteArray &value){
    qDebug() << "Notification received from: " << info.uuid().toString() << " Value:" << value;
    emit characteristicNotifySignal(info, value);
}


