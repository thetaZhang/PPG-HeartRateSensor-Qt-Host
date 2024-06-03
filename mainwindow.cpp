#include "mainwindow.h"
#include "ui_mainwindow.h"


#define HEARTRATE_SERV_UUID         "0000180d-0000-1000-8000-00805f9b34fb"
#define HEARTRATE_MEAS_UUID         "00002a37-0000-1000-8000-00805f9b34fb"
#define BODY_SENSOR_LOC_UUID        "00002a38-0000-1000-8000-00805f9b34fb"
#define HEARTRATE_CTRL_PT_UUID      "00002a39-0000-1000-8000-00805f9b34fb"

#define DEVINFO_SERV_UUID           "0000180a-0000-1000-8000-00805f9b34fb"
#define SYSTEM_ID_UUID              "00002a23-0000-1000-8000-00805f9b34fb"
#define MODEL_NUMBER_UUID           "00002a24-0000-1000-8000-00805f9b34fb"
#define SERIAL_NUMBER_UUID          "00002a25-0000-1000-8000-00805f9b34fb"
#define FIRMWARE_REV_UUID           "00002a26-0000-1000-8000-00805f9b34fb"
#define HARDWARE_REV_UUID           "00002a27-0000-1000-8000-00805f9b34fb"
#define SOFTWARE_REV_UUID           "00002a28-0000-1000-8000-00805f9b34fb"
#define MANUFACTURER_NAME_UUID      "00002a29-0000-1000-8000-00805f9b34fb"
#define IEEE_11073_CERT_DATA_UUID   "00002a2a-0000-1000-8000-00805f9b34fb"
#define PNP_ID_UUID                 "00002a50-0000-1000-8000-00805f9b34fb"

#define BATT_SERV_UUID              "0000180f-0000-1000-8000-00805f9b34fb"
#define BATT_LEVEL_UUID             "00002a19-0000-1000-8000-00805f9b34fb"

