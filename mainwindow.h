#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "blescanner.h"
#include "bledevicecontroller.h"
#include "heartratesensor.h"
#include "waveview.h"
#include <QFile>
#include <QTextStream>
#include <QtCharts/QScatterSeries>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    // 蓝牙扫描器控制信号
    void startBLEScanSignal();
    void stopBLEScanSignal();

    // 蓝牙设备控制器：控制信号
    void connectToBLEDeviceSignal();
    void disconnectFromBLEDeviceSignal();
    void readBLEDeviceCharacteristicSignal(const QString &serviceUuidStr, const QString &characteristicUuidStr);
    void writeBLEDeviceCharacteristicSignal(const QString &serviceUuidStr, const QString &characteristicUuidStr, const QByteArray &value);
    void enableBLEDeviceCharacteristicNotifySignal(const QString &serviceUuidStr, const QString &characteristicUuidStr, const bool &enable);

    // 蓝牙notify的心率数据
    void newHeartRateRawDataArrive(QByteArray newData);

private slots:


    void on_deviceDiscovered(const QBluetoothDeviceInfo &info);
    void on_scanFinished();

    void on_characteristicReadCb(QLowEnergyCharacteristic info, QByteArray value);
    void on_characteristicNotifyCb(QLowEnergyCharacteristic info, QByteArray value);

    void on_startBLEScanButton_clicked();
    void on_stopBLEScanButton_clicked();
    void on_startSampleButton_clicked();
    void on_stopSampleButton_clicked();

    void on_newHeartRateRawDataSignal(std::vector<int16_t> heartRateRawDataVector);
    void on_newHeartRateRawDataSignal1(std::vector<double> heartRateRawDataVector,std::vector<int> heartRatePeaksVector);
    void on_newHeartRateSignal(double heartRate,bool isFingerOn);

    void onHeartRateSignalCb(double heartRate,double RespiratoryRate,double HRV,bool isFingerOn);



    void isBLEConnected(int count);

private:
    Ui::MainWindow *ui;
    BLEScanner *m_BLEScanner = nullptr;                     // 蓝牙扫描器对象
    QThread *m_BLEScannerThread = nullptr;                  // 蓝牙扫描器线程
    BLEDeviceController *m_BLEDeviceController = nullptr;   // 蓝牙设备控制器对象
    QThread *m_BLEDeviceControllerThread = nullptr;         // 蓝牙设备控制器线程
    HeartRateSensor *m_heartRateSensor = nullptr;           // 心率传感器对象
    QThread *m_heartRateSensorThread = nullptr;             // 心率传感器线程

    QVBoxLayout *m_waveLayout = nullptr;

    WaveView* m_waveView;                                   // 波形显示
    WaveView* m_waveView1;
    WaveView* hr_waveView;

    vector<double> filter_a(const vector<double>& b, const vector<double>& a, const vector<double>& x);
    vector<double> forward_difference(const vector<double>& in_data);
};
#endif // MAINWINDOW_H
