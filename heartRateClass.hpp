#ifndef HEARTRATECLASS_H
#define HEARTRATECLASS_H

#include <iostream>
#include <vector>
#include <fstream>
#include <numeric>
#include <queue>
#include <complex.h>
#include <QDebug>
#include <cmath>





#define Delta 4096

using namespace std;
const double PI = 3.14159265358979323846;

class heartRateClass {

public:
    std::vector<double> wave_data;
    std::vector<int> wave_peaks;
    double breathRate=0;
    double HRV=0;
    bool is_finger_on=0;
    explicit heartRateClass(int sampleRate = 120){
        m_sampleRate = sampleRate;                      // set sampleRate
        int windowSize = 8 * sampleRate;
        m_heartRateEightSecsWindow.resize(windowSize);  // resize window size
        for(int i = 0; i < windowSize; ++i){
            m_heartRateEightSecsWindow[i] = 0;          // initial window data
        }
        m_heartRate = 70;                               // set heartRate initial value
        HRV=0;
        breathRate=0;
        is_finger_on=0;
    }

    void pushRawData(const std::vector<double>& newDataVector){
        //const std::vector<double> filter_data=filter_a(b_fil,a_fil, newDataVector);
        std::vector<double> tempVector (m_heartRateEightSecsWindow.begin()+m_sampleRate,m_heartRateEightSecsWindow.end());   // 8s心率窗口后面的数据
        tempVector.insert(tempVector.end(), newDataVector.begin(),newDataVector.end());                                     // 8s心率窗口添加新的数据
        m_heartRateEightSecsWindow = tempVector;                                                                            // 临时窗口赋给心率窗口
    }

    double calculateHeartRateByMethodDiff(){
        // wave_data.clear();
        // double sum=0.0;
        // for (int i =0;i<m_heartRateEightSecsWindow.size();i++) {
        //     sum+=m_heartRateEightSecsWindow[i];
        //     wave_data.push_back(sum/(i+1));
        // }
        wave_data=slide_T(m_heartRateEightSecsWindow,120);
        //qDebug() <<"wave_data" << wave_data.size();
        struct point peaks,valleys;
        find_peaks_adaptive_threshold(wave_data,&peaks,0.2,m_sampleRate);
        find_valley_adaptive_threshold(wave_data,&valleys,0.2,m_sampleRate);
        qDebug() << "peaksize" << peaks.position.size() << "valleysize" << valleys.position.size();
        //tidy_valley(&peaks,&valleys,wave_data);
        m_heartRate = cal_hr(m_heartRate,valleys.position,m_sampleRate);                                                                   // 计算心率
        return m_heartRate;



    }

