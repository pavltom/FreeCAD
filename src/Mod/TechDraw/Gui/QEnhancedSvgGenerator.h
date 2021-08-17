
#ifndef TECHDRAWGUI_QENHANCEDSVGGENERATOR_H
#define TECHDRAWGUI_QENHANCEDSVGGENERATOR_H


#include <QBuffer>
#include <QPaintEngine>
#include <QSvgGenerator>


class QEnhancedSvgPaintEngine;

class TechDrawGuiExport QEnhancedSvgGenerator : public QSvgGenerator
{
    public:
        QEnhancedSvgGenerator();
        ~QEnhancedSvgGenerator();

        QIODevice *outputDevice();
        void setOutputDevice(QIODevice *outputDevice);

    protected:
        virtual QPaintEngine *paintEngine() const override
            { return reinterpret_cast<QPaintEngine *>(enhancedPaintEngine); }

        QEnhancedSvgPaintEngine *enhancedPaintEngine;
};

#endif // TECHDRAWGUI_QENHANCEDSVGGENERATOR_H
