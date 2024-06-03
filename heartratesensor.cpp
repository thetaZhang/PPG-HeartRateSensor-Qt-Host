#include "heartratesensor.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <QDebug>

using namespace std;


HeartRateSensor::HeartRateSensor(int sampleRate)
    : QObject{nullptr}
{
    m_heartRateClass = new heartRateClass(sampleRate);
}

void HeartRateSensor::pushNewData(QByteArray newData){
    std::vector<int16_t> pd_data;
    // 确保QByteArray的大小是int16_t大小的整数倍
    if(newData.size() % sizeof(int16_t) == 0) {
        int numElements = newData.size() / sizeof(int16_t);
        const int16_t* dataPtr = reinterpret_cast<const int16_t*>(newData.data());

        // 使用指针初始化vector
        pd_data.assign(dataPtr, dataPtr + numElements);
        emit heartRateRawDataSignal(pd_data);
        //
        std::vector<double> tempVector;
        int size = pd_data.size();
        //            std::cout << "pd_data.size()=" << pd_data.size() << " size=" << size << std::endl;
        tempVector.resize(size);
        for(int i = 0; i < size; ++i){                          // 取绿光的数据
            tempVector[i] = static_cast<double>(pd_data[i]);
        }
        m_heartRateClass->pushRawData(tempVector);
        double heartRate = m_heartRateClass->calculateHeartRateByMethodDiff();
        vector<double> wave_display_data = m_heartRateClass->wave_data;
        vector<int> wave_display_peaks = m_heartRateClass->wave_peaks;
        bool isFingerOn=m_heartRateClass->is_finger_on;
        double RespiratoryRate=m_heartRateClass->breathRate;
        double HRV_SDNN=m_heartRateClass->HRV;
        emit heartRateAfterDataSignal(wave_display_data,wave_display_peaks);
        emit heartRateSignal(heartRate,RespiratoryRate,HRV_SDNN,isFingerOn);

    } else {
        qDebug() << "QByteArray 的大小不是 int16_t 的整数倍";
    }
}

