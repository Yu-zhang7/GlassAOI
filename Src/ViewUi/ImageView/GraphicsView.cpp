#include "GraphicsView.h"
#include <qrect.h>
#include <qlayout.h>
#include <qdialog.h>
#include <QWheelEvent>
#include <QtMath>
#include <qglobal.h>
#include "DefectRectShow.h"
#include "DefectInfoItem.h"
#include "Global.h"
#include "Log.hpp"		
#include <QDebug>

std::unique_ptr<DefectRectShow> m_defectRectShow;

//标尺的刻度和刻度值显示优化
double niceStep(double desiredStep) {
    const double exponents[] = { 1.0, 2.0, 5.0, 10.0 }; // 基准乘数
    double magnitude = std::pow(10.0, std::floor(std::log10(desiredStep)));
    double normalized = desiredStep / magnitude;
    double niceNormalized = 1.0;
    for (double e : exponents) {
        if (normalized <= e) {
            niceNormalized = e;
            break;
        }
    }
    return niceNormalized * magnitude;
}

//获得缺陷级别字符串
std::string GetDefectLevelString(DefectLevel level)
{
    std::string defectLevelStr;
    switch (level)
    {
    case DefectLevel::MINOR:
        defectLevelStr = "MINOR";
        break;
    case DefectLevel::MEDIUM:
        defectLevelStr = "MEDIUM";
        break;
    case DefectLevel::SERIOUS:
        defectLevelStr = "SERIOUS";
        break;
    case DefectLevel::NORMAL:
        defectLevelStr = "NORMAL";
        break;
    default:
        defectLevelStr = "NORMAL";
        break;
    }
    return defectLevelStr;
}

//获得缺陷类型对应的字符串
std::string GetDefectTypeString(DefectType type)
{ 
    std::string defectTypeStr;
    switch (type)
    {
    case DefectType::TYPE_POORCOATING:
        defectTypeStr = "POORCOATING";
        break;
    case DefectType::TYPE_SCRATCH:
        defectTypeStr = "SCRATCH";
        break;
    case DefectType::TYPE_CALCULUS:
        defectTypeStr = "CALCULUS";
        break;
    case DefectType::TYPE_BUBBLE:
        defectTypeStr = "BUBBLE";
        break;
    case DefectType::TYPE_TRADEMARK:
        defectTypeStr = "TRADEMARK";
        break;
    case DefectType::TYPE_WATERSTAIN:
        defectTypeStr = "WATERSTAIN";
        break;
    case DefectType::TYPE_SMUDGE:
        defectTypeStr = "SMUDGE";
        break;
    case DefectType::TYPE_SCREENPRINTING:
        defectTypeStr = "SCREENPRINTING";
        break;
    default:
        defectTypeStr = "NORMAL";
        break;
    }
    return defectTypeStr;
}

// 自定义颜色优先级函数
int colorPriority(const QColor& color)
{
    if (color == QColor(0, 255, 0)) return 0;     // 绿色
    if (color == QColor(255, 255, 0)) return 1;   // 黄色
    if (color == QColor(255, 0, 0)) return 2;     // 红色
    return 3; // 其他颜色排最后
}

GraphicsView::GraphicsView(QWidget* parent) :
    QGraphicsView(parent), 
    m_pixItem(nullptr),
    m_isChanged(false),
    m_pixelXmm(0.109977),
    m_pixelYmm(0.198901),
    m_physicalUnit("mm"),
    m_primaryKey("")
{
    m_pixelXmm = Pixle2MM_X;
    m_pixelYmm = Pixle2MM_Y;

    setRenderHint(QPainter::Antialiasing);          //设置抗锯齿，使绘制更加平滑
    setCacheMode(QGraphicsView::CacheBackground);   //设置背景缓存模式，提高绘图效率
    setScene(new QGraphicsScene());                 //设置场景
    this->setMinimumSize(QSize(500, 500));          //设置该窗口的最小大小
    setStyleSheet("margin:10");                     //设置margin为10，方便后面进行操作

    /* 设置滚动条永远不显示，事实上设置了setScene之后，就已经不会显示了*/
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* 设置变换和Resize锚点 */
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    /* 注册自定义类型 */
    RegisterType();

    /* 设置槽函数的链接 */
    SetConnect();

    m_resizeTimer.setSingleShot(true);
    connect(&m_resizeTimer, &QTimer::timeout, this, &GraphicsView::Slot_onResizeFinished);

    // 设置默认所有缺陷级别是否可见
    m_levelVisibility[DefectLevel::NORMAL]              = false;
    m_levelVisibility[DefectLevel::MINOR]               = true;
    m_levelVisibility[DefectLevel::MEDIUM]              = true;
    m_levelVisibility[DefectLevel::SERIOUS]             = true;
    m_levelVisibility[DefectLevel::ABNORMAL]            = false;
    m_levelVisibility[DefectLevel::AREAERROR]           = false;

    //设置默认所有缺陷类型是否可见
    m_typeVisibility[DefectType::TYPE_POORCOATING]      = true;
    m_typeVisibility[DefectType::TYPE_SCRATCH]          = true;
    m_typeVisibility[DefectType::TYPE_CALCULUS]         = true;
    m_typeVisibility[DefectType::TYPE_BUBBLE]           = true;
    m_typeVisibility[DefectType::TYPE_TRADEMARK]        = true;
    m_typeVisibility[DefectType::TYPE_WATERSTAIN]       = true;
    m_typeVisibility[DefectType::TYPE_SMUDGE]           = true;
    m_typeVisibility[DefectType::TYPE_SCREENPRINTING]   = false;
    m_typeVisibility[DefectType::TYPE_GLASS_CULLET]     = false;
    m_typeVisibility[DefectType::TYPE_PITTING]          = false;
    m_typeVisibility[DefectType::TYPE_CHIPPED_EDGE]     = false;
    m_typeVisibility[DefectType::TYPE_WAVINESS]         = false;
    m_typeVisibility[DefectType::TYPE_OTHER]            = false;
}

GraphicsView::~GraphicsView()
{
    // 先重置指针和容器，再清理场景
    m_pixItem = nullptr;

    // 清空查找表（只清空容器，不删除对象）
    m_itemLut.clear();

    // 让 scene 统一管理所有对象的生命周期
    if (scene()) {
        scene()->clear();  // scene 会删除所有 items
    }

    if (m_defectRectShow)
    {
        m_defectRectShow.reset(); // 自动释放旧对象，无需手动 delete
    }
    //std::cout << "GraphicsView::~GraphicsView()" << std::endl;
}