#define DEVICE_NAME                 "HeartRateSensorTeamG"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

   m_waveLayout =new QVBoxLayout();

    // 波形显示
    m_waveView = new WaveView(this);
    m_waveView->setMinimumWidth(800);
    m_waveView->resizeDisplayDataSize(960);
    //ui->horizontalLayout_3->addWidget(m_waveView);

    m_waveView1 = new WaveView(this);
    m_waveView1->setMinimumWidth(800);
    m_waveView1->resizeDisplayDataSize(960);
    //ui->horizontalLayout_3->addWidget(m_waveView1);

    hr_waveView= new WaveView(this);
    hr_waveView->setMinimumWidth(800);
    hr_waveView->resizeDisplayDataSize(50);
    hr_waveView->renameaxis("","HeartRate");
    hr_waveView->setlinered();



    m_waveLayout->addWidget(m_waveView);

    m_waveLayout->addWidget(m_waveView1);
    m_waveLayout->addWidget(hr_waveView);


    ui->horizontalLayout_3->addLayout(m_waveLayout);

    // 蓝牙扫描器线程
    m_BLEScannerThread = new QThread();     // 新建子线程
    m_BLEScanner = new BLEScanner();
    m_BLEScanner->moveToThread(m_BLEScannerThread);
    connect(this, &MainWindow::startBLEScanSignal, m_BLEScanner, &BLEScanner::startScan);
    connect(this, &MainWindow::stopBLEScanSignal, m_BLEScanner, &BLEScanner::stopScan);
    connect(m_BLEScanner,&BLEScanner::deviceDiscoveredSignal, this, &MainWindow::on_deviceDiscovered);
    connect(m_BLEScanner,&BLEScanner::scanFinishedSiganl, this, &MainWindow::on_scanFinished);
    m_BLEScannerThread->start();

    // 心率传感器线程
    m_heartRateSensorThread = new QThread();
    m_heartRateSensor = new HeartRateSensor(120);
    m_heartRateSensor->moveToThread(m_heartRateSensorThread);
    connect(this, &MainWindow::newHeartRateRawDataArrive, m_heartRateSensor, &HeartRateSensor::pushNewData);
    connect(m_heartRateSensor, &HeartRateSensor::heartRateRawDataSignal, this, &MainWindow::on_newHeartRateRawDataSignal);
    connect(m_heartRateSensor,&HeartRateSensor::heartRateAfterDataSignal,this, &MainWindow::on_newHeartRateRawDataSignal1);
    connect(m_heartRateSensor, &HeartRateSensor::heartRateSignal, this, &MainWindow::on_newHeartRateSignal);
    connect(m_heartRateSensor, &HeartRateSensor::heartRateSignal, this, &MainWindow::onHeartRateSignalCb);

    m_heartRateSensorThread->start();


    QFile file("out_rawdata.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error opening file for writing";
    } else {
        file.close();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_deviceDiscovered(const QBluetoothDeviceInfo &info){
    ui->dialog->append("发现设备: " + info.name() + " with Address: " + info.address().toString());
    if(info.name().toStdString() == DEVICE_NAME){
//    if(info.name().to){
      ui->label_2->setText("已找到目标设备，连接中......");
      emit MainWindow::stopBLEScanSignal();                   // 搜索到目标设备后就停止搜索
      m_BLEDeviceControllerThread = new QThread();            // 新建子线程
      m_BLEDeviceController = new BLEDeviceController(info);  // 新建蓝牙设备控制器对象
      m_BLEDeviceController->moveToThread(m_BLEDeviceControllerThread);
      // 连接蓝牙设备控制器相关对象
      connect(this, &MainWindow::connectToBLEDeviceSignal, m_BLEDeviceController, &BLEDeviceController::connectToDevice);
      connect(this, &MainWindow::disconnectFromBLEDeviceSignal, m_BLEDeviceController, &BLEDeviceController::disconnectFromDevice);
      connect(this, &MainWindow::readBLEDeviceCharacteristicSignal, m_BLEDeviceController, &BLEDeviceController::readCharacteristic);
      connect(this, &MainWindow::writeBLEDeviceCharacteristicSignal, m_BLEDeviceController, &BLEDeviceController::writeCharacteristic);
      connect(this, &MainWindow::enableBLEDeviceCharacteristicNotifySignal, m_BLEDeviceController, &BLEDeviceController::enableCharacteristicNotify);

      //连接后显示文字
      connect(m_BLEDeviceController,&BLEDeviceController::isConnectedSignal,this,&MainWindow::isBLEConnected);

      // 连接蓝牙设备控制器特征值回调函数
      connect(m_BLEDeviceController, &BLEDeviceController::characteristicReadSignal, this, &MainWindow::on_characteristicReadCb);
      connect(m_BLEDeviceController, &BLEDeviceController::characteristicNotifySignal, this, &MainWindow::on_characteristicNotifyCb);
      m_BLEDeviceControllerThread->start();               // 启动线程
      emit MainWindow::connectToBLEDeviceSignal();        //发现目标设备后即刻连接
    }
}

void MainWindow::on_scanFinished(){
    ui->dialog->append("蓝牙扫描完毕");
}

void MainWindow::on_characteristicReadCb(QLowEnergyCharacteristic info, QByteArray value){
    ui->dialog->append("Characteristic Read from: " + info.uuid().toString() + "Value:" + QString::number(value.toHex().toInt(nullptr, 16)));
}

void MainWindow::on_characteristicNotifyCb(QLowEnergyCharacteristic info, QByteArray value){
//    ui->dialog->append("Notification received from: " + info.uuid().toString() + "Value:" + QString::number(value.toHex().toInt(nullptr, 16)));
    emit newHeartRateRawDataArrive(value);
}

void MainWindow::on_startBLEScanButton_clicked()
{
    emit MainWindow::startBLEScanSignal();
    ui->label_2->setText("蓝牙扫描中......");
    ui->dialog->append("开启蓝牙扫描");
}


void MainWindow::on_stopBLEScanButton_clicked()
{
    emit MainWindow::stopBLEScanSignal();
    ui->dialog->append("停止蓝牙扫描");
}


void MainWindow::on_startSampleButton_clicked()
{
    emit MainWindow::enableBLEDeviceCharacteristicNotifySignal(HEARTRATE_SERV_UUID, HEARTRATE_MEAS_UUID, true);
    ui->dialog->append("开始测量");
}


void MainWindow::on_stopSampleButton_clicked()
{
    emit MainWindow::enableBLEDeviceCharacteristicNotifySignal(HEARTRATE_SERV_UUID, HEARTRATE_MEAS_UUID, false);
    ui->dialog->append("停止测量");
}

void MainWindow::on_newHeartRateRawDataSignal(std::vector<int16_t> heartRateRawDataVector){

    vector<double> heartRateRawDoubleDataVector(heartRateRawDataVector.begin(),heartRateRawDataVector.end());

    QFile fileOpen("out_rawdata.txt");
    if (!fileOpen.open(QIODevice::Append | QIODevice::Text)) {qDebug() << "fileOpen error";}
    else {
        QTextStream out(&fileOpen);
        for (int i =0; i < heartRateRawDoubleDataVector.size();i++) {
            out << heartRateRawDoubleDataVector[i] << '\n';
        }
    }
    fileOpen.close();


    // 波形更新
    for(int i =0; i < heartRateRawDataVector.size();i++){
      m_waveView->pushdata(heartRateRawDataVector[i]);
    }

}



void MainWindow::on_newHeartRateRawDataSignal1(std::vector<double> heartRateRawDataVector,std::vector<int> heartRatePeaksVector){


    // 波形更新
    for(int i =0; i < heartRateRawDataVector.size();i++){
        m_waveView1->pushdata(heartRateRawDataVector[i]);

    }

    /*
    for (int i=0;i<heartRatePeaksVector.size();i++) {
        m_waveView1->pushpeaks(heartRatePeaksVector[i],heartRateRawDataVector[heartRatePeaksVector[i]]);
    }*/

}


void MainWindow::on_newHeartRateSignal(double heartRate,bool isFingerOn){
    ui->dialog->append("HR: " + QString::number(heartRate));
}



void MainWindow::onHeartRateSignalCb(double heartRate,double RespiratoryRate,double HRV,bool isFingerOn)
{
    if (isFingerOn){
        ui->label->setText("HR: " + QString::number(heartRate)+"bpm");
        hr_waveView->pushdata(heartRate);

        ui->label_3->setText("RR: " + QString::number(RespiratoryRate)+"bpm");
        ui->label_4->setText("HRV: " + QString::number(HRV)+"ms");
    }
    else {
        ui->label->setText("请放好放稳您的手指");
        ui->label_3->setText("RR: -");
        ui->label_4->setText("HRV:-");
    }
}

void MainWindow::isBLEConnected(int count) {
    static double percent=0;
    if (count==18){
    ui->label_2->setText("已连接");
        count=0;
        percent=0;
    }
    else {
        percent+=10;
        ui->label_2->setText("已找到目标设备，连接中......");
    }
}