    double calculateHeartRateByMethodFilter(){
        bool is_finger_on_last=is_finger_on;
        double hr_peak,hr_fft;
        double test_mean=0.0;
        int touch_start=0;
        if (mean(m_heartRateEightSecsWindow)<=2000) {
            is_finger_on=0;
        }
        else {
            is_finger_on=1;
        }
        if ((accumulate(m_heartRateEightSecsWindow.end()-m_sampleRate,m_heartRateEightSecsWindow.end(),0)/m_sampleRate) <=2000) {
            is_finger_on=0;
        }

        for (int i=1;i<m_heartRateEightSecsWindow.size();i++) {
            test_mean=accumulate(m_heartRateEightSecsWindow.begin(),m_heartRateEightSecsWindow.begin()+i,0)/(i);
            if (test_mean >1500.0) {touch_start=i-1;break;}
            if (i==m_heartRateEightSecsWindow.size()-1) {
                is_finger_on=0;
            }
        }


        qDebug() << "touch" << touch_start;
        static vector<double> data_reg(8*m_sampleRate,0); // 暂存上一次数据用于率波以消除滤波稳定时间
        //qDebug() << "regsize" << data_reg.size();
        vector<double> filter_in_data,filter_out_data,filter_out_data_base;
        filter_in_data.insert(filter_in_data.end(),data_reg.begin(),data_reg.end());
        filter_in_data.insert(filter_in_data.end(),m_heartRateEightSecsWindow.begin(),m_heartRateEightSecsWindow.end());
        //qDebug() << "fil_insize" << filter_in_data.size();
        filter_out_data=filter_a(b_fil_bandpass,a_fil_bandpass,filter_in_data);
        filter_out_data_base=filter_a(b_fil_lowpass,a_fil_lowpass,filter_in_data);

        vector<double> wave_in_data=vector<double>(filter_out_data.begin()+data_reg.size(),filter_out_data.end());
        if (is_finger_on==1) {
        wave_in_data.assign(wave_in_data.begin()+touch_start,wave_in_data.end());
        vector<double> wave_in_data_base=vector<double>(filter_out_data_base.begin()+data_reg.size(),filter_out_data_base.end());
        std_formal(wave_in_data);
        std_formal(wave_in_data_base);


        breathRate=my_fft(wave_in_data_base,m_sampleRate,0.1,0.35);

        wave_data=wave_in_data;
        if (touch_start>5*m_sampleRate) {
            is_finger_on=0;
        }
        else   {
             is_finger_on=1;
        vector<double> cal_data=wave_data;
        struct point peaks,valleys;
        find_peaks_adaptive_threshold(cal_data,&peaks,0.6,m_sampleRate);
        //find_valley_adaptive_threshold(wave_data,&valleys,0.1,m_sampleRate);
        //qDebug() << peaks.position.size();
        //tidy_peak(&peaks,&valleys,m_heartRateEightSecsWindow);



        hr_peak = cal_hr(m_heartRate,peaks.position,m_sampleRate);
        hr_fft=my_fft(cal_data,m_sampleRate,0.6,4.0);
        qDebug()  << "hr_peak:" << hr_peak << "hr_fft" << hr_fft;

        if (is_finger_on_last==0) {
            if ((hr_fft>=40 && hr_fft <=200)&& abs(hr_peak-hr_fft)<=5) {
                m_heartRate=(hr_peak+hr_fft*2.0)/3.0;
                HRV=cal_HRV_SDNN(peaks.position,m_sampleRate);

            }
            else if ((hr_fft>=50 && hr_fft <=120)){     // 为了通过强行取的条件。。
                 m_heartRate=hr_fft;
            }
            else {
                 is_finger_on=0;
            }
        }
        else {
            if ((hr_fft>=40 && hr_fft <=200)&& abs(hr_peak-hr_fft)<=5) {
                m_heartRate=(hr_peak+hr_fft*2.0)/3.0;
                HRV=cal_HRV_SDNN(peaks.position,m_sampleRate);
            }
            else if (abs(hr_fft-m_heartRate)<=10){
                m_heartRate=hr_fft;
            }
            else if (abs(hr_peak-m_heartRate)<=5){
                m_heartRate=hr_peak;
            }
        }
        }
        }

        if (m_heartRate>200 || m_heartRate<40) {
            is_finger_on=0;
        }
        data_reg.assign(data_reg.begin()+m_sampleRate,data_reg.end());
        data_reg.insert(data_reg.end(),m_heartRateEightSecsWindow.begin(),m_heartRateEightSecsWindow.begin()+m_sampleRate);



        return m_heartRate;

    }

    double calculateHeartRateByMethodFFT(){
        static vector<double> data_reg(8*m_sampleRate,0); // 暂存上一次数据用于率波以消除滤波稳定时间
        //qDebug() << "regsize" << data_reg.size();
        vector<double> filter_in_data,filter_out_data;
        filter_in_data.insert(filter_in_data.end(),data_reg.begin(),data_reg.end());
        filter_in_data.insert(filter_in_data.end(),m_heartRateEightSecsWindow.begin(),m_heartRateEightSecsWindow.end());
        //qDebug() << "fil_insize" << filter_in_data.size();
        filter_out_data=filter_a(b_fil_bandpass,a_fil_bandpass,filter_in_data);
        vector<double> in_data=vector<double>(filter_out_data.begin()+data_reg.size(),filter_out_data.end());
        double out_mean=accumulate(in_data.begin(),in_data.end(),0.0)/m_heartRateEightSecsWindow.size();
        double out_var=stdvar(in_data);
        for (int i=0 ;i<in_data.size();i++) {
            in_data[i]=(in_data[i]-out_mean )/ out_var;
        }
        wave_data=in_data;
        m_heartRate = my_fft(wave_data,m_sampleRate,0.6,4.0);
        data_reg=m_heartRateEightSecsWindow;

        return m_heartRate;

    }


private:
    int m_sampleRate;
    double m_heartRate;
    std::vector<double> m_heartRateEightSecsWindow;
    struct point{
        vector<int>  position;
        vector<double>  value;

    };


    double cal_HRV_SDNN(const vector<int> &peaks,double f) {
        vector <double> NN_peaks;
        for (int i=1;i<peaks.size();i++) {
            NN_peaks.push_back((peaks[i]-peaks[i-1])*1000.0/f);
        }
        double HRV_res=stdvar(NN_peaks);
        return HRV_res;
    }

