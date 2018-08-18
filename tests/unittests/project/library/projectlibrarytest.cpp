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
#include <gtest/gtest.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/library/sym/symbol.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class ProjectLibraryTest : public ::testing::Test
{
    protected:
        FilePath mTempDir;
        FilePath mLibDir;
        FilePath mExistingSymbolFile;
        QScopedPointer<library::Symbol> mNewSymbol;
        FilePath mNewSymbolFile;

        ProjectLibraryTest() {
            mTempDir = FilePath::getRandomTempPath();
            mLibDir = mTempDir.getPathTo("project library test");

            // create component inside project library
            library::Symbol sym(Uuid::createRandom(), Version::fromString("1"),
                                "", ElementName("Existing Symbol"), "", "");
            sym.saveIntoParentDirectory(mLibDir.getPathTo("sym"));
            mExistingSymbolFile = mLibDir.getPathTo(QString("sym/%1/symbol.lp")
                                                    .arg(sym.getUuid().toStr()));

            // create symbol outside the project library (emulating workspace library)
            mNewSymbol.reset(new library::Symbol(Uuid::createRandom(), Version::fromString("1"),
                                                 "", ElementName("New Symbol"), "", ""));
            mNewSymbol->saveIntoParentDirectory(mTempDir);
            mNewSymbolFile = mLibDir.getPathTo(QString("sym/%1/symbol.lp")
                                               .arg(mNewSymbol->getUuid().toStr()));
        }

        virtual ~ProjectLibraryTest() {
            QDir(mTempDir.toStr()).removeRecursively();
        }

        library::Symbol* getFirstSymbol(ProjectLibrary& lib) {
            if (lib.getSymbols().isEmpty()) throw LogicError(__FILE__, __LINE__);
            return lib.getSymbols().values().first();
        }

        void save(ProjectLibrary& lib, bool toOriginal) {
            QStringList errors;
            bool success = lib.save(toOriginal, errors);
            ASSERT_TRUE(success);
            ASSERT_TRUE(errors.isEmpty());
        }

        void saveToTemporary(ProjectLibrary& lib) {
            save(lib, false);
        }

        void saveToOriginal(ProjectLibrary& lib) {
            save(lib, true);
        }
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(ProjectLibraryTest, testOpen)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testAddNewSymbol)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.addSymbol(*mNewSymbol.take());
        EXPECT_EQ(2, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
        EXPECT_FALSE(mNewSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    EXPECT_FALSE(mNewSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testAddNewSymbol_SaveToTemporary)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.addSymbol(*mNewSymbol.take());
        saveToTemporary(lib);
        EXPECT_EQ(2, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
        EXPECT_TRUE(mNewSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    EXPECT_FALSE(mNewSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testAddNewSymbol_SaveToOriginal)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.addSymbol(*mNewSymbol.take());
        saveToOriginal(lib);
        EXPECT_EQ(2, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
        EXPECT_TRUE(mNewSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    EXPECT_TRUE(mNewSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testAddNewSymbol_RemoveNewSymbol)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.addSymbol(*mNewSymbol);
        lib.removeSymbol(*mNewSymbol.take());
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
        EXPECT_FALSE(mNewSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    EXPECT_FALSE(mNewSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testAddNewSymbol_RemoveNewSymbol_SaveToTemporary)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.addSymbol(*mNewSymbol);
        lib.removeSymbol(*mNewSymbol.take());
        saveToTemporary(lib);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
        EXPECT_FALSE(mNewSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    EXPECT_FALSE(mNewSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testAddNewSymbol_RemoveNewSymbol_SaveToOriginal)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.addSymbol(*mNewSymbol);
        lib.removeSymbol(*mNewSymbol.take());
        saveToOriginal(lib);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
        EXPECT_FALSE(mNewSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    EXPECT_FALSE(mNewSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.removeSymbol(*getFirstSymbol(lib));
        EXPECT_EQ(0, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol_SaveToTemporary)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.removeSymbol(*getFirstSymbol(lib));
        saveToTemporary(lib);
        EXPECT_EQ(0, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol_SaveToOriginal)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        lib.removeSymbol(*getFirstSymbol(lib));
        saveToOriginal(lib);
        EXPECT_EQ(0, lib.getSymbols().count());
        EXPECT_FALSE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_FALSE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol_AddExistingSymbol)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        library::Symbol* sym = getFirstSymbol(lib);
        lib.removeSymbol(*sym);
        lib.addSymbol(*sym);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol_AddExistingSymbol_SaveToTemporary)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        library::Symbol* sym = getFirstSymbol(lib);
        lib.removeSymbol(*sym);
        lib.addSymbol(*sym);
        saveToTemporary(lib);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol_AddExistingSymbol_SaveToOriginal)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        library::Symbol* sym = getFirstSymbol(lib);
        lib.removeSymbol(*sym);
        lib.addSymbol(*sym);
        saveToOriginal(lib);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

TEST_F(ProjectLibraryTest, testRemoveExistingSymbol_Save_AddExistingSymbol_Save)
{
    {
        ProjectLibrary lib(mLibDir, false, false);
        library::Symbol* sym = getFirstSymbol(lib);
        lib.removeSymbol(*sym);
        saveToTemporary(lib);
        saveToOriginal(lib);
        EXPECT_EQ(0, lib.getSymbols().count());
        EXPECT_FALSE(mExistingSymbolFile.isExistingFile());
        lib.addSymbol(*sym);
        saveToTemporary(lib);
        saveToOriginal(lib);
        EXPECT_EQ(1, lib.getSymbols().count());
        EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
    }
    EXPECT_TRUE(mExistingSymbolFile.isExistingFile());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace project
} // namespace librepcb