void GraphicsView::SetConnect()
{
    connect(this, &GraphicsView::Signal_AddPixmapItem, this, &GraphicsView::Slot_AddPixmapItem);
    connect(this, &GraphicsView::Signal_ClearRectItem, this, &GraphicsView::Slot_ClearRectItem);
    connect(this, &GraphicsView::Signal_AddRectItem, this, &GraphicsView::Slot_AddRectItem);
    connect(this, &GraphicsView::Signal_UpdateLevelShow, this, &GraphicsView::Slot_UpdateLevelShow);
    connect(this, &GraphicsView::Signal_UpdateTypeShow, this, &GraphicsView::Slot_UpdateTypeShow);
	connect(this, &GraphicsView::Signal_BelowOnItem, this, &GraphicsView::BelowOnItem);
}

void GraphicsView::InsertLut(drawInformation& info, QGraphicsItem* item)
{
    if (m_itemLut.contains(info.ErrorType))
    {
        if (m_itemLut[info.ErrorType].contains(info.DefectType))
        {
            m_itemLut[info.ErrorType][info.DefectType].push_back(item);
        }
        else
        {
            m_itemLut[info.ErrorType].insert(info.DefectType, QVector<QGraphicsItem*>{item});
        }
    }
    else
    {
        m_itemLut.insert(info.ErrorType, QMap<DefectType, QVector<QGraphicsItem*>>{
            {info.DefectType, QVector<QGraphicsItem*>{item}}
        });
    }
}

void GraphicsView::RegisterType()
{
    qRegisterMetaType<drawInformation>("drawInformation");
    qRegisterMetaType<drawInformation>("drawInformation&");
    qRegisterMetaType<DefectLevel>("DefectLevel");
    qRegisterMetaType<DefectType>("DefectType");
    qRegisterMetaType<cv::Mat>("cv::Mat");
   }

//void GraphicsView::Slot_ClearRectItem()
//{
//    /* 清除所有的item */
//    if (!this->scene()->items().empty())
//    {
//        this->scene()->clear();
//    }
//
//    if (m_pixItem != nullptr)
//    {
//        m_pixItem = nullptr;
//    }
//
//    if (this->m_itemLut.empty())
//        return;
//
//    m_itemLut.clear();
//    
//    m_isChanged = false; //标志位复原
//}

void GraphicsView::Slot_ClearRectItem()
{
    // 先重置指针和容器，再清理场景
    m_pixItem = nullptr;

    // 清空查找表（只清空容器，不删除对象）
    m_itemLut.clear();

    // 让 scene 统一管理所有对象的生命周期
    if (scene() && !scene()->items().empty()) {
        scene()->clear();  // scene 会负责删除所有 items
    }

    m_isChanged = false;

    qDebug() << "GraphicsView::Slot_ClearRectItem - Cleared all items";
}

void GraphicsView::ExportToImage(const QString& filePath)
{
    // 获取所有 QGraphicsItem 的边界矩形
    QRectF itemsBoundingRect = this->scene()->itemsBoundingRect();

    // 创建一个 QImage 对象，大小为 itemsBoundingRect 的大小
    QImage image(itemsBoundingRect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent); // 填充透明背景

    // 创建一个 QPainter 对象，并将其设置为绘制到 QImage 上
    QPainter painter(&image);

    // 将 QGraphicsView 的场景绘制到 QImage 上
    this->scene()->render(&painter, QRectF(image.rect()), itemsBoundingRect);

    // 保存 QImage 为文件
    image.save(filePath);
}

bool GraphicsView::EditLut(QGraphicsItem* item, const DefectLevel level, const DefectType type, bool isDelete)
{
    //qDebug() << "EditLut begin";
    if (m_itemLut.empty())
    {
        return false;
    }
    /* 获取原来的type和level */
    QVariant var = item->data(0);
    if (var.isNull())
    {
        return false;
    }
    drawInformation info = var.value<drawInformation>();

    if (!m_itemLut.contains(info.ErrorType))
    {
        return false;
    }
    std::string newLevelStr;
    newLevelStr = GetDefectLevelString(level);
    std::string newTypeStr;
    newTypeStr = GetDefectTypeString(type);

    std::string oldLevelStr;
    oldLevelStr = GetDefectLevelString(info.ErrorType);
    std::string oldTypeStr;
    oldTypeStr = GetDefectTypeString(info.DefectType);
    FILE_LOG_INFO("EditLut: old type = %s , old level = %s", oldLevelStr.c_str(), oldLevelStr.c_str());
    /* 获取类型的查找表 */
    auto& typeLut = m_itemLut[info.ErrorType];
    if (typeLut.empty() || !typeLut.contains(info.DefectType))
        return false;

    /* 获取QVector类型的item指针列表 */
    auto& items = typeLut[info.DefectType];
    if (items.empty() || !items.contains(item))
    {
        return false;
    }
    int index = items.indexOf(item);
    if (index == -1)
    {
        return false;
    }
    if (!isDelete)
    {
        //修改最新的内容
        info.ErrorType = level;
        info.DefectType = type;
        //插入到新的地方
        InsertLut(info, item);
        FILE_LOG_INFO("EditLut: new type = %s , new level = %s", newLevelStr.c_str(), newLevelStr.c_str());
    }
    else
    {
        FILE_LOG_INFO("EditLut: delete defect. ");
    }
    //在现有地方删除
    items.removeOne(item);
    

    //修改item中的data内容
    var.clear();
    if (var.isNull())
    {
        var.setValue(info);
        item->setData(0, var);
    }

    m_isChanged = true; //标记当前有item的信息被修改
    //emit this->Signal_UpdateDefectNumShow();   //更新缺陷数量显示
    return true;
}

