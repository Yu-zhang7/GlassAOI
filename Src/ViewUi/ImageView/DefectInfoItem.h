#ifndef DEFECTINFOITEM_H
#define DEFECTINFOITEM_H

#define SCALESHOW 0.05;//图例相对于整张图的缩放

#include <QGraphicsItem>
#include <qstring.h>
#include <qrect.h>
#include <qpen.h>
#include <qpainterpath.h>
#include "GraphicsView.h"
#include "ChangeDefectInfoDialog.h"
#include <QUuid>

class DefectInfoItem  : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
	cv::Mat detect_image_0; //存放缺陷小图图像中的第一个通道图像

    cv::Mat detect_image_1; //存放缺陷小图图像中的第二个通道图像

    cv::Mat detect_image_2; //存放缺陷小图图像中的第三个通道图像
				  
    /* 构造函数 */
    DefectInfoItem(QString& info, QRect& area, QUuid& uuid, QGraphicsItem* parent = nullptr);

    /* 析构函数 */
    ~DefectInfoItem();

    /* QGraphicsItem重写函数，判断鼠标是否在item中 */
    bool contains(const QPointF& point) const override;

    /* QGraphicsItem重写函数，用来告诉Item的范围 */
    QPainterPath shape() const override;

    /* QGraphicsItem重写函数，用来告诉Item的形状 */
    QRectF boundingRect() const override;

    /* 绘图函数，绘制item的具体内容 */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    /* 绘制形状 */
    void DrawShape(QPainter& painter);

    /* QGrahpicsItem中特有的contextMenuEvent事件类型 */
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    /* 从场景中删除指定的单个瑕疵*/
    void RemoveFromScene();

    QRect getRect() { return m_defectArea; }

    /* 设置本Item是否显示 */
    void SetLevelShow(bool isShow)
    {
        m_isLevelShow = isShow;
        this->setVisible(m_isLevelShow && m_isTypeShow);
    }

    void SetTypeShow(bool isShow)
    {
        m_isTypeShow = isShow;
        this->setVisible(m_isLevelShow && m_isTypeShow);
    }

    /* 设置画笔 */
    void setPen(const QPen& pen)
    {
        m_pen = pen;
    }

    /* 获取画笔 */
    QPen pen() const
    {
        return m_pen;
    }

    /* 获得唯一标识*/
   QUuid GetUuid();
signals:
    void Signal_requestRemove(const QUuid& uuid);       // 请求删除指定索引的数据项
    void Signal_requestUpdate(const QUuid& uuid, int newLevel, int newType);  // 请求更新数据项

private:
    // 添加获取总缩放因子的方法
    qreal getTotalScale() const;

private:
    QString         m_defectInfo;   // 缺陷信息
    QRect           m_defectArea;   // 缺陷区域（正方形）
    QVector<QPoint> m_points;       // 缺陷区域（多边形区域使用）
    QPen            m_pen;          // 缺陷区域的画笔（控制颜色）
    bool            m_isLevelShow;  // 等级是否显示
    bool            m_isTypeShow;   // 类型是否显示
    int             m_SideLength;   // 返回的矩形的边长
    QUuid           m_uuid;         // 数据索引

    mutable qreal m_currentViewScale; // 当前视图缩放因子
};


#endif