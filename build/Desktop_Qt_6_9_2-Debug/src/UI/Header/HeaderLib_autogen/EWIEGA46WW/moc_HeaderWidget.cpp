/****************************************************************************
** Meta object code from reading C++ file 'HeaderWidget.hpp'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../src/UI/Header/HeaderWidget.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'HeaderWidget.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.2. It"
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
struct qt_meta_tag_ZN12HeaderWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto HeaderWidget::qt_create_metaobjectdata<qt_meta_tag_ZN12HeaderWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "HeaderWidget",
        "minimizeRequested",
        "",
        "maximizeRequested",
        "closeRequested",
        "openFileRequested",
        "saveFileRequested",
        "captureStartRequested",
        "analyzeFlowRequested",
        "analyzeStatisticsRequested"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'minimizeRequested'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'maximizeRequested'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'closeRequested'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'openFileRequested'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'saveFileRequested'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'captureStartRequested'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analyzeFlowRequested'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analyzeStatisticsRequested'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<HeaderWidget, qt_meta_tag_ZN12HeaderWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject HeaderWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12HeaderWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12HeaderWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12HeaderWidgetE_t>.metaTypes,
    nullptr
} };

void HeaderWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<HeaderWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->minimizeRequested(); break;
        case 1: _t->maximizeRequested(); break;
        case 2: _t->closeRequested(); break;
        case 3: _t->openFileRequested(); break;
        case 4: _t->saveFileRequested(); break;
        case 5: _t->captureStartRequested(); break;
        case 6: _t->analyzeFlowRequested(); break;
        case 7: _t->analyzeStatisticsRequested(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::minimizeRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::maximizeRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::closeRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::openFileRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::saveFileRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::captureStartRequested, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::analyzeFlowRequested, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (HeaderWidget::*)()>(_a, &HeaderWidget::analyzeStatisticsRequested, 7))
            return;
    }
}

const QMetaObject *HeaderWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HeaderWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12HeaderWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int HeaderWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void HeaderWidget::minimizeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void HeaderWidget::maximizeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void HeaderWidget::closeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void HeaderWidget::openFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void HeaderWidget::saveFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void HeaderWidget::captureStartRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void HeaderWidget::analyzeFlowRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void HeaderWidget::analyzeStatisticsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
