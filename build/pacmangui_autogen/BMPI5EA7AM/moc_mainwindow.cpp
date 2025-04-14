/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.hpp'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/mainwindow.hpp"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto pacmangui::gui::MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "pacmangui::gui::MainWindow",
        "onTabChanged",
        "",
        "index",
        "onSearchTextChanged",
        "text",
        "onSearchClicked",
        "onInstallPackage",
        "onRemovePackage",
        "onUpdatePackage",
        "onSyncAll",
        "onBatchInstall",
        "onPackageSelected",
        "QModelIndex",
        "onInstalledPackageSelected",
        "onPackageItemChanged",
        "QStandardItem*",
        "item",
        "onInstallAurPackage",
        "onUpdateAurPackages",
        "onSystemUpdate",
        "onCheckForUpdates",
        "onClearPackageCache",
        "onRemoveOrphans",
        "onCheckDatabase",
        "onFindPacnewFiles",
        "onBackupDatabase",
        "onRestoreDatabase",
        "onMaintenanceTaskFinished",
        "success",
        "message",
        "onCleanCache",
        "onClearPackageLock",
        "onCheckIntegrityAllPackages",
        "onRefreshMirrorList",
        "toggleTheme",
        "isDark",
        "openSettings",
        "onAbout",
        "onDetailPanelAnimationFinished",
        "closeDetailPanel",
        "onWaylandBackendAvailabilityChanged",
        "available",
        "onWaylandOutputChanged",
        "onWaylandPermissionChanged",
        "featureName",
        "granted",
        "onWaylandSecurityEvent",
        "eventType",
        "details",
        "onWaylandHardwareAccelerationStatusChanged",
        "onWaylandPerformanceMetricsUpdated",
        "QVariantMap",
        "metrics",
        "showStatusMessage",
        "timeout"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onTabChanged'
        QtMocHelpers::SlotData<void(int)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Slot 'onSearchClicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onInstallPackage'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemovePackage'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onUpdatePackage'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSyncAll'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBatchInstall'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPackageSelected'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 3 },
        }}),
        // Slot 'onInstalledPackageSelected'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 3 },
        }}),
        // Slot 'onPackageItemChanged'
        QtMocHelpers::SlotData<void(QStandardItem *)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Slot 'onInstallAurPackage'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onUpdateAurPackages'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSystemUpdate'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCheckForUpdates'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClearPackageCache'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveOrphans'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCheckDatabase'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFindPacnewFiles'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBackupDatabase'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRestoreDatabase'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMaintenanceTaskFinished'
        QtMocHelpers::SlotData<void(bool, const QString &)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 29 }, { QMetaType::QString, 30 },
        }}),
        // Slot 'onCleanCache'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClearPackageLock'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCheckIntegrityAllPackages'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRefreshMirrorList'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'toggleTheme'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'toggleTheme'
        QtMocHelpers::SlotData<void(bool)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 36 },
        }}),
        // Slot 'openSettings'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAbout'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDetailPanelAnimationFinished'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'closeDetailPanel'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWaylandBackendAvailabilityChanged'
        QtMocHelpers::SlotData<void(bool)>(41, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 42 },
        }}),
        // Slot 'onWaylandOutputChanged'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWaylandPermissionChanged'
        QtMocHelpers::SlotData<void(const QString &, bool)>(44, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 45 }, { QMetaType::Bool, 46 },
        }}),
        // Slot 'onWaylandSecurityEvent'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(47, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 48 }, { QMetaType::QString, 49 },
        }}),
        // Slot 'onWaylandHardwareAccelerationStatusChanged'
        QtMocHelpers::SlotData<void(bool)>(50, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 42 },
        }}),
        // Slot 'onWaylandPerformanceMetricsUpdated'
        QtMocHelpers::SlotData<void(const QVariantMap &)>(51, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 52, 53 },
        }}),
        // Method 'showStatusMessage'
        QtMocHelpers::MethodData<void(const QString &, int)>(54, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 30 }, { QMetaType::Int, 55 },
        }}),
        // Method 'showStatusMessage'
        QtMocHelpers::MethodData<void(const QString &)>(54, 2, QMC::AccessPrivate | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 30 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject pacmangui::gui::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t>.metaTypes,
    nullptr
} };

void pacmangui::gui::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onTabChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->onSearchTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->onSearchClicked(); break;
        case 3: _t->onInstallPackage(); break;
        case 4: _t->onRemovePackage(); break;
        case 5: _t->onUpdatePackage(); break;
        case 6: _t->onSyncAll(); break;
        case 7: _t->onBatchInstall(); break;
        case 8: _t->onPackageSelected((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 9: _t->onInstalledPackageSelected((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 10: _t->onPackageItemChanged((*reinterpret_cast< std::add_pointer_t<QStandardItem*>>(_a[1]))); break;
        case 11: _t->onInstallAurPackage(); break;
        case 12: _t->onUpdateAurPackages(); break;
        case 13: _t->onSystemUpdate(); break;
        case 14: _t->onCheckForUpdates(); break;
        case 15: _t->onClearPackageCache(); break;
        case 16: _t->onRemoveOrphans(); break;
        case 17: _t->onCheckDatabase(); break;
        case 18: _t->onFindPacnewFiles(); break;
        case 19: _t->onBackupDatabase(); break;
        case 20: _t->onRestoreDatabase(); break;
        case 21: _t->onMaintenanceTaskFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 22: _t->onCleanCache(); break;
        case 23: _t->onClearPackageLock(); break;
        case 24: _t->onCheckIntegrityAllPackages(); break;
        case 25: _t->onRefreshMirrorList(); break;
        case 26: _t->toggleTheme(); break;
        case 27: _t->toggleTheme((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 28: _t->openSettings(); break;
        case 29: _t->onAbout(); break;
        case 30: _t->onDetailPanelAnimationFinished(); break;
        case 31: _t->closeDetailPanel(); break;
        case 32: _t->onWaylandBackendAvailabilityChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 33: _t->onWaylandOutputChanged(); break;
        case 34: _t->onWaylandPermissionChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 35: _t->onWaylandSecurityEvent((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 36: _t->onWaylandHardwareAccelerationStatusChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 37: _t->onWaylandPerformanceMetricsUpdated((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 38: _t->showStatusMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 39: _t->showStatusMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *pacmangui::gui::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pacmangui::gui::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9pacmangui3gui10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int pacmangui::gui::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 40)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 40;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 40)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 40;
    }
    return _id;
}
QT_WARNING_POP
