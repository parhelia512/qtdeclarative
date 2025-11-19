// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITREADER_P_H
#define QQSTYLEKITREADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/QtQml>
#include <QtQuick/private/qquickpalette_p.h>

#include "qqstylekitcontrolproperties_p.h"
#include "qqstylekitstorage_p.h"

QT_BEGIN_NAMESPACE

class QQuickPalette;
class QQStyleKitVariation;
class QQStyleKitPropertyResolver;

class QQStyleKitReader : public QQStyleKitControlProperties
{
    Q_OBJECT
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool focused READ focused WRITE setFocused NOTIFY focusedChanged FINAL)
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(bool hovered READ hovered WRITE setHovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool vertical READ vertical WRITE setVertical NOTIFY verticalChanged FINAL)
    Q_PROPERTY(bool highlighted READ highlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    Q_PROPERTY(QQuickPalette *palette READ palette WRITE setPalette NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QQStyleKitControlProperties *global READ global CONSTANT FINAL)

    QML_NAMED_ELEMENT(StyleKitReader)

public:
    enum ControlType {
        Unknown = 100000,
        Control,
        AbstractButton,
        Button,
        CheckBox,
        ComboBox,
        FlatButton,
        Slider,
        SpinBox,
        SwitchControl,
        SearchField,
        TextArea,
        TextField,
        TextInput,
        RadioButton,
        ItemDelegate,
        Popup,
        Menu,
        Dialog,
        Pane,
        Page,
        Frame
    };
    Q_ENUM(ControlType)

    enum class AlternateState {
        Alternate1,
        Alternate2
    };
    Q_ENUM(AlternateState)

    QQStyleKitReader(QObject *parent = nullptr);
    ~QQStyleKitReader();

    int type() const;
    void setType(int type);
#ifdef QT_DEBUG
    ControlType typeAsControlType() const;
#endif

    bool hovered() const;
    void setHovered(bool hovered);

    bool enabled() const;
    void setEnabled(bool enabled);

    bool focused() const;
    void setFocused(bool focused);

    bool checked() const;
    void setChecked(bool checked);

    bool pressed() const;
    void setPressed(bool pressed);

    QQuickPalette *palette() const;
    void setPalette(QQuickPalette *palette);

    bool vertical() const;
    void setVertical(bool vertical);

    bool highlighted() const;
    void setHighlighted(bool highlighted);

    QQStyleKitControlProperties *global() const;

    QVariant readStyleProperty(PropertyStorageId key) const;
    void writeStyleProperty(PropertyStorageId key, const QVariant &value);
    void clearLocalStorage();

    QQSK::State controlState() const;

    static void setTransitionEnabled(bool enabled);
    static bool transitionEnabled();
    static void resetAll();

    static QList<QQStyleKitReader *> s_allReaders;

signals:
    void typeChanged();
    void customTypeChanged();
    void propertiesChanged();
    void enabledChanged();
    void focusedChanged();
    void checkedChanged();
    void hoveredChanged();
    void pressedChanged();
    void paletteChanged();
    void verticalChanged();
    void highlightedChanged();

private:
    void updateControl();
    void populateLocalStorage();
    bool dontEmitChangedSignals() const;

    QQuickStateGroup *stateGroup();
    QQmlComponent *createControlChangesComponent() const;
    QQmlComponent *createDelegateChangesComponent(const QString &delegateName) const;
    void instantiatePropertyChanges(QQmlComponent *comp);
    void maybeTrackDelegates();

private:
    Q_DISABLE_COPY(QQStyleKitReader)

    int m_type = ControlType::Unknown;

    bool m_dontEmitChangedSignals: 1;
    bool m_effectiveVariationsDirty: 1;
    bool m_parentChainDirty: 1;
    bool m_hierarchyHasVariations: 1;

    QQuickPalette m_palette;
    mutable QQStyleKitPropertyStorage m_storage;
    AlternateState m_alternateState = AlternateState::Alternate1;
    QQSK::State m_state = QQSK::StateFlag::NoState;
    QQuickStateGroup *m_stateGroup = nullptr;
    QQSK::Delegates m_trackedDelegates = QQSK::Delegate::NoDelegate;

    QPointer<QQStyleKitReader> m_parentReader;
    QList<QQStyleKitVariation *> m_effectiveVariations;

    QQStyleKitControlProperties m_global;

    static QMap<QString, QQmlComponent *> s_propertyChangesComponents;

    friend class QQStyleKitControlProperties;
    friend class QQStyleKitPropertyResolver;
    friend class QQStyleKitPropertyGroup;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITREADER_P_H
