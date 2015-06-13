/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <QPrinter>
#include "bgi_footprintpad.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_footprint.h"
#include "../board.h"
#include "../../project.h"
#include <eda4ucommon/boardlayer.h>
#include "../../../workspace/workspace.h"
#include "../../../workspace/settings/workspacesettings.h"
#include <eda4ulibrary/fpt/footprintpad.h>
#include "../../settings/projectsettings.h"
#include "../componentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_FootprintPad::BGI_FootprintPad(BI_FootprintPad& pad) noexcept :
    BGI_Base(), mPad(pad), mLibPad(pad.getLibPad())
{
    setZValue(Board::ZValue_FootprintsBottom);
    //QStringList localeOrder = mPin.getSymbol().getSchematic().getProject().getSettings().getLocaleOrder(true);
    //setToolTip(mLibPin.getName(localeOrder) % ": " % mLibPin.getDescription(localeOrder));

    mStaticText.setTextFormat(Qt::PlainText);
    mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

    mFont.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setFamily("Nimbus Sans L");
    mFont.setPixelSize(5);

    updateCacheAndRepaint();
}

BGI_FootprintPad::~BGI_FootprintPad() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_FootprintPad::updateCacheAndRepaint() noexcept
{
    mShape = QPainterPath();
    mShape.setFillRule(Qt::WindingFill);
    mBoundingRect = QRectF();

    // rotation
    Angle absAngle = /*mLibPad.getRotation() +*/ mPad.getFootprint().getRotation();
    mRotate180 = (absAngle <= -Angle::deg90() || absAngle > Angle::deg90());

    // circle
    //mShape.addEllipse(-mRadiusPx, -mRadiusPx, 2*mRadiusPx, 2*mRadiusPx);
    //mBoundingRect = mBoundingRect.united(mShape.boundingRect());

    // line
    //QRectF lineRect = QRectF(QPointF(0, 0), Point(0, mLibPin.getLength()).toPxQPointF()).normalized();
    //lineRect.adjust(-Length(79375).toPx(), -Length(79375).toPx(), Length(79375).toPx(), Length(79375).toPx());
    //mBoundingRect = mBoundingRect.united(lineRect).normalized();

    // text
    //qreal x = mLibPin.getLength().toPx() + 4;
    //mStaticText.setText(mPin.getDisplayText());
    //mStaticText.prepare(QTransform(), mFont);
    //mTextOrigin.setX(mRotate180 ? -x-mStaticText.size().width() : x);
    //mTextOrigin.setY(-mStaticText.size().height()/2);
    //mStaticText.prepare(QTransform().rotate(mRotate180 ? 90 : -90)
    //                                .translate(mTextOrigin.x(), mTextOrigin.y()), mFont);
    //if (mRotate180)
    //    mTextBoundingRect = QRectF(mTextOrigin.y(), mTextOrigin.x(), mStaticText.size().height(), mStaticText.size().width()).normalized();
    //else
    //    mTextBoundingRect = QRectF(mTextOrigin.y(), -mTextOrigin.x()-mStaticText.size().width(), mStaticText.size().height(), mStaticText.size().width()).normalized();
    //mBoundingRect = mBoundingRect.united(mTextBoundingRect).normalized();

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_FootprintPad::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    //const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    //const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    //const library::GenCompSignal* genCompSignal = mPin.getGenCompSignal();
    //const NetSignal* netsignal = (genCompSignal ? mPin.getGenCompSignalInstance()->getNetSignal() : nullptr);
    //bool requiredPin = mPin.getGenCompSignal()->isRequired();

    BoardLayer* layer = getBoardLayer(BoardLayer::LayerID::TopCopper);
    painter->setPen(QPen(layer->getColor(false), 0));
    painter->setBrush(layer->getColor(false));
    painter->drawRect(mLibPad.toPxQRectF());

#ifdef QT_DEBUG
    if (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect())
    {
        // draw bounding rect
        painter->setPen(QPen(Qt::red, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mBoundingRect);
    }
    /*if (Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsTextBoundingRect())
    {
        // draw text bounding rect
        painter->setPen(QPen(Qt::magenta, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(mTextBoundingRect);
    }*/
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BoardLayer* BGI_FootprintPad::getBoardLayer(uint id) const noexcept
{
    return mPad.getFootprint().getComponentInstance().getBoard().getProject().getBoardLayer(id);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project