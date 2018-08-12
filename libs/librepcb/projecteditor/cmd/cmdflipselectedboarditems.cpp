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
#include "cmdflipselectedboarditems.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/boards/items/bi_polygon.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceedit.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/cmd/cmdboardplaneedit.h>
#include <librepcb/project/boards/boardselectionquery.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdFlipSelectedBoardItems::CmdFlipSelectedBoardItems(Board& board, Qt::Orientation orientation) noexcept :
    UndoCommandGroup(tr("Flip Board Elements")), mBoard(board), mOrientation(orientation)
{
}

CmdFlipSelectedBoardItems::~CmdFlipSelectedBoardItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdFlipSelectedBoardItems::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all selected items
    std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
    query->addSelectedFootprints();
    query->addSelectedVias();
    query->addSelectedNetLines(BoardSelectionQuery::NetLineFilter::All);
    query->addNetPointsOfNetLines(BoardSelectionQuery::NetLineFilter::All,
                                  BoardSelectionQuery::NetPointFilter::All);
    //query->addNetPointsOfVias();
    query->addNetSegmentsOfSelectedItems();
    query->addSelectedPlanes();
    query->addSelectedPolygons();
    query->addSelectedBoardStrokeTexts();
    query->addSelectedFootprintStrokeTexts();
    query->addSelectedHoles();

    // find the center of all elements
    Point center = Point(0, 0);
    int count = 0;
    foreach (BI_Footprint* footprint, query->getFootprints()) {
        center += footprint->getPosition();
        ++count;
    }
    foreach (BI_Via* via, query->getVias()) {
        center += via->getPosition();
        ++count;
    }
    foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
        center += netpoint->getPosition();
        ++count;
    }
    foreach (BI_Plane* plane, query->getPlanes()) {
        for (const Vertex& vertex : plane->getOutline().getVertices()) {
            center += vertex.getPos();
            ++count;
        }
    }
    foreach (BI_Polygon* polygon, query->getPolygons()) {
        for (const Vertex& vertex : polygon->getPolygon().getPath().getVertices()) {
            center += vertex.getPos();
            ++count;
        }
    }
    foreach (BI_StrokeText* text, query->getStrokeTexts()) {
        // do not count texts of footprints if the footprint is selected too
        if (!query->getFootprints().contains(text->getFootprint())) {
            center += text->getPosition();
            ++count;
        }
    }
    foreach (BI_Hole* hole, query->getHoles()) {
        center += hole->getPosition();
        ++count;
    }
    if (count > 0) {
        center /= count;
        center.mapToGrid(mBoard.getGridProperties().getInterval());
    } else {
        // no items selected --> nothing to do here
        undoScopeGuard.dismiss();
        return false;
    }

    // disconnect all affected netsegments
    foreach (BI_NetSegment* netsegment, query->getNetSegments()) {
        execNewChildCmd(new CmdBoardNetSegmentRemove(*netsegment)); // can throw
    }

    // flip all netpoints -> this will automatically flip netlines too
    foreach (BI_NetPoint* netpoint, query->getNetPoints()) {
        GraphicsLayer* layer = mBoard.getLayerStack().getLayer(
            GraphicsLayer::getMirroredLayerName(netpoint->getLayer().getName()));
        if (!layer) throw LogicError(__FILE__, __LINE__);
        QScopedPointer<CmdBoardNetPointEdit> cmd(new CmdBoardNetPointEdit(*netpoint));
        if (!netpoint->isAttached()) {
            cmd->setPosition(netpoint->getPosition().mirrored(mOrientation, center), false);
        }
        cmd->setLayer(*layer);
        execNewChildCmd(cmd.take());
    }

    // merge redundant netpoints from vias together
    foreach (BI_NetPoint* netpoint, query->getNetPoints()) { Q_ASSERT(netpoint);
        if (!netpoint->isAttachedToVia()) continue;
        QVector<BI_NetPoint*> otherNetPointsOnSameLayer;
        foreach (BI_NetPoint* otherNetPoint, netpoint->getNetSegment().getNetPoints()) {
            if ((otherNetPoint != netpoint)
                    && (otherNetPoint->getVia() == netpoint->getVia())
                    && (&otherNetPoint->getLayer() == &netpoint->getLayer())) {
                otherNetPointsOnSameLayer.append(otherNetPoint);
            }
        }
        foreach (BI_NetPoint* otherNetPoint, otherNetPointsOnSameLayer) {
            QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
                new CmdBoardNetSegmentAddElements(netpoint->getNetSegment()));
            QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
                new CmdBoardNetSegmentRemoveElements(netpoint->getNetSegment()));

            // replace connected netlines
            foreach (BI_NetLine* netline, netpoint->getNetSegment().getNetLines()) {
                if (&netline->getStartPoint() == otherNetPoint) {
                    cmdRemove->removeNetLine(*netline);
                    cmdAdd->addNetLine(*netpoint, netline->getEndPoint(), netline->getWidth());
                } else if (&netline->getEndPoint() == otherNetPoint) {
                    cmdRemove->removeNetLine(*netline);
                    cmdAdd->addNetLine(netline->getStartPoint(), *netpoint, netline->getWidth());
                }
            }

            // remove netpoint
            cmdRemove->removeNetPoint(*otherNetPoint);
            execNewChildCmd(cmdAdd.take());
            execNewChildCmd(cmdRemove.take());
        }
    }

    // move all vias
    foreach (BI_Via* via, query->getVias()) { Q_ASSERT(via);
        QScopedPointer<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(*via));
        cmd->setPosition(via->getPosition().mirrored(mOrientation, center), false);
        execNewChildCmd(cmd.take()); // can throw
    }

    // flip all device instances
    foreach (BI_Footprint* footprint, query->getFootprints()) { Q_ASSERT(footprint);
        QScopedPointer<CmdDeviceInstanceEdit> cmd(new CmdDeviceInstanceEdit(footprint->getDeviceInstance()));
        cmd->mirror(center, mOrientation, false); // can throw
        execNewChildCmd(cmd.take()); // can throw
    }

    // flip all planes
    foreach (BI_Plane* plane, query->getPlanes()) {
        QScopedPointer<CmdBoardPlaneEdit> cmd(new CmdBoardPlaneEdit(*plane, false));
        cmd->mirror(center, mOrientation, false);
        execNewChildCmd(cmd.take()); // can throw
    }

    // flip all polygons
    foreach (BI_Polygon* polygon, query->getPolygons()) {
        QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(polygon->getPolygon()));
        cmd->mirror(center, mOrientation, false);
        execNewChildCmd(cmd.take()); // can throw
    }

    // flip all stroke texts
    foreach (BI_StrokeText* text, query->getStrokeTexts()) {
        QScopedPointer<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(text->getText()));
        cmd->mirror(center, mOrientation, false);
        execNewChildCmd(cmd.take()); // can throw
    }

    // move all holes
    foreach (BI_Hole* hole, query->getHoles()) {
        QScopedPointer<CmdHoleEdit> cmd(new CmdHoleEdit(hole->getHole()));
        cmd->setPosition(hole->getPosition().mirrored(mOrientation, center), false);
        execNewChildCmd(cmd.take()); // can throw
    }

    // reconnect all affected netsegments
    foreach (BI_NetSegment* netsegment, query->getNetSegments()) {
        execNewChildCmd(new CmdBoardNetSegmentAdd(*netsegment)); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void CmdFlipSelectedBoardItems::flipDevice(BI_Device& device, const Point& center)
{
    // disconnect all netpoints/netlines
    foreach (BI_FootprintPad* pad, device.getFootprint().getPads()) {
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) {
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
            cmd->setPadToAttach(nullptr);
            execNewChildCmd(cmd); // can throw
        }
    }

    // flip device
    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(device);
    cmd->mirror(center, mOrientation, false); // can throw
    execNewChildCmd(cmd); // can throw

    // reconnect all netpoints/netlines and change netline layers if required
    foreach (BI_FootprintPad* pad, device.getFootprint().getPads()) {
        //if (pad->getLibPad().getTechnology() == library::FootprintPad::Technology_t::SMT) {
        //    // netline/netpoint must be flipped too (place via and change layer)
        //
        //}
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) {
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
            cmd->setPadToAttach(pad);
            execNewChildCmd(cmd); // can throw
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