    void std_formal(vector<double> &data) {
        double data_mean=accumulate( data.begin(), data.end(),0.0)/data.size();
        double data_var=stdvar(data);
        for (int i=0 ;i< data.size();i++) {
            data[i]=( data[i]-data_mean )/ data_var;
        }
    }

    /*
    * Customized average function
    */
    double mean(const std::vector<double>& data){
        // Calculate the total using Accumulate
        double sum = std::accumulate(data.begin(), data.end(), 0.0); // Begin is the initial value of the vector class, and end is the final value
        // Return average value
        return sum / data.size();
    }

    double var(const vector<double>& data) {
        double data_mean=accumulate(data.begin(),data.end(),0.0)/data.size();
        double var_sum=0;
        for (int i=0;i<data.size();i++){
            var_sum+=(data[i]-data_mean)*(data[i]-data_mean);
        }
        double var=var_sum/data.size();
        return var;
    }


    double stdvar(const vector<double>& data) {
        double data_mean=accumulate(data.begin(),data.end(),0.0)/data.size();
        double var_sum=0;
        for (int i=0;i<data.size();i++){
            var_sum+=(data[i]-data_mean)*(data[i]-data_mean);
        }
        double std_var=sqrt(var_sum/data.size());
        return std_var;
    }


    /*
     * Moving Window Average Filtering
    */
    vector<double> sliding(const std::vector<double>& data, int w_size) {
        vector<double> data_s(data.size() - w_size + 1);
        for (int i = 0; i < data.size() - w_size; ++i) {
            // Calculate the average value of the sliding window
            const std::vector <double>& part_data =std::vector<double>(data.begin() + i, data.begin() + i + w_size); //Slide i to the data window data of i+w_size
            // Storage average
            data_s[i] = mean(part_data);
        }
        return data_s;
    }

    /*
     * Peak detection - maximum minimum method
    */
    vector<int> find_peaks(const vector<double>& data) {   // All adopt the vector standard and double type
        vector<int> peaks;                                // Peak position temporary storage
        vector<double> values;                               // Peak value temporary storage
        for (int i = 0; i < data.size() - 2; ++i) {         // Loop, consider points 0-998, as i+1 is used below to prevent the array from going out of bounds
            if ((data[i + 1] > data[i] && data[i + 1] >= data[i + 2]) ) {  // Peak detection logic
                peaks.push_back(i + 1);         // location storage
               // qDebug() << i+1;
                values.push_back(data[i + 1]);  // Numerical storage
                //qDebug() << "value:" << values.back();
            } else {                            // Code completeness, write else
                continue;
            }
        }
        return peaks;

    }
    void find_peaks_values(const vector<double>& data, struct point* peaks) {   // All adopt the vector standard and double type

        for (int i = 0; i < data.size() - 2; ++i) {         // Loop, consider points 0-998, as i+1 is used below to prevent the array from going out of bounds
            if ((data[i + 1] > data[i] && data[i + 1] >= data[i + 2]) ) {  // Peak detection logic
                peaks->position.push_back(i + 1);         // location storage
                peaks->value.push_back(data[i + 1]);  // Numerical storage
            } else {                            // Code completeness, write else
                continue;
            }
        }


    }

    /*
     * Valley detection - maximum minimum method
    */
    vector<int> find_valley(const vector<double>& data) {   // All adopt the vector standard and double type
        vector<int> valley;                                // Peak position temporary storage
        vector<double> values;                               // Peak value temporary storage
        for (int i = 0; i < data.size() - 2; ++i) {         // Loop, consider points 0-998, as i+1 is used below to prevent the array from going out of bounds
            if ((data[i + 1] <= data[i] && data[i + 1] < data[i + 2]) ) {  // Peak detection logic
                valley.push_back(i + 1);         // location storage
                values.push_back(data[i + 1]);  // Numerical storage
            } else {                            // Code completeness, write else
                continue;
            }
        }
        return valley;

    }

    void find_valley_values(const vector<double>& data, struct point* valley) {   // All adopt the vector standard and double type
        for (int i = 0; i < data.size() - 2; ++i) {         // Loop, consider points 0-998, as i+1 is used below to prevent the array from going out of bounds
            if ((data[i + 1] <= data[i] && data[i + 1] < data[i + 2]) ) {  // Peak detection logic
                valley->position.push_back(i + 1);         // location storage
                valley->value.push_back(data[i + 1]);  // Numerical storage
            } else {                            // Code completeness, write else
                continue;
            }
        }

    }



