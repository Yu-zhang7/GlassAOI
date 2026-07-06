#include "ImageWidget.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QGraphicsSceneDragDropEvent>
#include <QDrag>
#include <math.h>
#include "Log.hpp"

ImageWidget::ImageWidget(QPixmap pixmap):
    m_pix(pixmap)
{
    Init();
}

ImageWidget::ImageWidget(QGraphicsItem* parent) :
    QGraphicsItem(parent)
{
    Init();
}

void ImageWidget::Init()
{


    setAcceptDrops(true);//If enabled is true, this item will accept hover events; otherwise, it will ignore them. By default, items do not accept hover events.
    m_scaleValue = 0;
    m_scaleDafault = 0;
    m_isMove = false;
}

QRectF ImageWidget::boundingRect() const
{
   /* return QRectF(-m_pix.width() / 2, -m_pix.height() / 2,
                  m_pix.width(), m_pix.height());*/

    return QRectF(0, 0, m_pix.width(), m_pix.height());
}

void ImageWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                    QWidget *)
{
    // 添加空图像检查
    if (m_pix.isNull())
    {
        return;
    }
    //painter->drawPixmap(-m_pix.width() / 2, -m_pix.height() / 2, m_pix);
    painter->drawPixmap(0, 0, m_pix);
}

void ImageWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_pix.isNull())
    {
        return;
    }
    if(event->button()== Qt::LeftButton)
    {
        m_startPos = event->pos();//鼠标左击时，获取当前鼠标在图片中的坐标，
        m_isMove = true;//标记鼠标左键被按下
    }
    //else if(event->button() == Qt::RightButton)
    //{
    //    ResetItemPos();//右击鼠标重置大小
    //}
}

void ImageWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_isMouseMove)
    {
        return;
    }
    //qDebug() << "ImageWidger: mouseMoveEvent";

    if (m_pix.isNull())
    {
        return;
    }
    if(m_isMove)
    {
        QPointF point = (event->pos() - m_startPos)*m_scaleValue;
        moveBy(point.x(), point.y());
    }
}

void ImageWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    if (!m_isMouseMove)
    {
        return;
    }
    //qDebug() << "ImageWidger: mouseReleaseEvent";
    if (m_pix.isNull())
    {
        return;
    }
    m_isMove = false;//标记鼠标左键已经抬起
}


void ImageWidget::wheelEvent(QGraphicsSceneWheelEvent *event)//鼠标滚轮事件
{
#if 0
    event->ignore();
#else
    //qDebug() << "ImageWidger: wheelEvent";
    if (m_pix.isNull())
    {
        return; // 空图忽略滚轮事件
    }
    if((event->delta() > 0)&&(m_scaleValue >= 50))//最大放大到原始图像的50倍
    {
        return;
    }
    else if((event->delta() < 0)&&(m_scaleValue <= m_scaleDafault))//图像缩小到自适应大小之后就不继续缩小
    {
        ResetItemPos();//重置图片大小和位置，使之自适应控件窗口大小
    }
    else
    {
        qreal qrealOriginScale = m_scaleValue;
        if(event->delta() > 0)//鼠标滚轮向前滚动
        {
            m_scaleValue*=1.1;//每次放大10%
        }
        else
        {
            m_scaleValue*=0.9;//每次缩小10%
        }
        setScale(m_scaleValue);
        if(event->delta() > 0)
        {
            moveBy(-event->pos().x()*qrealOriginScale*0.1, -event->pos().y()*qrealOriginScale*0.1);//使图片缩放的效果看起来像是以鼠标所在点为中心进行缩放的
        }
        else
        {
            moveBy(event->pos().x()*qrealOriginScale*0.1, event->pos().y()*qrealOriginScale*0.1);//使图片缩放的效果看起来像是以鼠标所在点为中心进行缩放的
        }
    }
#endif
}

void ImageWidget::setQGraphicsViewWH(int nwidth, int nheight)//将主界面的控件QGraphicsView的width和height传进本类中，并根据图像的长宽和控件的长宽的比例来使图片缩放到适合控件的大小
{
    if (m_pix.isNull()) // 检测空pixmap
    {
        m_scaleDafault = 1.0; // 设置安全默认值
        setScale(1.0);
        m_scaleValue = 1.0;
        return;
    }
    int nImgWidth = m_pix.width();
    int nImgHeight = m_pix.height();
    qreal temp1 = nwidth * 1.0/nImgWidth;
    qreal temp2 = nheight * 1.0/nImgHeight;
    if(temp1>temp2)
    {
        m_scaleDafault = temp2;
    }
    else
    {
        m_scaleDafault = temp1;
    }
    setScale(m_scaleDafault);
    m_scaleValue = m_scaleDafault;
}

void ImageWidget::ResetItemPos()//重置图片位置
{
    m_scaleValue = m_scaleDafault;//缩放比例回到一开始的自适应比例
    setScale(m_scaleDafault);//缩放到一开始的自适应大小
    setPos(0,0);
}

qreal ImageWidget::getScaleValue() const
{
    return m_scaleValue;
}

void ImageWidget::SetPixmap(QPixmap pixmap)
{
    if (pixmap.isNull()) {
        return;
    }
    m_pix = pixmap;
    //qDebug() << "ImageWidget : pixmap.size() = " << m_pix.size();
}

void ImageWidget::SetMouseMove(bool isMove)
{
    m_isMouseMove = isMove;
}