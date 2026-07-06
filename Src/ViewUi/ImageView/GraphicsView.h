#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <qgraphicsview.h>
#include <qgraphicsitem.h>
#include <qmap.h>
#include <qvector.h>
#include <qmetatype.h>
#include <qpixmap.h>
//#include "header.h"
#include "GeneralMethod.h"
#include "ImageWidget.h"
#include "Global.h"


using ItemPointerGroup = QVector<QGraphicsItem*>;
using TypeLut   = QMap<DefectType, ItemPointerGroup>;
using LevelLut  = QMap<DefectLevel, TypeLut>;

//Q_DECLARE_METATYPE(drawInformation)

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

public:

    /* 构造函数 */
    GraphicsView(QWidget* parent = nullptr);

    /* 析构函数 */
    ~GraphicsView();
    
    /* 在界面显示缩放因子标题 */
    void ZoomFacotrShow();

    /* 将GraphicsScene中的内容保存为图片 */
    void ExportToImage(const QString& filePath);

    /* 修改查找表中信息的函数 */
    bool EditLut(QGraphicsItem* item, const DefectLevel level, const DefectType type, bool isDelete);

    /* 增加将图片进行靠左下显示 */
    void BelowOnItem();

    /* 移动内容函数，用于将一个GraphicsView中的item移动到另一个item中 */
    //bool MoveTo(GraphicsView* newView);
    bool MoveTo(GraphicsView* newView, double dx, double dy);

    /* 获取当前是否修改过的标志位值 */
    bool GetModifiedFlag() const
    {
        return m_isChanged;
    }

    void SetModifyFlag(bool isChanged) noexcept
    {
        m_isChanged = isChanged;
    }

    /* 获取内部的缺陷等级和类型的查找表 */
    const LevelLut& GetLut() const
    {
        return m_itemLut;
    }

    /* 获取ImageWidget的对象指针，方便外面对其进行操作 */
    ImageWidget* GetImageWidget() const
    {
        return m_pixItem;
    }

    /* 获取像素到物理单位的转换比例 */
    double GetPhysicalXRatio() const
    {
        return m_pixelXmm;
    }
    
    double GetPhysicalYRatio() const
    {
        return m_pixelYmm;
    }

    /* 设置物理比例和单位 */
    void SetPhysicalRatio(double ratioX, double ratioY, const QString& unit)
    {
        m_pixelXmm = ratioX;
        m_pixelYmm = ratioY;

        //PIXEL_X_MM = ratioX;    //预留。如果后期写值到config或配方，再启用。
        //PIXEL_Y_MM = ratioY;
        m_physicalUnit = unit;
        viewport()->update();
    }

    /* 获取物理单位 */
    QString GetPhysicalUnit() const
    {
        return m_physicalUnit;
    }

    /* 设置指定瑕疵级别是否显示 */
    void SetDefectVisibility(DefectLevel level, bool visible);

    /* 设置指定瑕疵类型是否显示*/
    void SetDefectVisibility(DefectType type, bool visible);

    /* 设置鼠标是否可以双击(用于图例是否可以双击弹出小窗) */
    void setMouseDoubleClicked(bool isDoubleClicked);

    /* 鼠标滚轮是否可以进行缩放 */
    void setMouseWheel(bool isMouseWheel);

    /* 鼠标是否可以移动 */
    void setMouseMove(bool isMove);

signals:
    /* 添加图片展示的信号 */
    void Signal_AddPixmapItem(QPixmap& map, float glassPixelWidth,float glassPixelHeight,float glassPhysicalWidth, float glassPhysicalHeight);

    /* 清理RectItem */
    void Signal_ClearRectItem();

    /* 添加画出缺陷的小矩形 */
    //void Signal_AddRectItem(drawInformation& info);
    void Signal_AddRectItem(drawInformation& info, double dx, double dy, cv::Mat small_image_0, cv::Mat small_image_1, cv::Mat small_image_2, bool isLastItem, bool isVisable = true);
    /* 当界面缺陷等级显示时进行更新 */
    void Signal_UpdateLevelShow(DefectLevel level, bool show);

    /* 当界面缺陷类型显示时进行更新 */
    void Signal_UpdateTypeShow(DefectType type, bool show);

    /* 当缺陷进行修改后，更新界面缺陷信息数量的显示 */
    //void Signal_UpdateDefectNumShow();

    /* 让item居中显示 */
    void Signal_BelowOnItem();

    /* 请求删除指定索引的数据项*/
    void Signal_requestRemove(const QUuid& uuid);  

    /* 请求更新数据项*/
    void Signal_requestUpdate(const QUuid& uuid, int newLevel, int newType);  