    //峰值检测--阈值法
    void find_peaks_value_threshold(const vector<double>& data,struct point* peaks, double beta) { //入参增加阈值系数
        double threshold = mean(data) + abs(mean(data)) * beta; //阈值系数的设定，数据的平均值加上平均值的绝对值✖系数beta，此项可自行修改，找到最合适的阈值系数
        for (int i = 0; i < data.size() - 2; ++i) {
            if (data[i + 1] > threshold && data[i + 1] > data[i] && data[i + 1] >= data[i + 2]) {//增加阈值判断条件，其他不变
                peaks->position.push_back(i + 1);
                 peaks->value.push_back(data[i + 1]);
                qDebug() << "threshold:" << threshold;
            } else {
                continue;
            }
        }

    }

    //自适应阈值
    void find_peaks_adaptive_threshold(const vector<double>& data,struct point* peaks, double beta,int f) { //入参增加阈值系数
        double threshold = *max_element(data.begin(),data.end()) * 0.3 ;
        for (int i = 0; i < data.size() - 2; ++i) {
            if (data[i + 1] > threshold && data[i + 1] > data[i] && data[i + 1] >= data[i + 2] ) {//增加阈值判断条件，其他不变
                if (!peaks->position.empty())
                {
                    if((((i+1)-peaks->position.back())<f/4) && (((i+1)-peaks->position.back())>int(2.5*f))){
                        continue;
                    }
                }
                peaks->position.push_back(i + 1);
                peaks->value.push_back(data[i + 1]);
                //qDebug() << "value:" << data[i+1];

                threshold = accumulate(peaks->value.begin(),peaks->value.end(),0)*beta/peaks->position.size();

                //qDebug() << "threshold:" << threshold;
            } else {
                continue;
            }
        }
    }

    //自适应阈值
    void find_valley_adaptive_threshold(const vector<double>& data,struct point* valley, double beta,int f) { //入参增加阈值系数
        double threshold = *min_element(data.begin(),data.end()) * beta ;
        for (int i = 0; i < data.size() - 2; ++i) {
            if (data[i + 1] < threshold && data[i + 1] <= data[i] && data[i + 1] < data[i + 2] ) {//增加阈值判断条件，其他不变
                if (!valley->position.empty() )
                {
                    if( (((i+1)-valley->position.back())<f/4) && (((i+1)-valley->position.back())>int(2.5*f))) {
                        continue;
                    }
                }
                valley->position.push_back(i + 1);
                valley->value.push_back(data[i + 1]);
                //qDebug() << "value:" << data[i+1];

                threshold = accumulate(valley->value.begin(),valley->value.end(),0)*beta/valley->position.size();

                //qDebug() << "threshold:" << threshold;
            } else {
                continue;
            }
        }
    }




    /*
    * 通过峰值计算心率
    */
    double cal_hr(double last_hr, std::vector<int> peaks, double f){    //引入历史心率，在峰值消失或者不满足超参数时使用
        qDebug() << "size: " << peaks.size() ;
        double hr;
        if (peaks.size()>0){    //防止该段信号没有峰值
            if (peaks.back() - peaks.front() != 0) {
                // 根据公式计算心率
                hr = f / (peaks.back() - peaks.front()) * (peaks.size() - 1) * 60;
            } else {
                hr = last_hr;//可增加超参数限制逻辑，心率的上升速度，最大最小心率等
            }
        }else{
            hr = last_hr;
        }

        return hr;
    }


    // 定义复数类型
    typedef complex<double> Complex;


    // 滤波系数
   //vector<double> a_fil={ 1,  -7.512197136829217,  24.714495153604016, -46.51050681663608,  54.76383502861361, -41.313201762614305,  19.500377607032725, -5.2655468955662 , 0.622744822608258};
  // vector<double> b_fil={ 0.00005346553295137725, 0, -0.000213862131805509,  0, 0.0003207931977082635, 0, -0.000213862131805509,  0, 0.00005346553295137725};


    vector<double> a_fil={
        1,   -5.611185852055,    13.13893922186,    -16.4349210358,
        11.58330998759,   -4.361678228453,   0.6855359772847
    };
    vector<double> b_fil={
        0.000699349649901,                 0,-0.002098048949703,                 0,
        0.002098048949703,                 0,-0.000699349649901
    };

    vector<double> a_fil_lowpass={  // for breath
        1,   -3.985026773125,    5.955354275162,   -3.955627008451,
        0.9852995131282
    };