void GraphicsView::BelowOnItem()
{
    //qDebug() << "BelowOnItem begin";
    if (!m_pixItem || !viewport()) {
        return; // 提前返回，避免空指针操作
    }

    if (m_pixItem->GetPixmap().isNull()) {
        return;
    }

    //缩放以留出顶部和右侧的空白
    this->ZoomBy(0.9);
    //qDebug() << "PixMapItem.boundingRect() = " << m_pixItem->boundingRect();
    //qDebug() << "PixMapItem.sceneBoundingRect() = " << m_pixItem->sceneBoundingRect();
    //qDebug() << "viewPort().height() = " << this->viewport()->height();
    //qDebug() << "viewPort().width() = " << this->viewport()->width();

    // 获取 viewport 的左下角点在 scene 中的坐标
    QPointF leftBottom = this->mapToScene(QPoint(40, this->viewport()->height() - 40));

    //qDebug() << "leftBottom = " << leftBottom;

    // 获取 pixItem 的边界矩形在 scene 中的全局坐标
    QRectF imageRect = m_pixItem->mapToScene(m_pixItem->boundingRect()).boundingRect();

    // 考虑缩放因子计算 dx 和 dy
    qreal dy = (leftBottom.y() - imageRect.height());

    // 设置 pixItem 的位置
    m_pixItem->setPos(leftBottom.x(), dy);

    //临时留白
    //this->ZoomBy(0.9);
}

bool GraphicsView::MoveTo(GraphicsView* newView,double dx,double dy)
{
    FILE_LOG_INFO("MoveTo: Move the Right image to the Left scene");
    //qDebug() << "MoveTo begin";
    if (newView == nullptr)
        return false;

    //没有东西，无需拷贝，直接返回true;
    //if (this->scene()->items().empty() || m_itemLut.empty())
    //    return true
    
    if (this->scene()->items().empty())
        return true;

    //理应一个场景中只有一个父对象（顶层对象），个数不对返回错误
    //if (this->scene()->items().size() != 1)
    //    return false;

    //auto fatheritem = this->scene()->items().front();
// 获取所有图形项
    QList<QGraphicsItem*> allItems = this->scene()->items();
    QGraphicsItem* topLevelItems = nullptr;

    // 筛选顶级项（无父项的项）
    GraphicsView* topLevelView = dynamic_cast<GraphicsView*>(newView);

    ImageWidget* imageWidget = dynamic_cast<ImageWidget*>(allItems.back());
    if (imageWidget != nullptr) 
    {
        QPixmap  tempMap = imageWidget->GetPixmap();
        emit topLevelView->Signal_AddPixmapItem(tempMap, m_glassPixelWidth, m_glassPixelHeight, m_glassPhysicalWidth, m_glassPhysicalHeight);
    }

    // 提取所有的 DefectInfoItem *
    QList<DefectInfoItem*> defectItems;
    for (QGraphicsItem* item : allItems)
    {
        DefectInfoItem* defectItem = dynamic_cast<DefectInfoItem*>(item);
        if (defectItem != nullptr)
        {
            defectItems.append(defectItem);
        }
    }

    // 按照颜色排序
    std::sort(defectItems.begin(), defectItems.end(), [](DefectInfoItem* a, DefectInfoItem* b) {
        return colorPriority(a->pen().color()) < colorPriority(b->pen().color());
        });

    // 设置 Z 值以控制显示顺序（绿色在下，红色在上）
    for (int i = 0; i < defectItems.size(); ++i) {
        defectItems[i]->setZValue(i); // 可选：如果你希望保留排序后的图层顺序
    }

    // 发送每个缺陷项给新视图
    for (DefectInfoItem* defectInfoItem : defectItems)
    {
        drawInformation item = defectInfoItem->data(0).value<drawInformation>();   //获取原始数据(原始数据已经从allItems得到。在addItem时，已经添加)
        if (!defectInfoItem->detect_image_0.empty() && !defectInfoItem->detect_image_1.empty() && !defectInfoItem->detect_image_2.empty())
        {
            emit topLevelView->Signal_AddRectItem(item, dx, dy, defectInfoItem->detect_image_0, defectInfoItem->detect_image_1, defectInfoItem->detect_image_2, false, defectInfoItem->isVisible());
        }
        //qDebug() << "MOVETO::::defectItems-uuid: " << item.uuid;
        //qDebug() << "defectInfoItem-uuid: " << defectInfoItem->GetUuid();
    }

    /*20250602：注释掉不支持按色卡排序得代码
    ///////*for (QGraphicsItem* item : allItems) {
    //////    DefectInfoItem * defectInfoItem = dynamic_cast<DefectInfoItem*>(item);
    //////    if (defectInfoItem != nullptr) {
    //////        auto item = defectInfoItem->data(0).value<drawInformation>();
    //////        emit topLevelView->Signal_AddRectItem(item, dx,dy,defectInfoItem->detect_image);

    //////    }
    //////}*/

    //auto fatheritem = topLevelItems;
    //this->scene()->removeItem(fatheritem);  //在原场景中移除
    //newView->scene()->addItem(fatheritem);  //在新场景中添加

    //newView->m_itemLut = this->m_itemLut;   //将item的查找表进行复制

    //将主键进行复制
    newView->m_primaryKey = this->m_primaryKey;
    newView->m_isChanged = this->GetModifiedFlag();

    emit this->Signal_ClearRectItem();

    FILE_LOG_INFO("MoveTo: Move Completed.");
    return true;
}

void GraphicsView::SetDefectVisibility(DefectLevel level, bool visible)
{
    m_levelVisibility[level] = visible;
    Slot_UpdateLevelShow(level, visible);
}

void GraphicsView::SetDefectVisibility(DefectType type, bool visible)
{
    m_typeVisibility[type] = visible;
    Slot_UpdateTypeShow(type, visible);
}

