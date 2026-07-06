#include "imageSaver.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include <QMEtaObject>
#include <QPointer> // 添加头文件
#include "Log.hpp"
#include "HDF5/HDF5Utils.h"

// 构造函数
ImageSaver::ImageSaver(QObject* parent)
    : QObject(parent)
{
}

// 异步保存图像
void ImageSaver::saveImageAsync(const std::string& filePath, const cv::Mat& image)
{
    if (image.empty() || image.data == nullptr) {
        //std::cout << "Image is empty, cannot save asynchronously.";
        FILE_LOG_INFO("ImageSaver: Image is empty, cannot save asynchronously.");
        return;
    }

    cv::Mat imageCopy = image.clone();  // 防护性操作，防止原始图像被释放
    std::string filePathCopy = filePath;
    // 使用QPointer捕获当前对象指针（线程安全）
    QPointer<ImageSaver> safeThis(this);

    // 在后台线程执行保存操作
    QtConcurrent::run([/*safeThis, */filePathCopy, imageCopy]()
        {
            //REGISTER_THREAD_NAME("saveImageAsync"); //用于分析线程崩溃时的调用栈
            
            // 按值捕获拷贝后的图像
            try {
                bool success = false;
                if (!imageCopy.empty() && imageCopy.data != nullptr)
                {
                    success = cv::imwrite(filePathCopy, imageCopy);
                }
                //qDebug() << "Image save" << (success ? "success" : "failed")
                //    << ":" << QString::fromStdString(filePath);
                FILE_LOG_INFO("ImageSaver: Image save %s : %s", (success == true ? "success" : "failed"), filePathCopy.c_str());
                // 如果对象仍然存在，通过信号通知结果
                //if (safeThis) {
                //    QMetaObject::invokeMethod(safeThis, "signalSaved",
                //        Qt::QueuedConnection,
                //        Q_ARG(bool, success),
                //        Q_ARG(QString, QString::fromStdString(filePath)));
                //}
            }
            catch (const cv::Exception& e)
            {
                FILE_LOG_WARN("ImageSaver Error: Image save failed. OpenCV exception: %s\nfilePath: %s", e.what(), filePathCopy.c_str());
                //qCritical() << "OpenCV exception:" << e.what();
            }
            catch (...) {
                FILE_LOG_WARN("ImageSaver Error: Image save failed. Unknown exception during image save");

                //qCritical() << "Unknown exception during image save";
            }
        }
    );
}

// 异步保存HDF5文件
void ImageSaver::saveHDF5Async(const std::string& filePath, const std::vector<cv::Mat>& images)
{
    if (images.empty()) {
        //std::cout << "Image is empty, cannot save asynchronously.";
        FILE_LOG_INFO("ImageSaver: Image is empty, cannot save asynchronously.");
        return;
    }
    std::vector<cv::Mat> imagesCopy;
    imagesCopy= images; // 防护性操作，防止原始图像被释放
    std::string filePathCopy = filePath;
    // 使用QPointer捕获当前对象指针（线程安全）
    QPointer<ImageSaver> safeThis(this);

    // 在后台线程执行保存操作
    QtConcurrent::run([safeThis, filePathCopy, imagesCopy]()
        { // 按值捕获拷贝后的图像
            try {
                bool success = false;
                if (!imagesCopy.empty())
                {
                    HDF5Utils hdf5Utils;
                    success = hdf5Utils.saveImagesToHDF5(imagesCopy, filePathCopy);
                }
                FILE_LOG_INFO("ImageSaver: Image save %s : %s", (success == true ? "success" : "failed"), filePathCopy.c_str());
                // 如果对象仍然存在，通过信号通知结果
                //if (safeThis) {
                //    QMetaObject::invokeMethod(safeThis, "signalSaved",
                //        Qt::QueuedConnection,
                //        Q_ARG(bool, success),
                //        Q_ARG(QString, QString::fromStdString(filePath)));
                //}
            }
            catch (const cv::Exception& e)
            {
                qCritical() << "OpenCV exception:" << e.what();
            }
            catch (...) {
                qCritical() << "Unknown exception during image save";
            }
        }
    );
}

// 保存HDF5文件
void ImageSaver::saveHDF5(const std::string& filePath, const std::vector<cv::Mat>& images)
{

    if (images.empty()) {
        //std::cout << "Image is empty, cannot save asynchronously.";
        FILE_LOG_INFO("ImageSaver: Image is empty, cannot save asynchronously.");
        return;
    }
    std::vector<cv::Mat> imagesCopy;
    imagesCopy = images; // 防护性操作，防止原始图像被释放
    std::string filePathCopy = filePath;

    try
    {
        bool success = false;
        if (!imagesCopy.empty())
        {
            HDF5Utils hdf5Utils;
            success = hdf5Utils.saveImagesToHDF5(imagesCopy, filePathCopy);
        }
        FILE_LOG_INFO("ImageSaver: Image save %s : %s", (success == true ? "success" : "failed"), filePathCopy.c_str());
    }
    catch (const cv::Exception& e)
    {
        qCritical() << "OpenCV exception:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception during image save";
    }
}