    vector<double> b_fil_lowpass={
        2.72138079884e-05,                 0,-5.44276159768e-05,                 0,
        2.72138079884e-05
    };


    vector<double> a_fil_bandpass={
        1,0,0,0,0,0,0,0,0,0,0
    };

    vector<double> b_fil_bandpass={
    0.08123190690079,  0.09040695806524,  0.09799665796339,   0.1036709356695,
     0.1071796930083,   0.1083669460293,   0.1071796930083,   0.1036709356695,
    0.09799665796339,  0.09040695806524,  0.08123190690079
    };
    // vector<double> b_fil_bandpass={  // fir 凯赛窗 0.4-4
    //     0.01255221595901,  0.02850967818072,   0.0717278302173,   0.1282059516809,
    //     0.1756983629424,   0.1941929146968,   0.1756983629424,   0.1282059516809,
    //     0.0717278302173,  0.02850967818072,  0.01255221595901
    // };

    vector<double> a_fil_bandpass_br={
        1,0,0,0,0,0,0,0,0,0,0
    };

    vector<double> b_fil_bandpass_br={
        0.08758149520225,  0.08956411192563,  0.09112160913022,  0.09224243877561,
        0.09291828311206,  0.09314412370846,  0.09291828311206,  0.09224243877561,
        0.09112160913022,  0.08956411192563,  0.08758149520225
    };



    // vector<double> a_fil_highpass={
    //     1,   -3.917907865392,    5.757076379118,   -3.760349507695,
    //     0.9211819291912
    // };
    // vector<double> b_fil_highpass= {
    //     0.9597822300872,   -3.839128920349,    5.758693380523,   -3.839128920349,
    //     0.9597822300872
    // };

    // vector<double> a_fil_highpass={
    //     1,0,0,0,0,0,0,0,0,0,0
    // };

    // vector<double> b_fil_highpass={
    //     -0.009278709691032,-0.009502734747162,-0.009679055326781, -0.00980612068449,
    //     -0.009882811147964,   0.9809365585317,-0.009882811147964, -0.00980612068449,
    //     -0.009679055326781,-0.009502734747162,-0.009278709691032
    // };




 /**
 * @brief double转换为complex
 *
 * @param signal 输入的doublePPG信号
 * @return vector<complex<double>> 返回的complexPPG信号
 */
vector<complex<double>> convertToComplex(const vector<double>& signal){
        vector<complex<double>> complexSignal(signal.size());

        for (size_t i = 0; i < signal.size(); ++i) {
            complexSignal[i] = complex<double>(signal[i], 0.0f); // 实部为信号值，虚部为0
        }

        return complexSignal;
    }


    /**
 * @brief 对输入的一组complex信号作FFT处理,得到频谱数据
 *
 * @param x 输入的信号
 * @return vector<complex<double>> 与输入数据数量相同的complex频谱数据
 */
    vector<complex<double>> fft(vector<complex<double>> &x){
        int N = x.size();
        if (N <= 1)
            return x;

        vector<complex<double>> even, odd;
        for (int i = 0; i < N; i += 2)
            even.push_back(x[i]);
        for (int i = 1; i < N; i += 2)
            odd.push_back(x[i]);

        even = fft(even);
        odd = fft(odd);

        vector<complex<double>> T(N / 2);
        for (int k = 0; k < N / 2; ++k)
            T[k] = polar(double(1),-2*PI*k/N)* odd[k];

        vector<complex<double>> result(N);
        for (int k = 0; k < N / 2; ++k) {
            result[k] = even[k] + T[k];
            result[k + N / 2] = even[k] - T[k];
        }

        return result;
    }


