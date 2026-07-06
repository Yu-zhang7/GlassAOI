#ifndef IMAGE_SAVER_H
#define IMAGE_SAVER_H

#include <QObject>
#include <QString>
#include <opencv2/opencv.hpp>

class ImageSaver : public QObject
{
    Q_OBJECT

public:
    explicit ImageSaver(QObject* parent = nullptr);
    void saveImageAsync(const std::string& filePath, const cv::Mat& image);

    void saveHDF5Async(const std::string& filePath, const std::vector<cv::Mat>& image);

    void saveHDF5(const std::string& filePath, const std::vector<cv::Mat>& images);

signals:
    void Signal_saved(bool success, const QString& filePath); // 保存完成信号
};

#endif // IMAGE_SAVER_H