#include "waveview.h"
#include <iostream>
#include <QtCharts/QScatterSeries>
/*
 *初始化
*/
WaveView::WaveView(QWidget* pParent)
    : QChartView(pParent)
{
    // 创建坐标轴
    m_axisX = new QValueAxis();
    m_axisY = new QValueAxis();
    m_axisX->setTitleText("points(N)");
    m_axisY->setTitleText("ADC RAW");
//    m_axisX->setRange(0,100);
//    m_axisY->setRange(0,10);

    // 创建曲线
    m_lineSeries = new QLineSeries();                               // 创建曲线绘制对象
    m_lineSeries->setVisible(true);                                     // 设置数据点可见

    // 创建chart
    m_chart = new QChart();
    // 添加chart元素
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_chart->addSeries(m_lineSeries);                                   // 将曲线对象添加到图表上
    m_lineSeries->attachAxis(m_axisX);
    m_lineSeries->attachAxis(m_axisY);


    // 创建峰值标记
    m_peakSeries = new QScatterSeries(m_chart);
    m_peakSeries->setPointLabelsVisible(true);
    m_peakSeries->setPointsVisible(true);
    m_peakSeries->setPointLabelsFormat("@yPoint");
    m_peakSeries->setMarkerSize(16); // 设置节点大小
    m_chart -> addSeries(m_peakSeries);

//    m_lineSeries->setUseOpenGL(true);

    this->setChart(m_chart);
    this->setRenderHint(QPainter::Antialiasing);                        // 设置渲染：抗锯齿，如果不设置那么曲线就显得不平滑
//    m_chart->setAnimationOptions(QChart::SeriesAnimations);           // (！！！不要打开！！！)动画：能使曲线绘制显示的更平滑，过渡效果更好看，但可能会使图像卡顿
    m_chart->legend()->hide();

    // 初始化框选区域
    m_selectRectRubberBand = new QRubberBand(QRubberBand::Rectangle,this);
    m_selectRectRubberBand->resize(0,0);
    m_selectRectRubberBand->hide();

    // 设置波形初始长度


    // 暂停标志
    m_pauseFlagBool = false;
    m_autoScaleEnableBool = true;

    //  刷新定时器
    m_axisRescaleTimer = new QTimer(this);
    m_axisRescaleTimer->setInterval(16);
    m_axisRescaleTimer->setSingleShot(false);
    connect(m_axisRescaleTimer, &QTimer::timeout, this, &WaveView::updatePlot);
    m_axisRescaleTimer->start();
}

/*
 * 重新设置波形显示长度
*/
void WaveView::resizeDisplayDataSize(unsigned int newDisplayDataSize){
    // 重新设置缓冲区和曲线长度
    m_waveDisplayDataSize = newDisplayDataSize;
    m_waveDisplayDataQueue.clear();
    QList<QPointF> points;
    for(unsigned int i = 0;i < m_waveDisplayDataSize; i++)
    {
        m_waveDisplayDataQueue.push_back(0);
        points.push_back(QPointF(i,0));
    }
    m_lineSeries->replace(points);
}

/*
 * 将数据添加到缓冲区
 *
*/
void WaveView::pushdata(double y){
    //TODO 空/错误判断
    if(m_pauseFlagBool) return;
    m_waveDisplayDataQueue.dequeue();
    m_waveDisplayDataQueue.enqueue(y);
}

void WaveView::pushpeaks(int x,double y){
    m_peakSeries->append(x,y);
    //qDebug() << x << ',' << y;
}

void WaveView::mousePressEvent(QMouseEvent *pEvent){
//    右键按下时记录起始坐标
    if(pEvent->buttons() & Qt::RightButton){
        m_startPos = pEvent->pos();
        m_selectRectRubberBand->resize(0,0);
        m_selectRectRubberBand->show();
    }

    QChartView::mousePressEvent(pEvent);
}

void WaveView::mouseMoveEvent(QMouseEvent *pEvent){
    // 右键拖动的时候画框
    if(pEvent->buttons() & Qt::RightButton){
        m_selectRectRubberBand->setGeometry(QRect(m_startPos,pEvent->pos()).normalized());
    }

    QChartView::mouseMoveEvent(pEvent);
}

void WaveView::mouseReleaseEvent(QMouseEvent *pEvent){
    if(pEvent->button() == Qt::RightButton){
        // 隐藏矩形框
        m_selectRectRubberBand->hide();

        // 起始点和结束点重合判定
        if(!(m_startPos == pEvent->pos())) m_autoScaleEnableBool = false;   // 没有重合
        else {                                                              // 起点终点重合
            QChartView::mouseReleaseEvent(pEvent);
            m_autoScaleEnableBool = true;                                   // 恢复自动调整坐标范围
            return;
        }

//        std::cout << "start pos: " << m_startPos.x() << ", " << m_startPos.y() << " end pos: " << pEvent->pos().x() << ", " << pEvent->pos().y() << std::endl;
        // 将屏幕坐标点转换为图表的值坐标
        QPointF valuePoint1 = m_chart->mapToValue(m_startPos);
        QPointF valuePoint2 = m_chart->mapToValue(pEvent->pos());

        // 现在 valuePoint1 和 valuePoint2 包含了与坐标轴相关的值

        // 接下来，你可以根据这些值来设置坐标轴的范围或进行其他操作
        qreal minX = std::min(valuePoint1.x(), valuePoint2.x());
        qreal maxX = std::max(valuePoint1.x(), valuePoint2.x());
        qreal minY = std::min(valuePoint1.y(), valuePoint2.y());
        qreal maxY = std::max(valuePoint1.y(), valuePoint2.y());
        // x轴缩放
        m_axisX->setRange(minX, maxX);
//        // y轴缩放
        m_axisY->setRange(minY,maxY);

    }

    QChartView::mouseReleaseEvent(pEvent);
}

void WaveView::mouseDoubleClickEvent(QMouseEvent *pEvent){
    if (pEvent->button() == Qt::LeftButton) {
        // 切换暂停状态
        m_pauseFlagBool = !m_pauseFlagBool;
    }
    QChartView::mouseDoubleClickEvent(pEvent);
}