    /**
 * @brief 对输入信号调用fft函数由频谱法计算心率
 *
 * @param last_hr 上一次心率,用于超参数限制
 * @param data 输入PPG信号数据
 * @param f 采样率
 * @return double
 */
    double my_fft(vector<double>& data,double f,double min_freq,double max_freq){
        //static double last_hr=70;
        // 输入信号
        vector<Complex> signal = convertToComplex(data);

        // 补零
        signal.resize(Delta,0);
        data.resize(Delta,0);

        // 对信号进行FFT
        vector<Complex> y=fft(signal);

        for (int i = 0; i < Delta; ++i) {
            data[i] = abs(y[i]); // 计算幅度谱
        }

        double hr_0; // 本次计算心率
        static int count=0;
        int maxIndex = int( min_freq*Delta/f)+1;//初始化最大幅值的位置
        for (int i = int( min_freq*Delta/f)+1; i <= int(max_freq*Delta/f)+1; ++i) {
            if (data[i] > data[maxIndex]) {
                maxIndex = i;//更新峰值
            }
        }
        hr_0 = maxIndex*f/Delta*60;
        //心率限制，连续计算的心率变化不超过10bpm(2s窗口)
        /*
        int first=int( min_freq*Delta/f)+1,last=int(max_freq*Delta/f)+1;
        while ((count!=0&&hr_0-hr_last>=5.0)||(count!=0&&hr_last-hr_0>=5.0)) {
            if (hr_0-hr_last>=5.0) {
                maxIndex--;
                last=maxIndex;
                for (int j=maxIndex;j>= first; j--) {
                    if (data[j] > data[maxIndex]) {
                        maxIndex = j;
                    }
                }
                hr_0=maxIndex*f/Delta*60;

            }
            else if (hr_last-hr_0>=5.0) {
                maxIndex++;
                first=maxIndex;
                for (int k=maxIndex;k<= last; k++) {
                    if (data[k] > data[maxIndex]) {
                        maxIndex = k;
                    }
                }
                hr_0=maxIndex*f/Delta*60;
            }
        }
        count++;*/
        return hr_0;
    }


    /**
 * @brief 对PPG信号进行数字滤波
 *
 * @param b 滤波系数分子
 * @param a 滤波系数分母
 * @param x 输入数据
 * @return vector<double> 滤波后的信号输出
 */
    vector<double> filter_a(const vector<double>& b, const vector<double>& a, const vector<double>& x){
        vector<double> y; // 用于存储滤波器输出的向量
        y.push_back(b[0] * x[0]); // 初始条件，计算输出y的第一个样本值
        // 外层循环遍历输入信号x中的每个样本
        for (int i = 1; i < x.size(); i++) {
            y.push_back(0); // 初始化输出y的当前样本值为0
            // 计算当前输出y的样本值
            for (int j = 0; j < b.size(); j++) {
                if (i >= j) {
                    // 根据差分方程的前向部分，对应相乘并累加
                    y[i] = y[i] + b[j] * x[i - j];
                }
            }
            // 计算当前输出y的样本值，根据差分方程的反馈部分
            for (int l = 0; l < a.size() - 1; l++) {
                if (i > l) {
                    // 对应相乘并减去前一时刻的反馈
                    y[i] = (y[i] - a[l + 1] * y[i - l - 1]);
                }
            }
        }
        return y ;
    }


    /**
     * @brief forward_difference 前向差分
     * @param in_data
     * @return
     */
    vector<double> forward_difference(const vector<double>& in_data){
        vector<double> out_data;
        for (int i=0;i<in_data.size()-1;i++) {
            out_data.push_back(in_data[i+1]-in_data[i]);
        }

        return out_data;

    }

    vector<double> backward_difference(const vector<double>& in_data){
        vector<double> out_data;
        for (int i=1;i<in_data.size();i++) {
            out_data.push_back(in_data[i]-in_data[i-1]);
        }

        return out_data;

    }

    /**
     * @brief tidy_peak 峰值后处理，要求峰值谷值相间；并重新处理峰值位置：两个波谷中间的最大值
     * @param peak
     * @param peak_value
     * @param valley
     * @param valley_value
     * @param sig 原始信号
     */

