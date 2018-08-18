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
#include <librepcb/common/exceptions.h>
#include "projectlibrary.h"
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/fileutils.h>
#include "../project.h"
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/common/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

using namespace library;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectLibrary::ProjectLibrary(const FilePath& libDir, bool restore, bool readOnly) :
    mLibraryPath(libDir)
{
    qDebug() << "load project library...";

    Q_UNUSED(restore)

    if ((!mLibraryPath.isExistingDir()) && (!readOnly)) {
        FileUtils::makePath(mLibraryPath); // can throw
    }

    try
    {
        // Load all library elements
        loadElements<Symbol>    (mLibraryPath.getPathTo("sym"),    "symbols",      mSymbols);
        loadElements<Package>   (mLibraryPath.getPathTo("pkg"),    "packages",     mPackages);
        loadElements<Component> (mLibraryPath.getPathTo("cmp"),    "components",   mComponents);
        loadElements<Device>    (mLibraryPath.getPathTo("dev"),    "devices",      mDevices);
    }
    catch (Exception &e)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mDevices);       mDevices.clear();
        qDeleteAll(mComponents);    mComponents.clear();
        qDeleteAll(mPackages);      mPackages.clear();
        qDeleteAll(mSymbols);       mSymbols.clear();
        throw; // ...and rethrow the exception
    }

    qDebug() << "project library successfully loaded!";
}

ProjectLibrary::~ProjectLibrary() noexcept
{
    // clean up all removed elements
    cleanupElements();

    // Delete all library elements (in reverse order of their creation)
    qDeleteAll(mDevices);       mDevices.clear();
    qDeleteAll(mComponents);    mComponents.clear();
    qDeleteAll(mPackages);      mPackages.clear();
    qDeleteAll(mSymbols);       mSymbols.clear();
}

/*****************************************************************************************
 *  Getters: Library Elements
 ****************************************************************************************/

Symbol* ProjectLibrary::getSymbol(const Uuid& uuid) const noexcept
{
    return mSymbols.value(uuid, 0);
}

Package* ProjectLibrary::getPackage(const Uuid& uuid) const noexcept
{
    return mPackages.value(uuid, 0);
}

Component* ProjectLibrary::getComponent(const Uuid& uuid) const noexcept
{
    return mComponents.value(uuid, 0);
}

Device* ProjectLibrary::getDevice(const Uuid& uuid) const noexcept
{
    return mDevices.value(uuid, 0);
}

/*****************************************************************************************
 *  Getters: Special Queries
 ****************************************************************************************/

QHash<Uuid, library::Device*> ProjectLibrary::getDevicesOfComponent(const Uuid& compUuid) const noexcept
{
    QHash<Uuid, library::Device*> list;
    foreach (library::Device* device, mDevices)
    {
        if (device->getComponentUuid() == compUuid)
            list.insert(device->getUuid(), device);
    }
    return list;
}

/*****************************************************************************************
 *  Add/Remove Methods
 ****************************************************************************************/

void ProjectLibrary::addSymbol(library::Symbol& s)
{
    addElement<Symbol>(s, mSymbols);
}

void ProjectLibrary::addPackage(library::Package& p)
{
    addElement<Package>(p, mPackages);
}

void ProjectLibrary::addComponent(library::Component& c)
{
    addElement<Component>(c, mComponents);
}

void ProjectLibrary::addDevice(library::Device& d)
{
    addElement<Device>(d, mDevices);
}

void ProjectLibrary::removeSymbol(library::Symbol& s)
{
    removeElement<Symbol>(s, mSymbols);
}

void ProjectLibrary::removePackage(library::Package& p)
{
    removeElement<Package>(p, mPackages);
}

void ProjectLibrary::removeComponent(library::Component& c)
{
    removeElement<Component>(c, mComponents);
}

void ProjectLibrary::removeDevice(library::Device& d)
{
    removeElement<Device>(d, mDevices);
}