void GraphicsView::Slot_AddRectItem(drawInformation& info, double dx,double dy,
    cv::Mat small_image_0, cv::Mat small_image_1, cv::Mat small_image_2,
    bool isLastItem, bool isVisabled)
{
    //qDebug() << "Slot_AddRectItem begin";
    if (m_pixItem == nullptr)
    {
        /* 如果没有大图，直接返回 */
        return;
    }

    /* 获取缺陷的颜色 */
    QColor color = DefectLevelToColor(info.ErrorType);
    /* 这里后面提取为公共的函数 */
    QString type;
    type = DefectTypeToString(info.DefectType);

    // 获取主图的当前位置和缩放
    QPointF imagePos = m_pixItem->pos();
    qreal imageScaleX = m_pixItem->transform().m11();
    qreal imageScaleY = m_pixItem->transform().m22();

    // 计算图例在场景中的位置（考虑主图的位置和缩放）
    int rectX = info.rect.x * imageScaleX;
    int rectY = info.rect.y * imageScaleY;
    int rectWidth = info.rect.width * imageScaleX;
    int rectHeight = info.rect.height * imageScaleY;

    QRect cvRect(rectX, rectY, rectWidth, rectHeight);


    QUuid uuid = info.uuid; // 生成一个新的随机 UUID
    //qDebug() << "UUID: " << uuid.toString();
    DefectInfoItem* cvRectItem      = new DefectInfoItem(type, cvRect, uuid, m_pixItem);
	cvRectItem->detect_image_0 = small_image_0.clone();											   
    cvRectItem->detect_image_1 = small_image_1.clone();
    cvRectItem->detect_image_2 = small_image_2.clone();


    //对每个item设置data
    QVariant var;
    drawInformation infoCopy = info;
    var.setValue(infoCopy);
    cvRectItem->setData(0, var);    //存入原始数据

    //设置矩形的边框和填充颜色
    cvRectItem->setPen(QPen(color));
    cvRectItem->setVisible(isVisabled);
    //cvRectItem->setBrush(QBrush(color));
    
    /* 如果没有找到对应的缺陷类型，则创建一个新的缺陷类型 */
    //查看是否已经存在对应的缺陷类型的itemGroup
    if (m_itemLut.contains(info.ErrorType))
    {
        if (m_itemLut[info.ErrorType].contains(info.DefectType))
        {
            m_itemLut[info.ErrorType][info.DefectType].push_back(cvRectItem);
        }
        else
        {
            m_itemLut[info.ErrorType].insert(info.DefectType, QVector<QGraphicsItem*>{cvRectItem});
        }
    }
    else
    {
        m_itemLut.insert(info.ErrorType, QMap<DefectType, QVector<QGraphicsItem*>>{
            {info.DefectType, QVector<QGraphicsItem*>{cvRectItem}}
        });
    }
    
    if (!this->scene()->items().contains(cvRectItem))
    {
        this->scene()->addItem(cvRectItem);
    }

    connect(cvRectItem, &DefectInfoItem::Signal_requestRemove, this, &GraphicsView::Slot_requestRemove);

    connect(cvRectItem, &DefectInfoItem::Signal_requestUpdate, this, &GraphicsView::Slot_requestUpdateItme); // 传递信号：修改缺陷信息

    // 根据预设可见性设置显示状态
    bool levelVisible = m_levelVisibility.value(info.ErrorType, false);
    bool typeVisible = m_typeVisibility.value(info.DefectType, false);
    //bool isVisible = levelVisible && typeVisible;

    //cvRectItem->setVisible(isVisible);
    cvRectItem->SetLevelShow(levelVisible);
    cvRectItem->SetTypeShow(typeVisible);


    this->scale(1.0, 1.0);
    if (isLastItem)
    {
        //emit Signal_UpdateDefectNumShow();
    }
}



QPixmap createFilledPixmap(const QPixmap& originalPixmap, const QColor& fillColor) {
    //qDebug() << "createFilledPixmap begin";
    // 创建一个大小与原始QPixmap一致的新QPixmap
    QPixmap filledPixmap(originalPixmap.size());

    // 使用QPainter来填充颜色
    QPainter painter(&filledPixmap);
    painter.fillRect(filledPixmap.rect(), fillColor);
    painter.end();

    return filledPixmap;
}


/// <summary>
/// 原始界面添加图片，会导致裁剪图与双击显示图像一致
/// </summary>
/// <param name="item"></param>
void GraphicsView::Slot_AddPixmapItem(QPixmap& item, float glassPixelWidth, float glassPixelHeight, float glassPhysicalWidth, float glassPhysicalHeight)
{
    //qDebug() << "Slot_AddPixmapItem begin";
    /* 清除那些已经添加的RectItem对象 */
    //Signal_ClearRectItem();
  
    if (item.isNull())
    {
        return;
    }
    if (m_pixItem == nullptr)
    {
        m_pixItem = new ImageWidget(nullptr);
    }
    if (m_pixItem)  //设置图像是否可以通过鼠标拖动(移动)
    {
        m_pixItem->SetMouseMove(m_isMouseMove);
    }

    m_glassPixelWidth = glassPixelWidth;
    m_glassPixelHeight = glassPixelHeight;
    m_glassPhysicalWidth = glassPhysicalWidth;
    m_glassPhysicalHeight = glassPhysicalHeight;

    //qDebug() << "source glassPixelWidth=" << glassPixelWidth << ", glassPixelWidth=" << glassPixelWidth << endl;
    //qDebug() << "glassPhysicalWidth=" << glassPhysicalWidth << ", glassPhysicalHeight=" << glassPhysicalHeight << endl;
    //qDebug() << "sizeChanged item.size().width()=" << item.size().width() << ", item.size().height()=" << item.size().height() << endl;

    // 根据传入的物理尺寸更新像素-毫米比例
    if (item.size().width() > 0 && glassPhysicalWidth > 0)
    {
        m_pixelXmm = glassPhysicalWidth / item.size().width();
    }
    if (item.size().height() > 0 && glassPhysicalHeight > 0)
    {
        m_pixelYmm = glassPhysicalHeight / item.size().height();
    }
    /* 添加图片对象 */
    m_pixItem->SetPixmap(item);
	
	int nWidth = this->viewport()->width();
    int nHeight = this->viewport()->height();
    m_pixItem->setQGraphicsViewWH(nWidth, nHeight);//这里对item的大小进行了缩小


    this->resetTransform();

    // 保存初始变换状态（原始比例和位置）
// 保存初始变换状态（在视图可见后）

    m_initialTransform = transform();

    this->scene()->addItem(m_pixItem);

    m_imageRect = m_pixItem->boundingRect();
    //使视窗的大小固定在原始大小，不会随图片的放大而放大（默认状态下图片放大的时候视窗两边会自动出现滚动条，并且视窗内的视野会变大），防止图片放大后重新缩小的时候视窗太大而不方便观察图片
    //定义了渲染的范围，在范围外的Scene不进行渲染，但是如果不设置，则范围外的Scene也进行渲染，所以会出现滚动条的情况。
    this->setSceneRect(QRectF(0, 0, nWidth, nHeight));
    this->setFocus();
    //qDebug() << "GraphicsView::Slot_AddPixmapItem";
    emit this->Signal_BelowOnItem(); //第一次将图像放到左下角，并设置显示比例0.9
}

