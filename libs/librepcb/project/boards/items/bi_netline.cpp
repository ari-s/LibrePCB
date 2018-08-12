/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "bi_netline.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "bi_netpoint.h"
#include "bi_netsegment.h"
#include "../../project.h"
#include "../../circuit/netsignal.h"
#include "bi_footprint.h"
#include "bi_footprintpad.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/scopeguard.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_NetLine::BI_NetLine(const BI_NetLine& other, BI_NetPoint& startPoint, BI_NetPoint& endPoint) :
    BI_Base(startPoint.getBoard()), mPosition(other.mPosition), mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mLayer(other.mLayer), mWidth(other.mWidth)
{
    init();
}

BI_NetLine::BI_NetLine(BI_NetSegment& segment, const SExpression& node,
                       const QHash<BI_NetPoint*, QString>& netpointLayerMap,
                       const QHash<Uuid, Uuid>& netPointReplacements) :
    BI_Base(segment.getBoard()),
    mPosition(),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mStartPoint(nullptr),
    mEndPoint(nullptr),
    mLayer(nullptr),
    mWidth(node.getValueByPath<PositiveLength>("width"))
{
    Uuid spUuid = node.getValueByPath<Uuid>("p1");
    auto it = netPointReplacements.find(spUuid);
    if (it != netPointReplacements.end()) spUuid = *it;
    mStartPoint = segment.getNetPointByUuid(spUuid);
    if(!mStartPoint) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(spUuid.toStr()));
    }

    Uuid epUuid = node.getValueByPath<Uuid>("p2");
    it = netPointReplacements.find(epUuid);
    if (it != netPointReplacements.end()) epUuid = *it;
    mEndPoint = segment.getNetPointByUuid(epUuid);
    if(!mEndPoint) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net point UUID: \"%1\""))
            .arg(epUuid.toStr()));
    }

    QString layerName = netpointLayerMap.value(mStartPoint); // backward compatibility, remove this some time!
    if (const SExpression* layerNode = node.tryGetChildByPath("layer")) {
        layerName = layerNode->getValueOfFirstChild<QString>();
    }
    mLayer = mBoard.getLayerStack().getLayer(layerName);
    if (!mLayer) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid board layer: \"%1\""))
            .arg(layerName));
    }

    init();
}

BI_NetLine::BI_NetLine(BI_NetPoint& startPoint, BI_NetPoint& endPoint,
                       GraphicsLayer& layer, const PositiveLength& width) :
    BI_Base(startPoint.getBoard()), mPosition(), mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint), mEndPoint(&endPoint), mLayer(&layer), mWidth(width)
{
    init();
}

void BI_NetLine::init()
{
    // check if both netpoints are in the same net segment
    if (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment()) {
        throw LogicError(__FILE__, __LINE__,
            tr("BI_NetLine: endpoints netsegment mismatch."));
    }

    // check layer
    if (!mLayer->isCopperLayer()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The layer of netpoint \"%1\" is invalid (%2)."))
            .arg(mUuid.toStr()).arg(mLayer->getName()));
    }

    // check if both netpoints are different
    if (mStartPoint == mEndPoint) {
        throw LogicError(__FILE__, __LINE__,
            tr("BI_NetLine: both endpoints are the same."));
    }

    mGraphicsItem.reset(new BGI_NetLine(*this));
    updateLine();

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

BI_NetLine::~BI_NetLine() noexcept
{
    mGraphicsItem.reset();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

BI_NetSegment& BI_NetLine::getNetSegment() const noexcept
{
    Q_ASSERT(mStartPoint && mEndPoint);
    Q_ASSERT(&mStartPoint->getNetSegment() == &mEndPoint->getNetSegment());
    return mStartPoint->getNetSegment();
}

BI_NetPoint* BI_NetLine::getOtherPoint(const BI_NetPoint& firstPoint) const noexcept
{
    if (&firstPoint == mStartPoint) {
        return mEndPoint;
    } else if (&firstPoint == mEndPoint) {
        return mStartPoint;
    } else {
        return nullptr;
    }
}

NetSignal& BI_NetLine::getNetSignalOfNetSegment() const noexcept
{
    return getNetSegment().getNetSignal();
}

bool BI_NetLine::isAttached() const noexcept
{
    return (mStartPoint->isAttached() || mEndPoint->isAttached());
}

bool BI_NetLine::isAttachedToVia() const noexcept
{
    return (mStartPoint->isAttachedToVia() || mEndPoint->isAttachedToVia());
}

bool BI_NetLine::isAttachedToFootprint() const noexcept
{
    return (mStartPoint->isAttachedToPad() || mEndPoint->isAttachedToPad());
}

Path BI_NetLine::getSceneOutline(const Length& expansion) const noexcept
{
    Length width = mWidth + (expansion * 2);
    if (width > 0) {
        return Path::obround(mStartPoint->getPosition(), mEndPoint->getPosition(),
                             PositiveLength(width));
    } else {
        return Path();
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

//void BI_NetLine::setLayer(GraphicsLayer& layer)
//{
//    if (&layer != mLayer) {
//        if (isUsed() || isAttached() || (!layer.isCopperLayer())) {
//            throw LogicError(__FILE__, __LINE__);
//        }
//        mLayer = &layer;
//    }
//}

void BI_NetLine::setWidth(const PositiveLength& width) noexcept
{
    if (width != mWidth) {
        mWidth = width;
        mGraphicsItem->updateCacheAndRepaint();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_NetLine::addToBoard()
{
    if (isAddedToBoard()
        || (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment())
        /*|| (&mStartPoint->getLayer() != &mEndPoint->getLayer())*/)
    {
        throw LogicError(__FILE__, __LINE__);
    }

    mStartPoint->registerNetLine(*this); // can throw
    auto sg = scopeGuard([&](){mStartPoint->unregisterNetLine(*this);});
    mEndPoint->registerNetLine(*this); // can throw

    mHighlightChangedConnection = connect(&getNetSignalOfNetSegment(),
                                              &NetSignal::highlightedChanged,
                                              [this](){mGraphicsItem->update();});
    BI_Base::addToBoard(mGraphicsItem.data());
    sg.dismiss();
}

void BI_NetLine::removeFromBoard()
{
    if ((!isAddedToBoard())
        || (&mStartPoint->getNetSegment() != &mEndPoint->getNetSegment())
        /*|| (&mStartPoint->getLayer() != &mEndPoint->getLayer())*/)
    {
        throw LogicError(__FILE__, __LINE__);
    }

    mStartPoint->unregisterNetLine(*this); // can throw
    auto sg = scopeGuard([&](){mEndPoint->registerNetLine(*this);});
    mEndPoint->unregisterNetLine(*this); // can throw

    disconnect(mHighlightChangedConnection);
    BI_Base::removeFromBoard(mGraphicsItem.data());
    sg.dismiss();
}

void BI_NetLine::updateLine() noexcept
{
    mPosition = (mStartPoint->getPosition() + mEndPoint->getPosition()) / 2;
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_NetLine::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendChild(mUuid);
    root.appendChild("layer", SExpression::createToken(mLayer->getName()), false);
    root.appendChild("width", mWidth, false);
    root.appendChild("p1", mStartPoint->getUuid(), true);
    root.appendChild("p2", mEndPoint->getUuid(), true);
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_NetLine::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->shape();
}

bool BI_NetLine::isSelectable() const noexcept
{
    return mGraphicsItem->isSelectable();
}

void BI_NetLine::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BI_NetLine::checkAttributesValidity() const noexcept
{
    if (mStartPoint == nullptr) return false;
    if (mEndPoint == nullptr)   return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
