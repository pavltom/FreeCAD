
#include "PreCompiled.h"
#ifndef _PreComp_
# include <QPainter>
# include <QPaintEngine>
# include <QPaintEngineState>
#endif

#include <QXmlQuery>
#include <QXmlResultItems>

#include <Base/Console.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/QDomNodeModel.h>

#include "QEnhancedSvgGenerator.h"

struct QPainterClipInfoData
{
    enum ClipType { RegionClip, PathClip, RectClip, RectFClip };

    ClipType clipType;
    QTransform matrix;
    Qt::ClipOperation operation;
    QPainterPath path;
    QRegion region;
    QRect rect;
    QRectF rectf;
};

class TechDrawGuiExport QModifiablePaintEngineState : public QPaintEngineState
{
    public:
        QModifiablePaintEngineState();
        virtual ~QModifiablePaintEngineState();

        QPen &penRef() { return pen; }
        QBrush &brushRef() { return brush; }
        QPointF &brushOriginRef() { return brushOrigin; }
        QBrush &backgroundBrushRef() { return bgBrush; }
        Qt::BGMode &backgroundModeRef() { return bgMode; }
        QFont &fontRef() { return font; }
        QTransform &transformRef() { return matrix; }

        Qt::ClipOperation &clipOperationRef() { return clipOperation; }
        QRegion &clipRegionRef() { return clipRegion; }
        QPainterPath &clipPathRef() { return clipPath; }
        void setClipEnabled(bool enabled) { clipEnabled = enabled; }

        QPainter::RenderHints &renderHintsRef() { return renderHints; }
        QPainter::CompositionMode &compositionModeRef() { return composition_mode; }
        qreal &opacityRef() { return opacity; }

        void setPainter(QPainter *p) { painter = p; }

    protected:
        QPointF brushOrigin;
        QFont font;
        QFont deviceFont;
        QPen pen;
        QBrush brush;
        QBrush bgBrush;
        QRegion clipRegion;
        QPainterPath clipPath;
        Qt::ClipOperation clipOperation;
        QPainter::RenderHints renderHints;
        QVector<QPainterClipInfoData> clipInfo;
        QTransform worldMatrix;
        QTransform matrix;
        QTransform redirectionMatrix;
        int wx, wy, ww, wh;
        int vx, vy, vw, vh;
        qreal opacity;

        uint WxF:1;
        uint VxF:1;
        uint clipEnabled:1;

        Qt::BGMode bgMode;
        QPainter *painter;
        Qt::LayoutDirection layoutDirection;
        QPainter::CompositionMode composition_mode;
        uint emulationSpecifier;
        uint changeFlags;
};

class TechDrawGuiExport QEnhancedSvgPaintEngine : public QPaintEngine
{
    public:
        QEnhancedSvgPaintEngine(QPaintEngine *svgPaintEngine);

        virtual bool begin(QPaintDevice *pdev) override;
        virtual bool end() override;

        virtual void updateState(const QPaintEngineState &state) override;

        virtual void drawEllipse(const QRectF &r) override;
        virtual void drawPath(const QPainterPath &path) override;
        virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
        virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
        virtual void drawRects(const QRectF *rects, int rectCount) override;
        virtual void drawTextItem(const QPointF &pt, const QTextItem &item) override;
        virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                               Qt::ImageConversionFlags flags = Qt::AutoColor) override;

        virtual QPaintEngine::Type type() const override;

        QIODevice *outputDevice() { return targetDevice; }
        void setOutputDevice(QIODevice *outputDevice) { targetDevice = outputDevice; }

        QBuffer *outputBuffer() { return &svgBuffer; }

    protected:
        QPaintEngine *svgPaintEngine;
        QIODevice *targetDevice;
        QBuffer svgBuffer;

        QHash<int, QPainterPath> clipMap;
        int currentClipId;
        int lastClipId;
};

QEnhancedSvgPaintEngine::QEnhancedSvgPaintEngine(QPaintEngine *svgPaintEngine)
    : QPaintEngine(static_cast<QEnhancedSvgPaintEngine *>(svgPaintEngine)->gccaps), targetDevice(nullptr),
      currentClipId(0), lastClipId(0)
{
    this->svgPaintEngine = svgPaintEngine;
}

bool QEnhancedSvgPaintEngine::begin(QPaintDevice *pdev)
{
    svgBuffer.buffer().clear();

    clipMap.clear();
    currentClipId = 0;
    lastClipId = 0;

    return svgPaintEngine->begin(pdev);
}