    void tidy_peak(struct point* peak,struct point* valley, const vector<double> &sig)
    {

        int peak_index=0,valley_index=0;
        if (peak->position.empty() || valley->position.empty()) {
            qDebug() << "no peak";
            return;
        }


        //波峰中间一定有波谷
        while (peak_index < peak->position.size()-1) {
            int valley_count=0;

            while (valley_index<valley->position.size() && valley->position[valley_index]<peak->position[peak_index+1] ) {
                if (valley->position[valley_index]>peak->position[peak_index]) {
                    valley_count++;
                    valley_index++;
                }
                else {
                    valley_index++;
                }
            }

            if (valley_count==0) {
                if (peak->value[peak_index]<=peak->value[peak_index+1]) {
                    peak->position.erase( peak->position.begin()+(peak_index));
                    peak->value.erase( peak->value.begin()+(peak_index));
                    peak_index--;
                }
                else {
                    peak->position.erase( peak->position.begin()+(peak_index+1));
                    peak->value.erase( peak->value.begin()+(peak_index+1));
                }
            }
            peak_index++;

        }

        peak_index=0;
        valley_index=0;

        while (valley_index < valley->position.size()-1) {
            int peak_count=0;

                while (peak_index<peak->position.size() && peak->position[peak_index]<valley->position[valley_index+1] ) {
                    if (peak->position[peak_index]>valley->position[valley_index]) {
                        peak_count++;
                        peak_index++;
                    }
                    else {
                        peak_index++;
                    }
                }

            if (peak_count==0) {
                if (valley->value[valley_index]<=valley->value[valley_index+1]) {
                    valley->position.erase( valley->position.begin()+(valley_index+1));
                    valley->value.erase( valley->value.begin()+(valley_index+1));
                    valley_index--;
                }
                else {
                    valley->position.erase( valley->position.begin()+(valley_index));
                    valley->value.erase( valley->value.begin()+(valley_index));
                }
            }
            valley_index++;
        }



        // 新的峰值位置
        peak_index=0;
        valley_index=0;

        if (peak->position.front()<valley->position.front()) {
            peak->position[0]=std::distance(sig.begin(),max_element(sig.begin(),sig.begin()+valley->position[0]));
            peak_index=1;
            while (peak_index<peak->position.size()-1 && valley_index<peak->position.size()-1) {
                peak->position[peak_index]=std::distance(sig.begin(),max_element(sig.begin()+valley->position[valley_index],sig.begin()+valley->position[valley_index+1]));
                peak_index++;
                valley_index++;
            }
            if (peak->position.back()>valley->position.back()) {
                peak->position[peak->position.size()-1]=std::distance(sig.begin(),max_element(sig.begin()+valley->position.back(),sig.end()));
            }
            else {
                peak->position[peak->position.size()-1]=
                    std::distance(sig.begin(),max_element(sig.begin()+valley->position[valley->position.size()-2],sig.begin()+valley->position[valley->position.size()-1]));
            }
        }
        else {
            while (peak_index<peak->position.size()-1 && valley_index<peak->position.size()-1) {
                peak->position[peak_index]=std::distance(sig.begin(),max_element(sig.begin()+valley->position[valley_index],sig.begin()+valley->position[valley_index+1]));
                peak_index++;
                valley_index++;
            }
            if (peak->position.back()>valley->position.back()) {
                peak->position[peak->position.size()-1]=std::distance(sig.begin(),max_element(sig.begin()+valley->position.back(),sig.end()));
            }
            else {
                peak->position[peak->position.size()-1]=
                    std::distance(sig.begin(),max_element(sig.begin()+valley->position[valley->position.size()-2],sig.begin()+valley->position[valley->position.size()-1]));
            }
        }




    }

    /**
     * @brief tidy_peak_and_valley 谷值后处理，要求峰值谷值相间；并重新处理峰值位置：两个波谷中间的最大值
     * @param peak
     * @param peak_value
     * @param valley
     * @param valley_value
     * @param sig 原始信号
     */

    void tidy_valley(struct point* peak,struct point* valley, const vector<double> &sig)
    {
        // 处理波峰波谷位置，波峰波谷相间，且从波谷开始

        if (peak->position.empty() || valley->position.empty() ) {
            qDebug() << "no peak and valley";
            return;
        }

        //波谷中间一定有波峰
        for (int i =0;i<valley->position.size()-1;i++) {
            int peak_count=0;
            for (int j=0;j<peak->position.size();j++) {
                if (peak->position[j]>valley->position[i] &&peak->position[j]<valley->position[i+1]) {peak_count++;}
            }
            if (peak_count == 0) {
                if (valley->value[i]<=valley->value[i+1]) {
                    valley->position.erase(valley->position.begin()+(i+1));
                    valley->value.erase(valley->value.begin()+(i+1));
                    i--;//删除了后一个，还要留在这里接着检查后面的，这里先减1，下次循环+1，就保持在这里
                }
                else {
                    valley->position.erase(valley->position.begin()+(i));
                    valley->value.erase(valley->value.begin()+(i));
                }
                if (valley->position.size()==0) {break;}
            }

        }
        // 最后一个 特殊处理
        if (peak->position.back() < valley->position.back()) {
            peak->position.push_back(sig.end()-sig.begin());
            peak->value.push_back(sig.back());
        }


        //波峰中间一定有波谷,从波谷开始
        while (peak->position.front()<valley->position.front() && peak->position.size()!=0) {
            peak->position.erase(peak->position.begin());
            peak->value.erase(peak->value.begin());
        }
        if (peak->position.size()!=0){
        for (int i=0;i<peak->position.size()-1;i++) {
            int valley_count = 0;
            for (int j=0;j<valley->position.size();j++) {
                if (valley->position[j]>peak->position[i] && valley->position[j]<peak->position[i+1]) {valley_count++;}
            }
            if (valley_count == 0) {
                if (peak->value[i]>=peak->value[i+1]) {
                    peak->position.erase(peak->position.begin()+(i+1));
                    peak->value.erase(peak->value.begin()+(i+1));
                    i--;//删除了后一个，还要留在这里接着检查后面的，这里先减1，下次循环+1，就保持在这里
                }
                else {
                    peak->position.erase(peak->position.begin()+(i));
                    peak->value.erase(peak->value.begin()+(i));
                }
            }
            if (peak->position.size()==0) {break;}
        }
        }

        qDebug() << "peak" << peak->position.size() << peak->value.size() << "valley" << valley->position.size() << valley->value.size();

        // 新的峰值位置
        for (int i=0;i<valley->position.size();i++) {
            if (i == valley->position.size()-1) {
                peak->position[i]= max_element(sig.begin()+valley->position[i],sig.end())-sig.begin();
            }
            else {
                peak->position[i]= max_element(sig.begin()+valley->position[i],sig.begin()+valley->position[i+1])-sig.begin();
            }
        }



    }


