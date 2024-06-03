#ifndef HEARTRATESENSOR_H
#define HEARTRATESENSOR_H

#include <QObject>
#include <vector>
#include <QTimer>
#include "heartRateClass.hpp"

using namespace std;

class HeartRateSensor : public QObject
{
Q_OBJECT
public:
    HeartRateSensor(int sampleRate = 100);

public slots:
    void pushNewData(QByteArray newData);   // 新数据

signals:
    void heartRateRawDataSignal(std::vector<int16_t> heartRateRawDataVector);    // 原始数据信号
    void heartRateAfterDataSignal(std::vector<double> wave_data,std::vector<int> peaks_data);// 波形显示信号

    void heartRateSignal(double heartRate,double RespiratoryRate,double HRV,bool isFingerOn);                      // 心率值



private:
    heartRateClass *m_heartRateClass;
};


#endif // HEARTRATESENSOR_H
