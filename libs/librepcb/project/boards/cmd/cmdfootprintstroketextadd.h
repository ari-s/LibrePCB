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

#ifndef LIBREPCB_PROJECT_CMDFOOTPRINTSTROKETEXTADD_H
#define LIBREPCB_PROJECT_CMDFOOTPRINTSTROKETEXTADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommand.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_Footprint;
class BI_StrokeText;

/*****************************************************************************************
 *  Class CmdFootprintStrokeTextAdd
 ****************************************************************************************/

/**
 * @brief The CmdFootprintStrokeTextAdd class
 */
class CmdFootprintStrokeTextAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdFootprintStrokeTextAdd(BI_Footprint& footprint, BI_StrokeText& text) noexcept;
        ~CmdFootprintStrokeTextAdd() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        // Attributes from the constructor
        BI_Footprint& mFootprint;
        BI_StrokeText& mText;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDFOOTPRINTSTROKETEXTADD_H