void GraphicsView::Slot_UpdateLevelShow(DefectLevel level, bool show)
{

    /* 更新缺陷等级显示 */
    if (m_itemLut.contains(level))
    {
        for (auto typeIt = m_itemLut[level].begin(); typeIt != m_itemLut[level].end(); ++typeIt)
        {
            if (typeIt.value().isEmpty())
            {
                continue;
            }

            auto group = typeIt.value();
            for (auto it = group.begin(); it != group.end(); ++it)
            {
                if (*it != nullptr)
                {
                    auto item = dynamic_cast<DefectInfoItem*>(*it);
                    if (item != nullptr)
                    {
                        item->SetLevelShow(show);
                    }
                    else
                    {
                        (*it)->setVisible(show);
                    }
                }
            }
        }
        this->update();
    }
}

void GraphicsView::Slot_UpdateTypeShow(DefectType type, bool show)
{

    /* 更新缺陷类型显示 */
    if (!m_itemLut.empty())
    {
        for (auto levelIte = m_itemLut.begin(); levelIte != m_itemLut.end(); ++levelIte)
        {
            if (levelIte.value().contains(type))
            {
                if (levelIte.value()[type].isEmpty())
                {
                    continue;
                }

                auto group = levelIte.value()[type];
                for (auto groupIte = group.begin(); groupIte != group.end(); ++groupIte)
                {
                    if (*groupIte != nullptr)
                    {
                        auto item = dynamic_cast<DefectInfoItem*>(*groupIte);
                        if (item != nullptr)
                        {
                            item->SetTypeShow(show);
                        }
                        else
                        {
                            (*groupIte)->setVisible(show);
                        }
                    }
                }
            }
        }
        this->update();
    }
}


void GraphicsView::ZoomFacotrShow()
{
    //qDebug() << "ZoomFacotrShow begin";
    const int percent = qRound(transform().m11() * qreal(100));
    setWindowTitle("VIEW: " + QString::number(percent) + QLatin1Char('%'));
}

void GraphicsView::wheelEvent(QWheelEvent* event)
{
    if (!m_isMouseWheel)
    {
        return;
    }
    //qDebug() << "wheelEvent begin";
    ZoomBy(qPow(1.2, event->delta() / 240.0));
    //GraphicsView::wheelEvent(event);
}

void GraphicsView::ZoomBy(qreal factor)
{
    //qDebug() << "ZoomBy begin";
    const qreal currentZoom = transform().m11();    //获取X轴的缩放因子
    if ((factor < 1 && currentZoom < 0.1) || (factor > 1 && currentZoom > 10))
        return;
    scale(factor, factor);
    ZoomFacotrShow();
}

void GraphicsView::paintEvent(QPaintEvent* event)
{
    //qDebug() << "paintEvent begin";
    /* 先进行scene()的绘制，再进行边框的绘制，防止覆盖 */
    QGraphicsView::paintEvent(event);

    //绘制标尺
    //只在viewPort上进行绘制，不在QGraphicsView上进行绘制，防止滚动条挡住
    QPainter painter(this->viewport());

    //绘制水平标尺
    DrawHorizonRuler(painter);

    //绘制竖直标尺
    DrawVerticalRuler(painter);

    //绘制左上角符号
    DrawRuleCross(painter);
}

void GraphicsView::DrawVerticalRuler(QPainter& painter)
{
    if (!m_pixItem || m_pixItem->GetPixmap().isNull())
        return;

    QRectF sceneRect = m_pixItem->sceneBoundingRect();
    if (sceneRect.height() <= 0) return;

    double physPerSceneY = m_glassPhysicalHeight / sceneRect.height();
    double physPerPxY = physPerSceneY / transform().m22();

    const double TARGET_PX_SPACING = 80.0;
    double desiredPhysStep = TARGET_PX_SPACING * physPerPxY;
    double stepMm = niceStep(desiredPhysStep);

    QPointF imgBottomLeft = m_pixItem->mapToScene(m_pixItem->boundingRect().bottomLeft());

    QPointF viewTop = mapToScene(0, 0);
    QPointF viewBottom = mapToScene(0, viewport()->height());

    double physTop = (imgBottomLeft.y() - viewTop.y()) * physPerSceneY;
    double physBottom = (imgBottomLeft.y() - viewBottom.y()) * physPerSceneY;
    double physMin = std::min(physTop, physBottom);
    double physMax = std::max(physTop, physBottom);

    // 绘制标尺背景
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, 20, viewport()->height());
    painter.setPen(Qt::black);
    painter.drawLine(20, 0, 20, viewport()->height());

    QPolygon points;
    double prevTextY = -1e9;

    double startPhys = std::floor(physMin / stepMm) * stepMm;
    for (double phys = startPhys; phys <= physMax; phys += stepMm)
    {
        double sceneY = imgBottomLeft.y() - phys / physPerSceneY;
        QPoint viewPos = mapFromScene(0, sceneY);
        int viewY = viewPos.y();

        // 主刻度：仅在视口内时绘制刻度线和文本
        if (viewY >= 0 && viewY <= viewport()->height() - 20)
        {
            points << QPoint(20, viewY) << QPoint(0, viewY) << QPoint(20, viewY);

            // 刻度值（避免重叠）
            int textThreshold = 40;
            if (std::abs(viewY - prevTextY) > textThreshold)
            {
                painter.save();
                painter.setBrush(Qt::black);
                painter.translate(2, viewY - 2);
                painter.rotate(90);
                painter.drawText(QPoint(2, 0), QString::number(qRound(phys)) + " mm");
                painter.restore();
                prevTextY = viewY;
            }
        }

        // 子刻度：无论主刻度是否在视口内，只要子刻度映射后在视口内就绘制
        for (int i = 1; i < 10; ++i)
        {
            double subPhys = phys + i * (stepMm / 10.0);
            // 虽然物理值可能超出视口范围，但屏幕坐标检查会过滤
            double subSceneY = imgBottomLeft.y() - subPhys / physPerSceneY;
            QPoint subViewPos = mapFromScene(0, subSceneY);
            int subViewY = subViewPos.y();
            if (subViewY >= 0 && subViewY <= viewport()->height() - 20)
            {
                int tickWidth = (i % 5 == 0) ? 10 : 15;
                points << QPoint(20, subViewY) << QPoint(tickWidth, subViewY) << QPoint(20, subViewY);
            }
        }
    }

    painter.setPen(Qt::black);
    painter.drawPolyline(points);
}

