#pragma once
#include <QWidget>
#include <QtCharts>
#include <QVector>
#include <QMap>
#include <QMutex>

#include "Global.h"

/*******************************************示例
// 创建图表（不传入数据）
PieChartWidget* pieChart = new PieChartWidget(parent);
BarChartWidget* barChart = new BarChartWidget(parent);

// 稍后更新数据
pieChart->updateData(countMap);
barChart->updateData(countMap);

// 清空数据
pieChart->clearData();
barChart->clearData();
************************************************/

class PieChartWidget : public QWidget {
    Q_OBJECT

private:
    QtCharts::QPieSeries* series;
    QtCharts::QChart* chart;
    QtCharts::QChartView* chartView;

public:
    PieChartWidget(QWidget* parent = nullptr)
        : QWidget(parent), series(nullptr), chart(nullptr), chartView(nullptr) {
        initializeChart();
    }

    // 更新图表数据
    void updateData(const QMap<DefectType, int>& countMap) {
        if (!series) return;

        series->clear();

        // 计算总数
        int total = 0;
        for (auto it = countMap.begin(); it != countMap.end(); ++it) {
            DefectType type = it.key();
            if (m_defectTypeVisibility[type]) { // 只统计显示的类型
                total += it.value();
            }
        }

        // 添加数据到饼状图
        for (auto it = countMap.begin(); it != countMap.end(); ++it) {
            DefectType type = it.key();
            int count = it.value();

            // 如果该类型不显示，跳过
            if (!m_defectTypeVisibility[type]) {
                continue;
            }

            // 安全获取缺陷类型名称
            QString typeName;
            m_DefectNamesMutex.lock();
            typeName = DefectTypeToString(type);
            m_DefectNamesMutex.unlock();

            double percentage = total > 0 ? (count * 100.0) / total : 0;
            QString label = QString("%1: %2%").arg(typeName).arg(percentage, 0, 'f', 2);

            QtCharts::QPieSlice* slice = series->append(label, count);
            slice->setLabelVisible(true);
        }
    }

    // 清空图表数据
    void clearData()
    {
        if (series)
        {
            series->clear();
        }
    }

private:
    void initializeChart()
    {
        // 创建布局
        QVBoxLayout* layout = new QVBoxLayout(this);

        // 创建饼状图系列
        series = new QtCharts::QPieSeries();

        // 创建图表并添加系列
        chart = new QtCharts::QChart();
        chart->addSeries(series);
        chart->setTitle(tr("Defect Category Analysis Pie Chart"));
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);

        // 设置图表背景色
        chart->setBackgroundBrush(QBrush(QColor("#C3C3C3")));
        chart->setPlotAreaBackgroundBrush(QBrush(QColor("#C3C3C3")));
        chart->setPlotAreaBackgroundVisible(true);

        // 创建图表视图并设置图表
        chartView = new QtCharts::QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);

        // 将图表视图添加到布局
        layout->addWidget(chartView);
        setLayout(layout);
    }
};

class BarChartWidget : public QWidget {
    Q_OBJECT

private:
    QHorizontalBarSeries* series;
    QBarSet* barSet;
    QChart* chart;
    QChartView* chartView;
    QBarCategoryAxis* axisY;
    QValueAxis* axisX;
    QStringList categories;
    QVector<DefectType> m_defectTypes; // 存储按名称顺序排列的缺陷类型

public:
    BarChartWidget(QWidget* parent = nullptr)
        : QWidget(parent), series(nullptr), barSet(nullptr),
        chart(nullptr), chartView(nullptr), axisY(nullptr), axisX(nullptr)
    {


        initializeChart();
    }

    void updateAxis() {
        // 锁定并更新 categories 和 m_defectTypes
        m_DefectNamesMutex.lock();
        categories.clear();
        m_defectTypes.clear();

        for (int i = 0; i < RecipeInfo.defects.size(); ++i) {
            DefectType type = static_cast<DefectType>(RecipeInfo.defects[i].id);
            if (m_defectTypeVisibility[type]) {
                categories << DefectTypeToString(type);
                m_defectTypes << type;
            }
        }
        m_DefectNamesMutex.unlock();

        // 重新设置 Y 轴
        if (axisY) {
            axisY->clear();
            axisY->append(categories);
        }

        // 重置 barSet 数据
        if (barSet) {
            barSet->remove(0, barSet->count());
            // 注意：这里只是清空，数据会在 updateData 中重新填充
        }

        // 可选：重新设置 X 轴范围
        if (axisX) {
            axisX->setRange(0, 40); // 初始值，updateData 会再调整
        }
    }


    // 更新图表数据
    void updateData(const QMap<DefectType, int>& countMap) {
        if (!barSet) return;

        // 清空现有数据
        barSet->remove(0, barSet->count());

        // 更新Y轴（只显示可见的类型）
        updateAxis();

        // 按照m_defectTypes的顺序填充数据
        for (const auto& defectType : m_defectTypes) {
            int count = countMap.value(defectType, 0);
            *barSet << count;
        }

        // 更新坐标轴范围
        int maxValue = 0;
        for (int i = 0; i < barSet->count(); ++i) {
            maxValue = qMax(maxValue, static_cast<int>(barSet->at(i)));
        }
        int extendedMax = (maxValue == 0) ? 40 : qMax(maxValue + 10, 40);

        if (axisX) {
            axisX->setRange(0, extendedMax);
        }
    }

    // 清空图表数据
    void clearData() {
        if (barSet) {
            barSet->remove(0, barSet->count());

            // 重置坐标轴范围
            if (axisX) {
                axisX->setRange(0, 40);
            }
        }
    }

private:
    void initializeChart() {
        QVBoxLayout* layout = new QVBoxLayout(this);

        series = new QHorizontalBarSeries();
        barSet = new QBarSet(tr("Defect Count"));
        series->append(barSet);

        chart = new QChart();
        chart->addSeries(series);
        chart->setTitle(tr("Defect Distribution (Horizontal Bar)"));
        chart->setAnimationOptions(QChart::SeriesAnimations);

        axisX = new QValueAxis();
        axisX->setLabelFormat("%d");
        axisX->setTitleText(tr("Quantity"));
        axisX->setRange(0, 40);
        axisX->setTickCount(6);

        axisY = new QBarCategoryAxis();
        axisY->setTitleText(tr("Defect Type"));
        axisY->setLabelsFont(QFont("Microsoft Yahei", 9));

        series->setLabelsVisible(false);
        chart->setMargins(QMargins(20, 10, 50, 20));

        chart->addAxis(axisY, Qt::AlignLeft);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisY);
        series->attachAxis(axisX);

        chart->legend()->setVisible(false);
        barSet->setColor(QColor(79, 129, 189));
        chart->setBackgroundRoundness(0);

        // 设置图表背景色
        chart->setBackgroundBrush(QBrush(QColor("#C3C3C3")));
        chart->setPlotAreaBackgroundBrush(QBrush(QColor("#C3C3C3")));
        chart->setPlotAreaBackgroundVisible(true);

        chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);

        layout->addWidget(chartView);
        setLayout(layout);

        // 初始化轴
        updateAxis();
    }
};