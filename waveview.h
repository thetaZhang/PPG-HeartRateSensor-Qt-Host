
#ifndef WAVEVIEW_H
#define WAVEVIEW_H

#include <QChartView>
#include <QMenu>
#include <QValueAxis>
#include <QLineSeries>
#include <QHBoxLayout>
#include <iostream>
#include <QTimer>
#include <QQueue>
#include <QRubberBand>
#include <QtCharts/QScatterSeries>
#include <QDebug>
#include <QtCharts/QChart>
using namespace std;

class WaveView : public QChartView
{
    Q_OBJECT
public:

    WaveView(QWidget* pParent = nullptr);
    void pushdata(double y);
    void pushpeaks(int x,double y);
    void resizeDisplayDataSize(unsigned int newDisplayDataSize);
    void renameaxis(const QString &x,const QString &y){m_axisX->setTitleText(x);m_axisY->setTitleText(y);};
    void setlinered(){m_lineSeries->setPen(QPen(Qt::red,4));}
    //void resetlineformat()
    ~WaveView(){

    }
protected:
    virtual void mousePressEvent(QMouseEvent *pEvent) override;
    virtual void mouseMoveEvent(QMouseEvent *pEvent) override;
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *pEvent) override;

private:
    // 标出峰值
    QScatterSeries* m_peakSeries;

    //曲线，最大最小值，通道是否可见
    QLineSeries* m_lineSeries;
    //qchart
    //坐标轴
    QValueAxis *m_axisX, *m_axisY;
    QChart *m_chart;
    unsigned int m_waveDisplayDataSize;
    QQueue<double> m_waveDisplayDataQueue;
    QTimer *m_axisRescaleTimer;
    bool m_autoScaleEnableBool;
    QRubberBand *m_selectRectRubberBand;
    QPoint m_startPos;
    bool m_pauseFlagBool;
    //数据更新、调整坐标轴范围
    void updatePlot(){
        double minY, maxY;
        // 更新数据

        QList<QPointF> points = m_lineSeries->points();


        minY = maxY = m_waveDisplayDataQueue[0];
        for(unsigned int i = 0;i < m_waveDisplayDataSize; i++)
        {
            points[i].setY(m_waveDisplayDataQueue[i]);
            if(minY > m_waveDisplayDataQueue[i]) minY = m_waveDisplayDataQueue[i];
            if(maxY < m_waveDisplayDataQueue[i]) maxY = m_waveDisplayDataQueue[i];
            //qDebug() << m_waveDisplayDataQueue[i];
        }

        if(m_autoScaleEnableBool == true){
            m_axisY->setRange(minY-1,maxY+1);
            m_axisX->setRange(points.begin()->x(), points.last().x());
        }



        m_lineSeries->replace(points);
    }


};

#endif // WAVEVIEW_H