void GraphicsView::DrawHorizonRuler(QPainter& painter)
{
    if (!m_pixItem || m_pixItem->GetPixmap().isNull())
        return;

    QRectF sceneRect = m_pixItem->sceneBoundingRect();
    if (sceneRect.width() <= 0) return;

    double physPerSceneX = m_glassPhysicalWidth / sceneRect.width();
    double physPerPxX = physPerSceneX / transform().m11();

    const double TARGET_PX_SPACING = 80.0;
    double desiredPhysStep = TARGET_PX_SPACING * physPerPxX;
    double stepMm = niceStep(desiredPhysStep);

    QPointF imgBottomLeft = m_pixItem->mapToScene(m_pixItem->boundingRect().bottomLeft());

    QPointF viewLeft = mapToScene(0, viewport()->height());
    QPointF viewRight = mapToScene(viewport()->width(), viewport()->height());

    double physLeft = (viewLeft.x() - imgBottomLeft.x()) * physPerSceneX;
    double physRight = (viewRight.x() - imgBottomLeft.x()) * physPerSceneX;
    double physMin = std::min(physLeft, physRight);
    double physMax = std::max(physLeft, physRight);

    // 绘制标尺背景
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.drawRect(20, viewport()->height() - 20, viewport()->width() - 20, 20);
    painter.setPen(Qt::black);
    painter.drawLine(20, viewport()->height() - 20, viewport()->width(), viewport()->height() - 20);

    QPolygon points;
    double prevTextX = -1e9;

    double startPhys = std::floor(physMin / stepMm) * stepMm;
    for (double phys = startPhys; phys <= physMax; phys += stepMm)
    {
        double sceneX = imgBottomLeft.x() + phys / physPerSceneX;
        QPoint viewPos = mapFromScene(sceneX, 0);
        int viewX = viewPos.x();

        // 主刻度：仅在视口内时绘制刻度线和文本
        if (viewX >= 20 && viewX <= viewport()->width())
        {
            points << QPoint(viewX, viewport()->height())
                << QPoint(viewX, viewport()->height() - 20)
                << QPoint(viewX, viewport()->height());

            // 刻度值（避免重叠）
            if (std::abs(viewX - prevTextX) > 40)
            {
                painter.setBrush(Qt::black);
                painter.drawText(QPoint(viewX + 4, viewport()->height() - 6),
                    QString::number(qRound(phys)) + " mm");
                prevTextX = viewX;
            }
        }

        // 子刻度：无论主刻度是否在视口内，只要子刻度映射后在视口内就绘制
        for (int i = 1; i < 10; ++i)
        {
            double subPhys = phys + i * (stepMm / 10.0);
            double subSceneX = imgBottomLeft.x() + subPhys / physPerSceneX;
            QPoint subViewPos = mapFromScene(subSceneX, 0);
            int subViewX = subViewPos.x();
            if (subViewX >= 20 && subViewX <= viewport()->width())
            {
                int tickHeight = (i % 5 == 0) ? 10 : 5;
                points << QPoint(subViewX, viewport()->height())
                    << QPoint(subViewX, viewport()->height() - tickHeight)
                    << QPoint(subViewX, viewport()->height());
            }
        }
    }

    painter.setPen(Qt::black);
    painter.drawPolyline(points);
}

void GraphicsView::DrawRuleCross(QPainter& painter)
{
    // 获取视口的高度
    int viewportHeight = this->viewport()->height();

    // 绘制左下角标记
    painter.setBrush(QColor(255, 255, 255, 255));
    painter.setPen(Qt::white);
    painter.drawRect(QRect(0, viewportHeight - 20, 20, 20));
    painter.setPen(Qt::DashLine);
    painter.drawLine(QPoint(0, viewportHeight - 10), QPoint(20, viewportHeight - 10));
    painter.drawLine(QPoint(10, viewportHeight - 20), QPoint(10, viewportHeight));
    painter.setPen(Qt::SolidLine);
    painter.drawLine(QPoint(0, viewportHeight - 20), QPoint(20, viewportHeight - 20));
    painter.drawLine(QPoint(20, viewportHeight - 20), QPoint(20, viewportHeight));
}

void GraphicsView::DrawHorGridLine(QPainter& painter)
{
    QPoint lefttop = QPoint(0, 0);
    QPoint righttop = QPoint(this->width(), 0);

    //! 转换成 QGraphicsScene 坐标
    QPointF scene_lefttop = mapToScene(lefttop);
    QPointF scene_righttop = mapToScene(righttop);


    //! 还是以宽 为标准
    float fscale = (scene_righttop.x() - scene_lefttop.x()) * 1.0 / this->width();

    //! 步长
    int  nDistance = 100;
    nDistance = ((1.0 / fscale * 100) / 10) * 10;
    if (nDistance > 50 && nDistance < 150)
    {
        nDistance = 100;
    }
    if (nDistance >= 150 && nDistance < 200)
    {
        nDistance = 200;
    }

    QPolygon BoldPoints;
    QPolygon ThinPoints;

    //! 从 0 -> ∞
    for (int i = 0; i < scene_righttop.x(); i += nDistance)
    {
        for (int j = 0; j < 10; ++j)
        {
            int nxPt = i + j * (nDistance / 10);
            QPoint viewpointX = mapFromScene(nxPt, 0);
            if (viewpointX.x() < 20)
                continue;
            if (j == 0)
            {
                BoldPoints.push_back(QPoint(viewpointX.x(), 20));
                BoldPoints.push_back(QPoint(viewpointX.x(), this->height()));
                BoldPoints.push_back(QPoint(viewpointX.x(), 20));
            }
            else
            {
                ThinPoints.push_back(QPoint(viewpointX.x(), 20));
                ThinPoints.push_back(QPoint(viewpointX.x(), this->height()));
                ThinPoints.push_back(QPoint(viewpointX.x(), 20));
            }
        }
    }

    //! 从 0 到 负无穷
    for (int i = 0; i >= scene_lefttop.x(); i -= nDistance)
    {
        for (int j = 0; j < 10; ++j)
        {
            int nxPt = i - j * (nDistance / 10);
            QPoint viewpointX = mapFromScene(nxPt, 0);
            if (viewpointX.x() < 20)
                continue;
            if (j == 0)
            {
                BoldPoints.push_back(QPoint(viewpointX.x(), 20));
                BoldPoints.push_back(QPoint(viewpointX.x(), this->height()));
                BoldPoints.push_back(QPoint(viewpointX.x(), 20));
            }
            else
            {
                ThinPoints.push_back(QPoint(viewpointX.x(), 20));
                ThinPoints.push_back(QPoint(viewpointX.x(), this->height()));
                ThinPoints.push_back(QPoint(viewpointX.x(), 20));
            }
        }
    }

    QPen boldpen(QBrush(QColor(0, 0, 0, 50)), 1);
    painter.setPen(boldpen);
    painter.drawPolyline(BoldPoints);
    QPen thinpen(QBrush(QColor(0, 0, 0, 25)), 1);
    painter.setPen(thinpen);
    painter.drawPolyline(ThinPoints);
}

