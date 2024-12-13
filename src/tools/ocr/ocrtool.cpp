// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "ocrtool.h"

#include "src/utils/screenshotsaver.h"

#include <QBuffer>
#include <QPainter>
#include <QClipboard>

#ifdef OCR_ENABLED
#include <tesseract/baseapi.h>
#endif

static const int BYTES_PER_PIXEL = 4;
static const int PRE_SCAN_SCALE = 5;

OCRTool::OCRTool(QObject* parent)
  : AbstractActionTool(parent)
  , m_language("eng")
{}

bool OCRTool::closeOnButtonPressed() const
{
    return true;
}

QIcon OCRTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "ocr.svg");
}

QString OCRTool::name() const
{
    return tr("Text extractor");
}

CaptureTool::Type OCRTool::type() const
{
    return CaptureTool::TYPE_OCR;
}

QString OCRTool::description() const
{
    return tr("Extracts text from the screen to the clipboard");
}

CaptureTool* OCRTool::copy(QObject* parent)
{
    return new OCRTool(parent);
}

void OCRTool::pressed(CaptureContext& context)
{
#ifdef OCR_ENABLED
    // Initialize tesseract-ocr with the selected language
    auto api = std::make_unique<tesseract::TessBaseAPI>();
    if (api->Init(NULL, m_language.toStdString().c_str()))
        return;

    api->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_BLOCK);

    // Increase image and make it greyscale to help the detection
    const auto map = context.selectedScreenshotArea();
    const auto scaledImage = map.scaled(map.width() * PRE_SCAN_SCALE, map.height() * PRE_SCAN_SCALE, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
    const auto qimage = scaledImage.toImage().convertToFormat(QImage::Format_Grayscale8);

    // Get OCR result and save to the clipboard
    api->SetImage(qimage.constBits(), qimage.width(), qimage.height(), 1, qimage.bytesPerLine());
    QString text = QString::fromUtf8(api->GetUTF8Text());
    saveToClipboard(text);

    // Destroy used object and release memory
    api->End();

    emit requestAction(REQ_CLEAR_SELECTION);
    emit requestAction(REQ_CAPTURE_DONE_OK);
    emit requestAction(REQ_CLOSE_GUI);
#else
  Q_UNUSED(context)
#endif
}

void OCRTool::onLanguageChanged(const QString& c)
{
  m_language = c;
}