private slots:
    /* 图片添加展示的槽函数 */
    void Slot_AddPixmapItem(QPixmap& map, float glassPixelWidth, float glassPixelHeight, float glassPhysicalWidth, float glassPhysicalHeight);

    /* 清理RectItem槽函数 */
    void Slot_ClearRectItem();

    /* 添加缺陷的小矩形 */
    //void Slot_AddRectItem(drawInformation& info);
    void Slot_AddRectItem(drawInformation& info, double dx, double dy, cv::Mat small_image_0, cv::Mat small_image_1, cv::Mat small_image_2, bool isLastItem, bool isVisable);
    /* 更新缺陷等级显示 */ 
    void Slot_UpdateLevelShow(DefectLevel level, bool show);

    /* 更新缺陷类型显示 */ 
    void Slot_UpdateTypeShow(DefectType type, bool show);


    /* resize事件后图像重置 */
    void Slot_onResizeFinished();

    /* 请求删除指定索引的数据项*/
    void Slot_requestRemove(const QUuid& uuid);

    /* 接受修改图例*/
    void Slot_requestUpdateItme(const QUuid& uuid, const int& newLevel, const int& newType);

protected:

    /* 缩放函数重写 */
    void wheelEvent(QWheelEvent* event) override;

    /* 绘图函数重写，绘制刻度尺 */
    void paintEvent(QPaintEvent* event) override;

    /* 重写鼠标按下函数 */
    void mousePressEvent(QMouseEvent* event) override;

    /* 重写鼠标移动函数 */
    void mouseMoveEvent(QMouseEvent* event) override;

    /* 重写鼠标释放函数 */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /* 重写鼠标双击事件 */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /* 重写resize函数 */
    void resizeEvent(QResizeEvent* event) override;

private:
    /* 绘制水平上标尺的函数 */
    void DrawHorizonRuler(QPainter& painter);

    /* 绘制左侧标尺的函数 */
    void DrawVerticalRuler(QPainter& painter);

    /* 绘制坐上角的图标的函数 */
    void DrawRuleCross(QPainter& painter);

    /* 绘制竖向栅栏 */
    void DrawHorGridLine(QPainter& painter);

    /* 绘制横向栅栏 */
    void DrawVerGridLine(QPainter& painter);

    ///* 绘制画布，暂时不用 */
    //void DrawCanvasRect(QPainter& painter);

    /* 界面缩放 */
    void ZoomBy(qreal factor);

    /* 设置信号槽链接 */
    void SetConnect();

    /* 插入查找表 */
    void InsertLut(drawInformation& info, QGraphicsItem* item);

    /* 注册类型 */
    void RegisterType();

private:
    QPoint          m_movePos;   //鼠标移动坐标
    QPoint          m_clickPos;  //鼠标点击坐标
    bool            m_isMove;   //标记视窗是否正在被拖动
    ImageWidget*    m_pixItem;  //放置图片的Item
    ImageWidget*    m_pixItemShow;  //放置示例图的图片
    LevelLut        m_itemLut;  //缺陷等级和类型的查找表
    bool            m_isChanged;//用来标记当前QGraphicsScence中的缺陷是否已经被用户修改

    float			m_glassPixelWidth = 0.0f;
    float			m_glassPixelHeight = 0.0f;
    float		    m_glassPhysicalWidth = 0.0f;
    float			m_glassPhysicalHeight = 0.0f;

    //像素到物理单位的转换比例
    double          m_pixelXmm;
    double          m_pixelYmm;

    //物理单位
    QString         m_physicalUnit;

    QTransform m_initialTransform; // 保存初始变换状态

    QRectF m_imageRect;

    QTimer m_resizeTimer;

    QMap<DefectLevel, bool> m_levelVisibility;  //储存各种等级的显示状态

    QMap<DefectType, bool> m_typeVisibility;  //储存各种类型的显示状态

    bool m_isMouseDoubleClicked = true;

    bool m_isMouseWheel = true;

    bool m_isMouseMove = true;

public:
    //记录当前显示内容的数据库主键
    std::string     m_primaryKey;
};

#endif