bool QEnhancedSvgPaintEngine::end()
{
    if (!svgPaintEngine->end()) {
        return false;
    }

    QDomDocument doc(QString::fromUtf8("SVG"));

//    doc.setContent(svgBuffer.buffer());
    doc.setContent(QString::fromUtf8(
"<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
"<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n"
"  <g>X</g>\n"
"  <g fill='2'>Y</g>\n"
"  <g fill='abc'>Z</g>\n"
"</svg>\n"
));

    QDomElement docElem = doc.documentElement();
//    docElem.setAttribute(QString::fromUtf8("xmlns:svg"), QString::fromUtf8(SVG_NS_URI));

Base::Console().Warning("%s\n", doc.toString().toStdString().c_str());

    QXmlQuery query(QXmlQuery::XQuery10);
    QDomNodeModel model(query.namePool(), doc);
    query.setFocus(QXmlItem(model.fromDomNode(doc.documentElement())));

    // XPath query to select all <g> nodes with font-family attribute containing "|clipId="
    query.setQuery(QString::fromUtf8("declare default element namespace \"http://www.w3.org/2000/svg\"; "
                                     "declare namespace xlink=\"http://www.w3.org/1999/xlink\"; "
        //"//g[contains(@font-family, '|clipId=')]"));        
        "//g[contains(@fill, 'b')]"));

    QXmlResultItems queryResult;
    query.evaluateTo(&queryResult);

    Base::Console().Warning("List: %s\n", queryResult.hasError() ? "Err" : "Ok");

    while (!queryResult.next().isNull()) {
        Base::Console().Warning("-Item: ");
        QDomElement g(model.toDomNode(queryResult.current().toNodeModelIndex()).toElement());
        Base::Console().Warning("%s\n", g.attribute(QString::fromUtf8("fill")).toStdString().c_str());
    }

    if (targetDevice != nullptr) {

        bool close = false;
        if (!targetDevice->isOpen()) {
            if (!targetDevice->open(QIODevice::WriteOnly | QIODevice::Text)) {
                return false;
            }

            close = true;
        }
        else if (!targetDevice->isWritable()) {
            return false;
        }

        targetDevice->write(svgBuffer.buffer());

        if (close) {
            targetDevice->close();
        }
    }

    return true;
}

void QEnhancedSvgPaintEngine::updateState(const QPaintEngineState &state)
{
    if (state.isClipEnabled()) {
        QPainterPath clipPath = state.clipPath();

        if (!clipPath.isEmpty()) {
            int i = clipMap.values().indexOf(clipPath);

            if (i < 0) {
                currentClipId = ++lastClipId;
                clipMap.insert(currentClipId, clipPath);
            }
            else {
                currentClipId = clipMap.keys()[i];
            }
        }
        else {
            currentClipId = 0;
        }
    }
    else {
        currentClipId = 0;
    }

    if (currentClipId) {
        QModifiablePaintEngineState &modState = static_cast<QModifiablePaintEngineState &>
            (const_cast<QPaintEngineState &>(state));

        QFont &font = modState.fontRef();
        QString ff = font.family();

        font.setFamily(ff + QString::fromUtf8("|clipId") + QString::number(currentClipId));
        svgPaintEngine->updateState(state);
        font.setFamily(ff);
    }
    else {
        static_cast<QEnhancedSvgPaintEngine *>(svgPaintEngine)->state = const_cast<QPaintEngineState *>(&state);
        svgPaintEngine->updateState(state);
    }
}

void QEnhancedSvgPaintEngine::drawEllipse(const QRectF &r)
{
    svgPaintEngine->drawEllipse(r);
}

void QEnhancedSvgPaintEngine::drawPath(const QPainterPath &path)
{
    svgPaintEngine->drawPath(path);
}

void QEnhancedSvgPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    svgPaintEngine->drawPixmap(r, pm, sr);
}

void QEnhancedSvgPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    svgPaintEngine->drawPolygon(points, pointCount, mode);
}

void QEnhancedSvgPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    svgPaintEngine->drawRects(rects, rectCount);
}

void QEnhancedSvgPaintEngine::drawTextItem(const QPointF &pt, const QTextItem &item)
{
    svgPaintEngine->drawTextItem(pt, item);
}

void QEnhancedSvgPaintEngine::drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                                        Qt::ImageConversionFlags flags)
{
    svgPaintEngine->drawImage(r, pm, sr, flags);
}

QPaintEngine::Type QEnhancedSvgPaintEngine::type() const
{
    return svgPaintEngine->type();
}


QEnhancedSvgGenerator::QEnhancedSvgGenerator()
    : enhancedPaintEngine(new QEnhancedSvgPaintEngine(QSvgGenerator::paintEngine()))
{
    QSvgGenerator::setOutputDevice(enhancedPaintEngine->outputBuffer());
}

QEnhancedSvgGenerator::~QEnhancedSvgGenerator()
{
    delete enhancedPaintEngine;
    enhancedPaintEngine = nullptr;
}

QIODevice *QEnhancedSvgGenerator::outputDevice()
{
    return enhancedPaintEngine->outputDevice();
}

void QEnhancedSvgGenerator::setOutputDevice(QIODevice *outputDevice)
{
    enhancedPaintEngine->setOutputDevice(outputDevice);
}