void GraphicsView::DrawVerGridLine(QPainter& painter)
{
    QPoint lefttop = QPoint(0, 0);
    QPoint leftbtn = QPoint(this->width(), this->height());

    //! 转换成 QGraphicsScene 坐标
    QPointF scene_lefttop = mapToScene(lefttop);
    QPointF scene_leftbtn = mapToScene(leftbtn);
    //! 还是以宽 为标准
    float fscale = (scene_leftbtn.x() - scene_lefttop.x()) * 1.0 / this->width();

    //! 步长
    int  nDistance = 100;
    nDistance = ((1.0 / fscale * 100) / 10) * 10;
    if (nDistance > 50 && nDistance < 150)
    {
        nDistance = 100;
    }
    if (nDistance >= 150 && nDistance < 200)
    {
        nDistance = 200;
    }

    QPolygon BoldPoints;
    QPolygon ThinPoints;

    //! 从 0 向下 前进
    for (int i = 0; i < scene_leftbtn.y(); i += nDistance)
    {
        for (int j = 0; j < 10; ++j)
        {
            int nyPt = i + j * (nDistance / 10);
            QPoint viewpointY = mapFromScene(0, nyPt);
            if (viewpointY.y() < 20)
                continue;
            if (j == 0)
            {
                BoldPoints.push_back(QPoint(20, viewpointY.y()));
                BoldPoints.push_back(QPoint(this->width(), viewpointY.y()));
                BoldPoints.push_back(QPoint(20, viewpointY.y()));
            }
            else
            {
                ThinPoints.push_back(QPoint(20, viewpointY.y()));
                ThinPoints.push_back(QPoint(this->width(), viewpointY.y()));
                ThinPoints.push_back(QPoint(20, viewpointY.y()));
            }
        }
    }

    //! 从0 向上 移动
    for (int i = 0; i >= scene_lefttop.y(); i -= nDistance)
    {
        for (int j = 0; j < 10; ++j)
        {
            int nyPt = i - j * (nDistance / 10);
            QPoint viewpointY = mapFromScene(0, nyPt);
            if (viewpointY.y() < 20)
                continue;
            if (j == 0)
            {
                BoldPoints.push_back(QPoint(20, viewpointY.y()));
                BoldPoints.push_back(QPoint(this->width(), viewpointY.y()));
                BoldPoints.push_back(QPoint(20, viewpointY.y()));
            }
            else
            {
                ThinPoints.push_back(QPoint(20, viewpointY.y()));
                ThinPoints.push_back(QPoint(this->width(), viewpointY.y()));
                ThinPoints.push_back(QPoint(20, viewpointY.y()));
            }
        }
    }

    QPen boldpen(QBrush(QColor(0, 0, 0, 50)), 1);
    painter.setPen(boldpen);
    painter.drawPolyline(BoldPoints);
    QPen thinpen(QBrush(QColor(0, 0, 0, 25)), 1);
    painter.setPen(thinpen);
    painter.drawPolyline(ThinPoints);
}

void GraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (!m_isMouseMove)
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    //qDebug() << "mousePressEvent begin";
    m_movePos   = event->pos();
    m_clickPos  = event->pos();
    if (event->buttons() == Qt::MiddleButton && scene())
    {
        m_isMove = true;
        viewport()->setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
    this->update();
}

void GraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_isMouseDoubleClicked)
    {
        QGraphicsView::mouseDoubleClickEvent(event);
        return;
    }
    if (!scene() || !m_pixItem) {
        QGraphicsView::mouseDoubleClickEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem* clickedItem = scene()->itemAt(scenePos, transform());

    if (!clickedItem) {
        QGraphicsView::mouseDoubleClickEvent(event);
        return;
    }

    // 精确检查：只有当点击在 DefectInfoItem 的实际图例区域内时才处理
    DefectInfoItem* rectItem = dynamic_cast<DefectInfoItem*>(clickedItem);
    if (rectItem) {
        // 将场景坐标转换到 DefectInfoItem 的局部坐标
        QPointF itemPos = rectItem->mapFromScene(scenePos);

        // 检查是否在 DefectInfoItem 的实际绘制区域内
        if (rectItem->contains(itemPos)) {
            if (rectItem->detect_image_0.empty() || rectItem->detect_image_1.empty() || rectItem->detect_image_2.empty()) {
                return;
            }

            if (m_defectRectShow) {
                m_defectRectShow.reset();
            }
            m_defectRectShow = std::make_unique<DefectRectShow>(this);
            auto info = rectItem->data(0).value<drawInformation>();
            m_defectRectShow->SetDefectInfo(info, m_glassPixelWidth, m_glassPixelHeight, m_glassPhysicalWidth, m_glassPhysicalHeight);
            //m_defectRectShow->setWindowTitle("RectItem Image");
            //qxz0428小图显示前做旋转
           /* cv::Mat rotated0;
            cv::rotate(rectItem->detect_image_0, rotated0, cv::ROTATE_90_COUNTERCLOCKWISE);
            cv::Mat rotated1;
            cv::rotate(rectItem->detect_image_1, rotated1, cv::ROTATE_90_COUNTERCLOCKWISE);
            cv::Mat rotated2;
            cv::rotate(rectItem->detect_image_2, rotated2, cv::ROTATE_90_COUNTERCLOCKWISE);
            m_defectRectShow->SetCvMat(rotated0, rotated1, rotated2, 2);*/
            m_defectRectShow->show();
            m_defectRectShow->SetCvMat(rectItem->detect_image_0, rectItem->detect_image_1, rectItem->detect_image_2, 2);
            return;
        }
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}

//void GraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
//{
//    if (!scene() || !m_pixItem) {
//        QGraphicsView::mouseDoubleClickEvent(event);
//        return;
//    }
//
//    QPointF scenePos = mapToScene(event->pos());
//    QGraphicsItem* clickedItem = scene()->itemAt(scenePos, transform());
//
//    if (!clickedItem)
//    {
//        QGraphicsView::mouseDoubleClickEvent(event);
//        return;
//    }
//
//    // 获取所有在该位置的 items（从上到下）
//    QList<QGraphicsItem*> items = scene()->items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, transform());
//
//    for (QGraphicsItem* item : items)
//    {
//        DefectInfoItem* rectItem = dynamic_cast<DefectInfoItem*>(item);
//        if (rectItem)
//        {
//            // 精确判断：scenePos 是否在 rectItem 的 shape() 内
//            QPainterPath shape = rectItem->shape();
//            QTransform itemToScene = rectItem->sceneTransform();
//            QPainterPath sceneShape = itemToScene.map(shape);
//
//            if (sceneShape.contains(scenePos)) {
//                // 检查图像是否有效
//                if (rectItem->detect_image_0.empty() ||
//                    rectItem->detect_image_1.empty() ||
//                    rectItem->detect_image_2.empty()) {
//                    continue;
//                }
//
//                // 弹出窗口
//                if (m_defectRectShow) {
//                    m_defectRectShow.reset();
//                }
//                m_defectRectShow = std::make_unique<DefectRectShow>(this);
//                auto info = rectItem->data(0).value<drawInformation>();
//                m_defectRectShow->SetDefectInfo(info, m_glassPixelWidth, m_glassPixelHeight, m_glassPhysicalWidth, m_glassPhysicalHeight);
//                m_defectRectShow->setWindowTitle("RectItem Image");
//                m_defectRectShow->show();
//                m_defectRectShow->SetCvMat(rectItem->detect_image_0, rectItem->detect_image_1, rectItem->detect_image_2, 2);
//                return;
//            }
//        }
//    }
//
//    QGraphicsView::mouseDoubleClickEvent(event);
//}

void GraphicsView::Slot_onResizeFinished()
{
    resetMatrix();
    this->resetTransform();

    if (scene() && !scene()->sceneRect().isEmpty())
    {
        // 获取当前视图尺寸
        const QSize viewSize = viewport()->size();

        // 计算需要适配的场景区域
        QRectF targetRect = scene()->sceneRect();

        // 确保适配操作会实际改变视图
        if (matrix().mapRect(targetRect).size().toSize() != viewSize)
        {
            // 强制重设视图
            setSceneRect(targetRect);  // 更新场景边界
            fitInView(targetRect, Qt::KeepAspectRatio);

            // 确保变换应用
            viewport()->update();
        }
    }

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    BelowOnItem();
}

void GraphicsView::Slot_requestRemove(const QUuid& uuid)
{
    emit Signal_requestRemove(uuid);
}

void GraphicsView::Slot_requestUpdateItme(const QUuid& uuid, const int& newLevel, const int& newType)
{
    emit Signal_requestUpdate(uuid, newLevel, newType);
}

void GraphicsView::resizeEvent(QResizeEvent* event)
{
    // 防护：scene 不存在时直接交由父类处理
    if (!scene()) {
        QGraphicsView::resizeEvent(event);
        return;
    }

    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);

    resetMatrix();          // 等价于 resetTransform()
    // this->resetTransform(); // 可省略，resetMatrix() 已足够

    // 更新场景矩形：即使为空也设置（itemsBoundingRect() 对空 scene 返回 QRectF(0,0,0,0)）
    QRectF bounds = scene()->itemsBoundingRect();
    scene()->setSceneRect(bounds);

    // 仅当有实际内容时才 fitInView
    if (!bounds.isEmpty()) {
        fitInView(bounds, Qt::KeepAspectRatio);
    }

    viewport()->update();

    QGraphicsView::resizeEvent(event);

    // 启动延迟处理（如需要）
    m_resizeTimer.start(150); // 建议使用合理延迟（如150ms），0可能太激进
}

void GraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_isMouseMove)
    {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    //qDebug() << "mouseMoveEvent";
    if (m_isMove && scene() && viewport())
    {
        // 检查坐标有效性
        if (event->pos().x() < 0 || event->pos().y() < 0) {
            return;
        }
        //使移动更加平滑
        QPointF disPointF = this->mapToScene(event->pos()) - this->mapToScene(m_movePos);
        scene()->setSceneRect(scene()->sceneRect().x() - disPointF.x(),
            scene()->sceneRect().y() - disPointF.y(),
            scene()->sceneRect().width(),
            scene()->sceneRect().height());
        m_movePos = event->pos();
    }

    QGraphicsView::mouseMoveEvent(event);
    this->update();
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_isMouseMove)
    {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }
    //qDebug() << "mouseReleaseEvent";
    if (!event || !viewport()) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }
    if (m_isMove && scene())
    {
        //QPointF startpos = this->mapToScene(m_movePos);
        //QPointF endpos = this->mapToScene(event->pos());
        //QPointF disPointF = endpos - startpos;
        ////调整位置
        //scene()->setSceneRect(scene()->sceneRect().x() - disPointF.x(),
        //    scene()->sceneRect().y() - disPointF.y(),
        //    scene()->sceneRect().width(),
        //    scene()->sceneRect().height());
        viewport()->setCursor(Qt::ArrowCursor);
        m_isMove = false;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::setMouseDoubleClicked(bool isDoubleClicked)
{ 
    m_isMouseDoubleClicked = isDoubleClicked;
}

void GraphicsView::setMouseWheel(bool isMouseWheel)
{ 
    m_isMouseWheel = isMouseWheel;
}

void GraphicsView::setMouseMove(bool isMove)
{ 
    m_isMouseMove = isMove;
    if (m_pixItem)
    {
        m_pixItem->SetMouseMove(isMove);
    }
}