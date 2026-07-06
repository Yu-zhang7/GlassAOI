#include "DefectInfoItem.h"
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qmenu.h>
#include <qaction.h>
#include <qvariant.h>
#include <qgraphicssceneevent.h>
#include "GraphicsView.h"
#include "GeneralMethod.h"
#include "Log.hpp"
#include "TypeAutoRegister.h"


DefectInfoItem::DefectInfoItem(QString& info, QRect& area, QUuid& uuid, QGraphicsItem* parent) :
    m_defectInfo(info),
    m_defectArea(area),
    m_isLevelShow(true),
    m_isTypeShow(true),
    m_uuid(uuid),
    QGraphicsItem(parent)
{

    m_SideLength = 24;
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::RightButton);
    //setFlag(ItemIgnoresTransformations, true);  //使Item在场景中保持固定大小，不受视图缩放的影响
}

DefectInfoItem::~DefectInfoItem()
{
    //std::cout<< "DefectInfoItem::~DefectInfoItem()" << std::endl;
}

qreal DefectInfoItem::getTotalScale() const
{
    // 获取视图的缩放因子
    qreal viewScale = 1.0;
    if (scene() && !scene()->views().isEmpty()) {
        QGraphicsView* view = scene()->views().first();
        viewScale = view->transform().m11();
    }

    // 获取父项的缩放因子
    qreal parentScale = 1.0;
    if (parentItem()) {
        parentScale = parentItem()->scale();
    }

    // 计算总缩放因子（视图缩放 × 父项缩放）
    return viewScale * parentScale;
}

bool DefectInfoItem::contains(const QPointF& point) const
{
    // 获取实际的绘制区域（考虑缩放后的固定大小）
    QRectF actualRect = boundingRect();

    //// 如果需要更精确，可以缩小点击区域
    //qreal margin = m_SideLength * 0.1; // 10% 的边距
    //actualRect.adjust(margin, margin, -margin, -margin);

    // 检查点是否在实际绘制区域内
    return actualRect.contains(point);
}

QPainterPath DefectInfoItem::shape() const
{
    //QPainterPath path;
    //path.addRect(m_defectArea);
    QPainterPath path;
    path.addRect(boundingRect());
    //QFont font;
    //QFontMetrics metrics(font);
    //QRect textRect = metrics.boundingRect(m_defectInfo);
    //path.addRect(textRect);
    return path;
}

////QRectF DefectInfoItem::boundingRect() const
////{
////    //QFont font;
////    //QFontMetrics metrics(font);
////    //QRect textRect = metrics.boundingRect(m_defectInfo);
////
////    //// 计算文本的位置，使其位于 m_defectArea 的下方
////    //QPointF textPos(m_defectArea.left(), m_defectArea.bottom() + metrics.height() + 10);
////    //textRect.moveTo(textPos.toPoint());
////
////    //// 返回包含 m_defectArea 和文本矩形的边界矩形
////    ////QRect temp = QRect(m_defectArea.x(), m_defectArea.y(), m_SideLength, m_SideLength);
////
////    //return QRectF(m_defectArea.x(), m_defectArea.y(), m_SideLength, m_SideLength);
////    ////return temp;
////    ////return m_defectArea.united(temp);
////
////
////    // 计算缺陷矩形的中心点
////    QPointF center(m_defectArea.x() + m_defectArea.width() / 2,
////                   m_defectArea.y() + m_defectArea.height() / 2);
////
////    // 返回以中心点为中心，边长为 m_SideLength 的正方形
////    return QRectF(center.x() - m_SideLength / 2,
////                  center.y() - m_SideLength / 2,
////                  m_SideLength, m_SideLength);
////}

QRectF DefectInfoItem::boundingRect() const
{
    qreal totalScale = getTotalScale();
    qreal effectiveSideLength = m_SideLength;

    // 只有在有缩放时才调整碰撞区域大小
    if (totalScale > 0.001) { // 避免除零和极小值
        effectiveSideLength = m_SideLength / totalScale;
    }

    // 计算缺陷矩形的中心点
    QPointF center(m_defectArea.x() + m_defectArea.width() / 2,
        m_defectArea.y() + m_defectArea.height() / 2);

    // 返回以中心点为中心，考虑缩放的有效正方形
    return QRectF(center.x() - effectiveSideLength / 2,
        center.y() - effectiveSideLength / 2,
        effectiveSideLength, effectiveSideLength);
}

void DefectInfoItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 保存原始变换状态
    painter->save();

    // 计算总缩放因子
    qreal totalScale = getTotalScale();

    // 计算反向缩放比例，抵消总缩放
    qreal fixedScale = 1.0 / totalScale;

    // 计算图例中心点
    QPointF center = boundingRect().center();

    // 应用变换：先平移到中心点，缩放，再平移回来
    painter->translate(center);
    painter->scale(fixedScale, fixedScale);
    painter->translate(-center);

    painter->setPen(m_pen);
    painter->setBrush(QBrush(m_pen.color()));
    this->setOpacity(0.99);

    // 这里要根据不同的缺陷类型来绘制不同的形状
    this->DrawShape(*painter);

    // 恢复原始变换状态
    painter->restore();
}

void DefectInfoItem::DrawShape(QPainter& painter)
{
    //这里要根据不同的缺陷类型来绘制不同的形状
    QVariant var = this->data(0);
    if (var.isNull())
    {
        // 计算缺陷矩形的中心点
        QPointF center(m_defectArea.x() + m_defectArea.width() / 2,
                       m_defectArea.y() + m_defectArea.height() / 2);

        // 绘制以中心点为中心的正方形
        QRectF cvRect(center.x() - m_SideLength / 2,
                      center.y() - m_SideLength / 2,
                      m_SideLength, m_SideLength);
        painter.drawRect(cvRect);
        return;
    }
    auto info = var.value<drawInformation>();
    auto type = info.DefectType;

 // 计算缺陷矩形的中心点
    QPointF center(m_defectArea.x() + m_defectArea.width() / 2, 
                   m_defectArea.y() + m_defectArea.height() / 2);
    
    // 创建以中心点为中心的正方形
    QRectF fixedRect(center.x() - m_SideLength / 2, 
                     center.y() - m_SideLength / 2, 
                     m_SideLength, m_SideLength);

    cv::Rect cvRect(fixedRect.x(),
                    fixedRect.y(),
                    fixedRect.width(),
                    fixedRect.height());

    //这里获取父对象，并获取宽和高
    auto parentItem = this->parentItem();
    auto pixItem = dynamic_cast<ImageWidget*>(parentItem);
    if (pixItem == nullptr)
    {
        painter.drawRect(m_defectArea);
        return;
    }

    QPixmap pixmap = pixItem->GetPixmap();
    TypeAutoRegister::Instance().SetWH(pixmap.width(), pixmap.height());
    std::vector<cv::Point> points;
    TypeAutoRegister::Instance().Parse(type, cvRect, points);
    //if (!points.empty())
    //{
        QPolygon polygon;
        for (auto& point : points)
        {
            polygon << QPoint(point.x, point.y);
        }
        QImage temp = DefectTypeToImage(type);
        if (temp.isNull())
        {
            painter.drawPolygon(polygon);
        }
  
        // 缩放图像
        int showSize = temp.width();
        //float scaleShow = SCALESHOW;//图例相对于整张图的缩放

        //if (pixmap.width() < pixmap.height()) {
        //    showSize = static_cast<int>(pixmap.width() * scaleShow);
        //}
        //else {
        //    showSize = static_cast<int>(pixmap.height() * scaleShow);
        //}
        if (showSize > DefectMarkerSize_Max)
        {
            showSize = DefectMarkerSize_Max;
        }
        if (showSize < DefectMarkerSize_Min)
        {
            showSize = DefectMarkerSize_Min;
        }
        temp = temp.scaled(m_SideLength, m_SideLength, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QImage BackgroundImage(temp.size(), QImage::Format_ARGB32);
        if (info.ErrorType == DefectLevel::MINOR)
        {
            BackgroundImage.fill(Qt::green); // 填充绿色背景
        }
        else if (info.ErrorType == DefectLevel::MEDIUM)
        {
            BackgroundImage.fill(Qt::yellow); // 填充黄色背景
        }
        else if (info.ErrorType == DefectLevel::SERIOUS)
        {
            BackgroundImage.fill(Qt::red); // 填充红色背景
        }
        else
        {
            BackgroundImage.fill(Qt::green); // 填充绿色背景
        }
        // 将原图绘制到红色背景上（透明区域自动被红色覆盖）
        QPainter m_painter(&BackgroundImage);
        m_painter.drawImage(0, 0, temp);
        m_painter.end();

        painter.drawImage(fixedRect.topLeft(), BackgroundImage);
        //m_SideLength = BackgroundImage.height();

        int newSideLength = BackgroundImage.height();
        if (newSideLength != m_SideLength) 
        {
            prepareGeometryChange();//必须调用!
            m_SideLength = newSideLength;

            m_defectArea = fixedRect.toRect(); // 更新缺陷区域为固定大小的矩形
        }
}

void DefectInfoItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
#if 0   //20251213: 按照团队讨论，暂时取消图例的右键菜单功能
    QMenu menu;
    QAction* editAction     = menu.addAction(tr("Edit"));
    QAction* deleteAction   = menu.addAction(tr("Delete"));
    
    //获取选择的内容
    qDebug() << "menu exec begin";
    QAction* selectedAction = menu.exec(event->screenPos());
    qDebug() << "menu exec over";
    if (selectedAction == editAction)
    {
        //获取Data
        DefectLevel level;
        DefectType  type;
        QVariant var = this->data(0);
        if (var.isNull())
        {
            level   = DefectLevel::ABNORMAL;
            type    = DefectType::TYPE_POORCOATING;
        }
        else
        {
            auto defectInfo = var.value<drawInformation>();
            level   = defectInfo.ErrorType;
            type    = defectInfo.DefectType;
        }

        //创建模态对话框
        ChangeDefectInfoDialog dialog(level, type);
        if (dialog.exec() == QDialog::Accepted)
        {
            level   = dialog.GetDefectLevel();
            type    = dialog.GetDefectType();

            //看是否要重新存到drawInfo中
            auto views = this->scene()->views();
            if (!views.empty() && views.size() == 1)
            {
                //将第一个QGraphicsViews*强制转换为GraphicsView*
                auto view = views.at(0);
                GraphicsView* graphicsView = dynamic_cast<GraphicsView*>(view);
                if (graphicsView != nullptr)
                {
                    //这里修改查找表
                    if (!graphicsView->EditLut(this, level, type,false))
                    {
                        //打印日志
                        FILE_LOG_INFO("Edit Lut Failed! Please check!");
                        return;
                    }
                }
                else
                {
                    //dynamic_cast转换失败
                    FILE_LOG_ERROR("cast to GraphicsView Failed!");
                    return;
                }
            }
            else
            {
                //获取view失败
                FILE_LOG_ERROR("Get item view Failed!");
                return;
            }

            //重新绘制
            QPen pen;
            pen.setColor(DefectLevelToColor(level));
            m_pen = pen;

            m_defectInfo = DefectTypeToString(type);

            emit Signal_requestUpdate(m_uuid, level, type);
            update();
        }
    }
    else if (selectedAction == deleteAction)
    {
        //删除内容，目前直接不显示
        setVisible(false);

        //获取Data
        DefectLevel level;
        DefectType  type;
            level = DefectLevel::ABNORMAL;
            type = DefectType::TYPE_POORCOATING;

        auto views = this->scene()->views();
        if (!views.empty() && views.size() == 1)
        {
            //将第一个QGraphicsViews*强制转换为GraphicsView*
            auto view = views.at(0);
            GraphicsView* graphicsView = dynamic_cast<GraphicsView*>(view);
            if (graphicsView != nullptr)
            {
                //这里修改查找表.最后一个参数为true表示从查找表中删除
                if (!graphicsView->EditLut(this, level, type, true))
                {
                    //打印日志
                    FILE_LOG_INFO("Edit Lut Failed! Please check!");
                    return;
                }
            }
            else
            {
                //dynamic_cast转换失败
                FILE_LOG_ERROR("cast to GraphicsView Failed!");
                return;
            }
        }
        RemoveFromScene();  // 删除信号
    }
#endif
}

void DefectInfoItem::RemoveFromScene()
{
    if (scene()) {
        scene()->removeItem(this);
    }

    emit Signal_requestRemove(m_uuid);  // 发送信号请求删除数据
    this->deleteLater();
}

QUuid DefectInfoItem::GetUuid()
{
    return m_uuid;
}