/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool ProjectLibrary::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // determine what to do
    QSet<LibraryBaseElement*> currentElements = getCurrentElements();
    QSet<LibraryBaseElement*> toRemove = mTemporarySavedElements;
    if (toOriginal) toRemove += mOriginalSavedElements;
    toRemove -= currentElements;
    QSet<LibraryBaseElement*> toAdd = currentElements - mOriginalSavedElements - mTemporarySavedElements;

    // remove no longer needed elements
    foreach (LibraryBaseElement* element, toRemove) {
        Q_ASSERT(element->getFilePath().getParentDir().getParentDir() == mLibraryPath);
        try {
            element->moveIntoParentDirectory(FilePath::getRandomTempPath()); // can throw
            mTemporarySavedElements.remove(element);
            mOriginalSavedElements.remove(element);
        }
        catch (Exception& e) {
            success = false;
            errors.append(e.getMsg());
        }
    }

    // add new elements
    foreach (LibraryBaseElement* element, toAdd) {
        Q_ASSERT(element->getFilePath().getParentDir().getParentDir() != mLibraryPath);
        try {
            element->moveIntoParentDirectory(mLibraryPath.getPathTo(element->getShortElementName())); // can throw
            if (toOriginal) {
                mOriginalSavedElements.insert(element);
            } else {
                mTemporarySavedElements.insert(element);
            }
        }
        catch (Exception& e) {
            success = false;
            errors.append(e.getMsg());
        }
    }
    if (toOriginal) {
        mOriginalSavedElements += mTemporarySavedElements;
        mTemporarySavedElements.clear();
    }

    // force upgrading file format of currently added elements
    if (toOriginal) {
        foreach (LibraryBaseElement* element, currentElements - mUpgradedElements) {
            Q_ASSERT(element->getFilePath().getParentDir().getParentDir() == mLibraryPath);
            try {
                element->save(); // can throw
                mUpgradedElements.insert(element);
            }
            catch (Exception& e) {
                success = false;
                errors.append(e.getMsg());
            }
        }
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

QSet<LibraryBaseElement*> ProjectLibrary::getCurrentElements() const noexcept
{
    QSet<LibraryBaseElement*> currentElements;
    foreach (auto element, mSymbols) {currentElements.insert(element);}
    foreach (auto element, mPackages) {currentElements.insert(element);}
    foreach (auto element, mComponents) {currentElements.insert(element);}
    foreach (auto element, mDevices) {currentElements.insert(element);}
    return currentElements;
}

template <typename ElementType>
void ProjectLibrary::loadElements(const FilePath& directory, const QString& type,
                                  QHash<Uuid, ElementType*>& elementList)
{
    QDir dir(directory.toStr());

    // search all subdirectories which have a valid UUID as directory name
    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable);
    dir.setNameFilters(QStringList() << QString("*.%1").arg(directory.getBasename()));
    foreach (const QString& dirname, dir.entryList())
    {
        FilePath subdirPath(directory.getPathTo(dirname));

        // check if directory is a valid library element
        if (!LibraryBaseElement::isValidElementDirectory<ElementType>(subdirPath)) {
            if (subdirPath.isEmptyDir()) {
                qInfo() << "Empty library element directory will be removed:" << subdirPath.toNative();
                QDir(subdirPath.toStr()).removeRecursively();
            } else {
                qWarning() << "Found an invalid directory in the library:" << subdirPath.toNative();
            }
            continue;
        }

        // load the library element --> an exception will be thrown on error
        ElementType* element = new ElementType(subdirPath, false);

        if (elementList.contains(element->getUuid())) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("There are multiple library elements with the same "
                "UUID in the directory \"%1\"")).arg(subdirPath.toNative()));
        }

        elementList.insert(element->getUuid(), element);
        mOriginalSavedElements.insert(element);
    }

    qDebug() << "successfully loaded" << elementList.count() << qPrintable(type);
}

template <typename ElementType>
void ProjectLibrary::addElement(ElementType& element,
                                QHash<Uuid, ElementType*>& elementList)
{
    if (elementList.contains(element.getUuid())) {
        throw LogicError(__FILE__, __LINE__, QString(tr(
            "There is already an element with the same UUID in the project's library: %1"))
            .arg(element.getUuid().toStr()));
    }
    if (element.getFilePath().getParentDir().getParentDir() != mLibraryPath) {
        // copy from workspace *immediately* to freeze/backup their state
        element.saveIntoParentDirectory(FilePath::getRandomTempPath());
    }
    elementList.insert(element.getUuid(), &element);
}

template <typename ElementType>
void ProjectLibrary::removeElement(ElementType& element,
                                   QHash<Uuid, ElementType*>& elementList)
{
    Q_ASSERT(elementList.value(element.getUuid()) == &element);
    elementList.remove(element.getUuid());
}

void ProjectLibrary::cleanupElements() noexcept
{
    QSet<LibraryBaseElement*> toRemove = mTemporarySavedElements - mOriginalSavedElements;
    foreach (LibraryBaseElement* element, toRemove) {
        Q_ASSERT(element->getFilePath().getParentDir().getParentDir() == mLibraryPath);
        QDir(element->getFilePath().toStr()).removeRecursively();
    }
    qDeleteAll(toRemove - getCurrentElements());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
