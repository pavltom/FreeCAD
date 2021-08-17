/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <assert.h>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <qmath.h>
#include <QRectF>
#include "PreferencesGui.h"
#include "QGCustomRect.h"
#include "ZVALUE.h"
#include "QGIMatting.h"

using namespace TechDrawGui;

QGIMatting::QGIMatting() :
    m_height(10.0),
    m_width(10.0),
    //m_holeStyle(0),
    m_radius(5.0)

{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    m_mat = new QGraphicsPathItem();
//    addToGroup(m_mat);

    m_border = new QGraphicsPathItem();
    addToGroup(m_border);

    m_pen.setStyle(Qt::NoPen);
    m_brush.setColor(Qt::white);
    m_brush.setStyle(Qt::SolidPattern);

    m_penB.setColor(Qt::black);
    m_brushB.setStyle(Qt::NoBrush);

    m_mat->setPen(m_pen);
    m_mat->setBrush(m_brush);
    m_border->setPen(m_penB);
    m_border->setBrush(m_brushB);

    setZValue(ZVALUE::MATTING);

    updateClipPath();
}

void QGIMatting::draw()
{
    prepareGeometryChange();

    m_border->setPath(m_clipPath);
    m_border->setZValue(+ZVALUE::MATTING);

    m_mat->setPath(m_clipPath);
    m_mat->setZValue(-ZVALUE::MATTING);
}

int QGIMatting::getHoleStyle() const
{
    return PreferencesGui::mattingStyle();
}

//need this because QQGIG only updates BR when items added/deleted.
QRectF QGIMatting::boundingRect() const
{
    return QRectF(-m_width - 1.0, -m_height - 1.0, 2.0*(m_width + 1.0), 2.0*(m_height + 1.0));
}

void QGIMatting::setRadius(double r)
{
    m_radius = r;

    updateClipPath();
}

void QGIMatting::updateClipPath()
{
    m_clipPath = QPainterPath();

    if (getHoleStyle() == 0) {
        m_clipPath.addEllipse(QRectF(-m_radius, -m_radius, 2.0*m_radius, 2.0*m_radius));
    }
    else {
        double halfSide = m_radius/M_SQRT2;
        m_clipPath.addRect(QRectF(-halfSide, -halfSide, 2.0*halfSide, 2.0*halfSide));
    }

   double radiusFudge = 1.5;       //keep slightly larger than fudge in App/DVDetail to prevent bleed through
   m_width = m_radius*radiusFudge;
   m_height = m_radius*radiusFudge;
}

void QGIMatting::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //painter->drawRect(boundingRect().adjusted(-2.0,-2.0,2.0,2.0));

    QGraphicsItemGroup::paint (painter, &myOption, widget);
}