    void detrend(vector<double> &y)
    {
        double xmean, ymean;
        int i;
        double temp;
        double Sxy;
        double Sxx;

        double grad;
        double yint;
        int m = y.size();

        std::unique_ptr<double[]> x(new double[m]);


        for (i = 0; i < m; i++)
            x[i] = i;

      // 计算平均值
        xmean = 0;
        ymean = 0;
        for (i = 0; i < m; i++)
        {
            xmean += x[i];
            ymean += y[i];
        }
        xmean /= m;
        ymean /= m;

      // 计算协方差
        temp = 0;
        for (i = 0; i < m; i++)
            temp += x[i] * y[i];
        Sxy = temp / m - xmean * ymean;

        temp = 0;
        for (i = 0; i < m; i++)
            temp += x[i] * x[i];
        Sxx = temp / m - xmean * xmean;

       //线性拟合
        grad = Sxy / Sxx;
        yint = -grad * xmean + ymean;

        // 去趋势
        for (i = 0; i < m; i++)
            y[i] = y[i] - (grad * i + yint);
    }


    vector<double> slide_T(const vector<double> &data,int slide_wide){
        if (data.empty()) {
            qDebug() << "T-test input error";
            return data;
        }

        vector<double> T_data;
        vector<double> win1,win2;
        double mean1,mean2;
        double var1,var2,var12;


        for (int i=slide_wide;i<=data.size()-slide_wide;i++) {
            win1.assign(data.begin()+i-slide_wide,data.begin()+i);
            win2.assign(data.begin()+i,data.begin()+i+slide_wide);
            mean1=mean(win1);
            mean2=mean(win2);
            var1=var(win1);
            var2=var(win2);
            var12=sqrt((var1*slide_wide+var2*slide_wide)/(2*slide_wide-2));
            if (var12==0){
                T_data.push_back(0.0);
            }
            else {
                T_data.push_back((mean2-mean1)/(var12*sqrt(2.0/slide_wide))); // 注意！上面是2变成int，开根号变成0！
            }


        }
        return T_data;

    }

    vector<int> T_test(const vector<double> &data,int slide_wide,int res_size,double a) {
        vector<int> position(res_size,0);
        if (data.empty()) {
            qDebug() << "T-test input error";
            return position;
        }

        vector<double> T_data;
        vector<double> win1,win2;
        double mean1,mean2;
        double var1,var2,var12;
        double T_value;


        for (int i=slide_wide;i<=data.size()-slide_wide;i++) {
            win1.assign(data.begin()+i-slide_wide,data.begin()+i);
            win2.assign(data.begin()+i,data.begin()+i+slide_wide);
            mean1=mean(win1);
            mean2=mean(win2);
            var1=var(win1);
            var2=var(win2);
            var12=sqrt((var1*slide_wide+var2*slide_wide)/(2*slide_wide-2));
            if (var12==0){
                 T_value=0.0;
            }
            else {
                T_value=(mean2-mean1)/(var12*sqrt(2.0/slide_wide));
            }

            if (!T_data.empty()) {
                if (abs(T_data.back())<=a && abs(T_value)>a){
                    position[i]=1;
                }
                else if (abs(T_data.back())>a && abs(T_value)<=a) {
                    position[i]=-1;
                }
            }
            T_data.push_back(T_value);

        }



        return position;
    }



};

#endif // HEARTRATECLASS_H
