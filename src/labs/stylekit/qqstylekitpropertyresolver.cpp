// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitpropertyresolver_p.h"
#include "qqstylekitcontrol_p.h"
#include "qqstylekitcontrols_p.h"
#include "qqstylekitvariation_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"
#include "qqstylekitdebug_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtCore/QScopedValueRollback>

QT_BEGIN_NAMESPACE

bool QQStyleKitPropertyResolver::s_styleWarningsIssued = false;
bool QQStyleKitPropertyResolver::s_isReadingProperty = false;
QQSK::State QQStyleKitPropertyResolver::s_cachedState = QQSK::StateFlag::NoState;
QVarLengthArray<QQSK::StateFlag, 10> QQStyleKitPropertyResolver::s_cachedStateList;

PropertyPathId QQStyleKitPropertyResolver::pathId(
    const QQStyleKitPropertyGroup *group, const QQSK::Property property, PathId flag)
{
    /* Follow the parent chain of the property up to the QQStyleKitControlProperties
     * group it's inside. This group path, together with the enum value of the property,
     * will form its unique PropertyPathId.
     * E.g the property 'color' will get a different path ID when it's a part of
     * 'background.color' compared to 'background.border.color'.
     * This path ID will later be used together with different state combinations to
     * form different PropertyStorageId's. A storage ID is used as the key into QMaps
     * that stores property values for each state in each QQStyleKitControl. */
    if (property == QQSK::Property::NoProperty)
        return PropertyPathId();

    const int propertyCount = int(QQSK::Property::COUNT);
    const int groupCount = int(QQSK::PropertyGroup::COUNT);

    /* Deliberatly use extra wide types for calculations, in order
     * to do rough overflow checks in the end. */
    quint64 id = qint64(property);
    quint64 idSpaceForPreviousLevel = propertyCount;

    const QQStyleKitPropertyGroup *groupParent = group;

    if (flag == PathId::ExcludeSubType && groupParent->isDelegateSubType())
        groupParent = static_cast<QQStyleKitPropertyGroup *>(groupParent->parent());

    if (groupParent->isPathFlag()) {
        /* property 'global' is not a real group, it's just a hint to the property
         * resolver that it should read style properties directly from the style, igoring
         * any ongoing transition inside the reader. So just skip it. */
        groupParent = static_cast<QQStyleKitPropertyGroup *>(groupParent->parent());
    }

    while (!groupParent->isControlProperties()) {
        // Add 1 to the group number since group 0 (with start ID == 0) is
        // reserved for properties that are not nested in a group (e.g control.implicitWidth).
        const int groupNumber = int(groupParent->group()) + 1;
        const quint64 idSpaceForCurrentGroup = groupNumber * idSpaceForPreviousLevel;

        id += idSpaceForCurrentGroup;

        /* Every time we move up one group level, all the possible property paths
         * in the previous level can theoretically occur inside each group on this
         * level. So we need to multiply this space with group count. */
        idSpaceForPreviousLevel *= groupCount;

        groupParent = static_cast<QQStyleKitPropertyGroup *>(groupParent->parent());
        if (flag == PathId::ExcludeSubType && groupParent->isDelegateSubType()) {
            groupParent = static_cast<QQStyleKitPropertyGroup *>(groupParent->parent());
            Q_ASSERT(groupParent);
        }
        Q_ASSERT(groupParent);
        if (groupParent->isPathFlag()) {
            groupParent = static_cast<QQStyleKitPropertyGroup *>(groupParent->parent());
            Q_ASSERT(groupParent);
        }
    }

    const PropertyPathId pathId(property, id, idSpaceForPreviousLevel);

#ifdef QT_DEBUG
    // Check that the id calculation didn't overflow
    Q_ASSERT_X(pathId.pathId() == id,
               __FUNCTION__, QQStyleKitDebug::propertyPath(group, property).toUtf8().constData());

    /* Also check in advance that the path ID can be used in combination with
     * any possible state combination later on to form a storage ID. */
    const QQSK::StateFlag maxNestedState = QQSK::StateFlag::MAX_STATE;
    Q_ASSERT_X(id < pathId.storageId(maxNestedState),
               __FUNCTION__, QQStyleKitDebug::propertyPath(group, property).toUtf8().constData());
#endif

    return pathId;
}

const QList<QQStyleKitExtendedControlType> QQStyleKitPropertyResolver::baseTypesForType(
    QQStyleKitExtendedControlType exactType)
{
    /* The base types should, by default, mirror the class hierarchy in Qt Quick Controls.
     * Exceptions:
     * ItemDelegate - is normally used as an menu item in a combo, or item in a ListView. It
     * is in any case styled quite differently than a button. */
    switch (exactType) {
    case QQStyleKitReader::Button:
    case QQStyleKitReader::RadioButton:
    case QQStyleKitReader::CheckBox:
    case QQStyleKitReader::SwitchControl: {
        static QList<QQStyleKitExtendedControlType> t =
            { QQStyleKitReader::AbstractButton, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::Menu:
    case QQStyleKitReader::Dialog: {
        static QList<QQStyleKitExtendedControlType> t =
            { QQStyleKitReader::Popup, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::Page:
    case QQStyleKitReader::Frame: {
        static QList<QQStyleKitExtendedControlType> t =
            { QQStyleKitReader::Pane, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::TextField:
    case QQStyleKitReader::TextArea: {
        static QList<QQStyleKitExtendedControlType> t =
            { QQStyleKitReader::TextInput, QQStyleKitReader::Control };
        return t; }
    default: {
        static QList<QQStyleKitExtendedControlType> t =
            { QQStyleKitReader::Control };
        return t; }
    }

    Q_UNREACHABLE();
    return {};
}

void QQStyleKitPropertyResolver::cacheReaderState(QQSK::State state)
{
    if (state == s_cachedState)
        return;

    s_cachedState = state;

    /* Note: The order in which we add the states below matters.
     * The reason is that the s_cachedStateList that we build is used by QQStyleKitPropertyResolver
     * later to generate all the different state combinations that should be tested when
     * searching for a property. And the states added first to the list will "win" if the
     * same property is set in several of the states. */
    s_cachedStateList.clear();
    if (state.testFlag(QQSK::StateFlag::Pressed))
        s_cachedStateList.append(QQSK::StateFlag::Pressed);
    if (state.testFlag(QQSK::StateFlag::Hovered))
        s_cachedStateList.append(QQSK::StateFlag::Hovered);
    if (state.testFlag(QQSK::StateFlag::Highlighted))
        s_cachedStateList.append(QQSK::StateFlag::Highlighted);
    if (state.testFlag(QQSK::StateFlag::Focused))
        s_cachedStateList.append(QQSK::StateFlag::Focused);
    if (state.testFlag(QQSK::StateFlag::Checked))
        s_cachedStateList.append(QQSK::StateFlag::Checked);
    if (state.testFlag(QQSK::StateFlag::Vertical))
        s_cachedStateList.append(QQSK::StateFlag::Vertical);
    if (state.testFlag(QQSK::StateFlag::Disabled))
        s_cachedStateList.append(QQSK::StateFlag::Disabled);
}

void QQStyleKitPropertyResolver::addTypeVariationsToReader(
    QQStyleKitReader *styleReader,
    const QQStyleKitExtendedControlType parentType,
    const QQStyleKitStyle *style)
{
    static PropertyPathIds ids;
    if (ids.property.property() == QQSK::Property::NoProperty) {
        /* ids is made static, since the 'variations' path will be the same for all
         * StyleKitControls. Also, since sub types are only possible for delegates,
         * and 'variations' is a control property, we can exclude sub types. */
        ids.property = pathId(styleReader, QQSK::Property::Variations, PathId::ExcludeSubType);
        ids.alternative = pathId(styleReader, QQSK::Property::NoProperty, PathId::ExcludeSubType);
        ids.subTypeProperty = PropertyPathId();
        ids.subTypeAlternative = PropertyPathId();
    }

    const auto parentBaseTypes = baseTypesForType(parentType);
    const QVariant inStyleVariationsVar = readPropertyInStyle(ids, parentType, parentBaseTypes, style);
    if (!inStyleVariationsVar.isValid())
        return;

    /* Inside each Type Variation, check if the control type that styleReader represents has
     * been defined. If so, it means that the variation might affect it, and should therefore be
     * added to the style readers list of effective variations. */
    const QQStyleKitExtendedControlType styleReaderType = styleReader->type();
    const auto styleReaderBaseType = baseTypesForType(styleReaderType);

    const auto inStyleVariations = *qvariant_cast<QList<QQStyleKitVariation *> *>(inStyleVariationsVar);
    for (auto *variation : inStyleVariations) {
        if (!variation) {
            // This happens if the user adds non QQStyleKitVariation elements to the QML array
            continue;
        }
        /* Note: when we read the variations property from the style, it returns the varations
         * set in the most specific storage/state/control (because of propagation). And those
         * are the only ones that will take effect. This means that even if there are variations
         * in the fallback style for the requested type (control) that overrides some properties,
         * and the style/theme has variations that overrides something else, the variations in
         * the fallback style will, in that case, not be used. The properties not set in the
         * effective variation will instead propagate back to be read from the type in the theme
         * or the style. This approach is easier to understand and work with, since the propagation
         * always flow in one direction, and doesn't jump back and forth between variations, styles
         * and themes. */
        if (variation->getControl(styleReaderType)) {
            styleReader->m_effectiveInStyleVariations.append(variation);
        } else {
            for (int type : styleReaderBaseType) {
                if (variation->getControl(type)) {
                    styleReader->m_effectiveInStyleVariations.append(variation);
                    break;
                }
            }
        }
    }
}

void QQStyleKitPropertyResolver::addInstanceVariationsToReader(
    QQStyleKitReader *styleReader, const QStringList &inAppVariationNames,
    const QVarLengthArray<const QQStyleKitControls *, 6> &stylesAndThemes)
{
    /* Add the variations set from the application to the list of effective variations
     * in the styleReader. But, to speed up property look-up later on, we only add the
     * variations that has the potential to affect the control type, or its base types,
     * that the styleReader represents. The variations that are closest to styleReader
     * in the hierarchy will be added first and take precendence over the ones added last. */
    const QQStyleKitExtendedControlType styleReaderType = styleReader->type();
    const auto styleReaderBaseTypes = baseTypesForType(styleReaderType);

    for (const QString &attachedVariationName : inAppVariationNames) {
        for (const QQStyleKitControls *styleOrTheme : stylesAndThemes) {
            const QList<QQStyleKitVariation *> variationsInStyleOrTheme = styleOrTheme->variations();
            for (QQStyleKitVariation *variationInStyleOrTheme : variationsInStyleOrTheme)  {
                if (variationInStyleOrTheme->name() != attachedVariationName)
                    continue;
                /* Invariant: we found a variation in a Style or a Theme with a name that matches
                 * a name in the attached variation list. Check if the found variation contains the
                 * type, or the sub types, of the style reader. If not, it doesn't affect it and can
                 * therefore be skipped. */
                if (variationInStyleOrTheme->getControl(styleReaderType)) {
                    styleReader->m_effectiveInAppVariations.append(variationInStyleOrTheme);
                } else {
                    for (int baseType : styleReaderBaseTypes) {
                        if (variationInStyleOrTheme->getControl(baseType)) {
                            styleReader->m_effectiveInAppVariations.append(variationInStyleOrTheme);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void QQStyleKitPropertyResolver::rebuildVariationsForReader(
    QQStyleKitReader *styleReader, const QQStyleKitStyle *style)
{
    /* Traverse up the parent chain of \a styleReader, and for each parent, look for an
     * instance of QQStyleKitControlAttached. And for each attached object, check if it
     * has variations that can potentially affect the style reader. If so, add the
     * variations to the style readers list of effective variations.
     * A QQStyleKitControlAttached can specify both Instance Variations and Type Variations.
     * The former should affect all descendant StyleKitReaders of the parent, while the
     * latter should only affect descendant StyleKitReaders of a specific type. */
    Q_ASSERT(styleReader->m_effectiveVariationsDirty);
    styleReader->m_effectiveVariationsDirty = false;
    styleReader->m_effectiveInAppVariations.clear();
    styleReader->m_effectiveInStyleVariations.clear();

    const bool hasInstanceVariations = QQStyleKitControlAttached::s_variationCount > 0;
    const bool styleHasVariations = QQStyleKitVariation::s_variationCount > 0;
    if (!styleHasVariations) {
        /* No variations are defined in the current style or theme. In that case
         * it doesn't matter if the application has variations set - they
         * will anyway not map to any variations in the style. */
        return;
    }

    /* We need to search through all variations defined in either the style or the theme,
     * including the ones in the fallback styles, since the variation property propagates,
     * like all the other style properties. */
    QVarLengthArray<const QQStyleKitControls *, 6> stylesAndThemes;
    const QQStyleKitStyle *styleOrFallbackStyle = style;
    while (styleOrFallbackStyle) {
        if (const auto *theme = styleOrFallbackStyle->theme())
            stylesAndThemes.append(theme);
        stylesAndThemes.append(styleOrFallbackStyle);
        styleOrFallbackStyle = styleOrFallbackStyle->fallbackStyle();
    }

    QObject *parentObj = styleReader;
    while (parentObj) {
        QObject *attachedObject = qmlAttachedPropertiesObject<QQStyleKitControl>(parentObj, false);
        if (attachedObject) {
            auto *controlAtt = static_cast<QQStyleKitControlAttached *>(attachedObject);
            if (hasInstanceVariations)
                addInstanceVariationsToReader(styleReader, controlAtt->variations(), stylesAndThemes);
            if (styleHasVariations)
                addTypeVariationsToReader(styleReader, controlAtt->controlType(), style);
        }

        parentObj = parentObj->parent();
    }
}

template <class T>
QVariant QQStyleKitPropertyResolver::readPropertyInStorageForState(
    const PropertyPathId main, const PropertyPathId alternative,
    const T *storageProvider, QQSK::State state)
{
    /* If the propertyId or altPropertyId is set in the control storage, we've found the best
     * match, and can return the value back to the application. The reason we need an altPropertyId,
     * is because some properties can be set more that one way. For example 'topLeftRadius' can be
     * set using either 'topLeftRadius' (main) or 'radius' (alternative). The first found in the
     * propagation chain wins. This means that when resolving 'topLeftRadius' for a button, if
     * 'radius' is set in 'button', and 'topLeftRadius' is set in abstractButton, 'radius' will
     * override 'topLeftRadius', and win. */
    Q_ASSERT(qlonglong(state) <= qlonglong(QQSK::StateFlag::MAX_STATE));

    const PropertyStorageId propertyKey = main.storageId(state);

    if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
        QQStyleKitDebug::trace(main, storageProvider, state, propertyKey);

    const QVariant propertyValue = storageProvider->readStyleProperty(propertyKey);
    if (propertyValue.isValid()) {
        if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
            QQStyleKitDebug::notifyPropertyRead(main, storageProvider, state, propertyValue);
        return propertyValue;
    }

    const PropertyStorageId altPropertyKey = alternative.storageId(state);

    if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
        QQStyleKitDebug::trace(alternative, storageProvider, state, altPropertyKey);

    const QVariant altValue = storageProvider->readStyleProperty(altPropertyKey);
    if (altValue.isValid()) {
        if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
            QQStyleKitDebug::notifyPropertyRead(main, storageProvider, state, altValue);
        return altValue;
    }

    return {};
}

template <class INDICES_CONTAINER>
QVariant QQStyleKitPropertyResolver::readPropertyInControlForStates(
    const PropertyPathId main, const PropertyPathId alternative,
    const QQStyleKitControl *control, INDICES_CONTAINER &stateListIndices,
    int startIndex, int recursionLevel)
{
    for (int i = startIndex; i < s_cachedStateList.length(); ++i) {
        /* stateListIndices is a helper list to track which index in the state list
         * each recursion level is currently processing. The first recursion level
         * will iterate through all of the states. The second recursion level will
         * only the iterate through the states that comes after the state at the
         * previous recursion. And so on recursively. The end result will be a list of
         * state combinations (depth first) where no state is repeated more than
         * once. And for each combination, we check if the storage has a value assigned
         * for the given property for the given state combination. */
        stateListIndices[recursionLevel] = i;
        const QQSK::StateFlag stateFlag = s_cachedStateList[i];

        if (!control->m_writtenStates.testFlag(stateFlag)) {
            /* optimization: the control doesn't have any properties for the state
             * we're processing. So continue to the next state in the list. */
            continue;
        }

        if (recursionLevel < s_cachedStateList.length() - 1) {
            // Continue the recursion towards the longest possible nested state
            const QVariant value = readPropertyInControlForStates(
                main, alternative, control, stateListIndices, i + 1, recursionLevel + 1);
            if (value.isValid())
                return value;
        }

        // Check the current combination
        QQSK::State storageState = QQSK::StateFlag::NoState;
        for (int j = 0; j <= recursionLevel; ++j)
            storageState.setFlag(s_cachedStateList[stateListIndices[j]]);
        const QVariant value = readPropertyInStorageForState(main, alternative, control, storageState);
        if (value.isValid())
            return value;
    }

    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInControl(
    const PropertyPathIds &ids, const QQStyleKitControl *control)
{
    /* Find the most specific state combination (based on the state of the reader) that
     * has a value set for the property in the contol. In case several state combinations
     * could be found, the order of the states in the stateList decides the priority.
     * If we're reading a property in a sub type, try all state combinations in the sub
     * type first, before trying all the state combinations in the super type. */
    QVarLengthArray<int, 10> stateListIndices(s_cachedStateList.length());

    if (ids.subTypeProperty.property() != QQSK::Property::NoProperty) {
        if (s_cachedState != QQSK::StateFlag::NoState) {
            QVariant value = readPropertyInControlForStates(
                ids.subTypeProperty, ids.subTypeAlternative, control, stateListIndices, 0, 0);
            if (value.isValid())
                return value;
        }

        if (control->m_writtenStates.testFlag(QQSK::StateFlag::Normal)) {
            const QVariant value = readPropertyInStorageForState(
                ids.subTypeProperty, ids.subTypeAlternative, control, QQSK::StateFlag::Normal);
            if (value.isValid())
                return value;
        }
    }

    if (s_cachedState != QQSK::StateFlag::NoState) {
        const QVariant value = readPropertyInControlForStates(
            ids.property, ids.alternative, control, stateListIndices, 0, 0);
        if (value.isValid())
            return value;
    }

    /* The normal state is the propagation fall back for all state combinations.
     * If the normal state has the property set, it'll return a valid QVariant,
     * which will cause the propagation to stop. Otherwise we'll return an invalid
     * variant which will cause the search to continue. */
    if (control->m_writtenStates.testFlag(QQSK::StateFlag::Normal))
        return readPropertyInStorageForState(ids.property, ids.alternative, control, QQSK::StateFlag::Normal);

    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInRelevantControls(
    const QQStyleKitControls *controls, const PropertyPathIds &ids,
    const QQStyleKitExtendedControlType exactType,
    const QList<QQStyleKitExtendedControlType> baseTypes)
{
    if (!controls)
        return {};

    if (const QQStyleKitControl *control = controls->getControl(exactType)) {
        const QVariant value = readPropertyInControl(ids, control);
        if (value.isValid())
            return value;
    }

    for (const int type : baseTypes) {
        if (const QQStyleKitControl *control = controls->getControl(type)) {
            const QVariant value = readPropertyInControl(ids, control);
            if (value.isValid())
                return value;
        }
    }

    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInStyle(
    const PropertyPathIds &ids,
    const QQStyleKitExtendedControlType exactType,
    const QList<QQStyleKitExtendedControlType> baseTypes,
    const QQStyleKitStyle *style)
{
    QVariant value;

    while (true) {
        value = readPropertyInRelevantControls(style->theme(), ids, exactType, baseTypes);
        if (value.isValid())
            break;
        value = readPropertyInRelevantControls(style, ids, exactType, baseTypes);
        if (value.isValid())
            break;

        if (auto *fallbackStyle = style->fallbackStyle()) {
            /* Recurse into the fallback style, and search for the property there. If not
             * found, and the fallback style has a fallback style, the recursion continues. */
            fallbackStyle->setPalette(style->palette());
            value = readPropertyInStyle(ids, exactType, baseTypes, fallbackStyle);
            if (value.isValid())
                break;
        }

        break;
    }

    if (Q_UNLIKELY(QQStyleKitDebug::enabled())) {
        if (!value.isValid())
            QQStyleKitDebug::notifyPropertyNotResolved(ids.property);
    }

    return value;
}

QVariant QQStyleKitPropertyResolver::readProperty(
    const PropertyPathIds &ids, QQStyleKitReader *styleReader, QQStyleKitStyle *style)
{
    if (styleReader->m_effectiveVariationsDirty)
        rebuildVariationsForReader(styleReader, style);

    /* Sync the palette of the style with the palette of the current reader. Note
     * that this will cause palette bindings in the style to change, which will
     * result in calls to writeStyleProperty(). */
    style->setPalette(styleReader->palette());

    const QQStyleKitExtendedControlType exactType = styleReader->type();
    const QList<QQStyleKitExtendedControlType> baseTypes = baseTypesForType(exactType);

    QVariant value;

    while (true) {
        for (const QPointer<QQStyleKitVariation> &variation : std::as_const(styleReader->m_effectiveInAppVariations)) {
            if (!variation)
                continue;
            value = readPropertyInRelevantControls(variation, ids, exactType, baseTypes);
            if (value.isValid())
                break;
        }
        if (value.isValid())
            break;

        for (const QPointer<QQStyleKitVariation> &variation : std::as_const(styleReader->m_effectiveInStyleVariations)) {
            if (!variation)
                continue;
            value = readPropertyInRelevantControls(variation, ids, exactType, baseTypes);
            if (value.isValid())
                break;
        }
        if (value.isValid())
            break;

        value = readPropertyInStyle(ids, exactType, baseTypes, style);
        break;
    }

    if (Q_UNLIKELY(QQStyleKitDebug::enabled())) {
        if (!value.isValid())
            QQStyleKitDebug::notifyPropertyNotResolved(ids.property);
    }

    return value;
}

QVariant QQStyleKitPropertyResolver::readStyleProperty(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property,
    const QQSK::Property alternative)
{
    auto [controlProperties, subType, pathFlags] = group->inspectGroupPath();
    const QQSK::Subclass subclass = controlProperties->subclass();

    if (subclass == QQSK::Subclass::QQStyleKitState) {
        /* Because of propagation, we need to know the state of the reader in order to
         * resolve the value for the given property. E.g the color of a background delegate
         * will take a difference search path, and end up with a different value, depending
         * of if the reader is a Slider or a Button, and if it's hovered or pressed etc.
         * Therefore, when a property is instead accessed from a control (or a state in a
         * control) in the UnifedStyle, we cannot support propagation. But to still allow
         * static (as in non-propagating) bindings between the properties in a Style,
         * we fall back to simply return the value specified in the accessed control. */
        const auto [control, nestedState] = controlProperties->asQQStyleKitState()->controlAndState();
        const PropertyPathId propertyPathId = pathId(group, property, PathId::IncludeSubType);
        const PropertyStorageId key = propertyPathId.storageId(nestedState);
        return control->readStyleProperty(key);
    }

    QQStyleKitStyle *style = controlProperties->style();
    if (!style) {
        if (!s_styleWarningsIssued) {
            s_styleWarningsIssued = true;
            qmlWarning(group) << "style properties cannot be read: No StyleKit style has been set!";
        }
        return {};
    }

    if (!style->loaded()) {
        // Optimization: Skip reads until both the style and the theme is ready
        return {};
    }

    if (subclass == QQSK::Subclass::QQStyleKitReader) {
        QQStyleKitDebug::groupBeingRead = group;
        QQStyleKitReader *styleReader = controlProperties->asQQStyleKitReader();

        if (s_isReadingProperty) {
            if (!s_styleWarningsIssued) {
                s_styleWarningsIssued = true;
                qmlWarning(styleReader) << "The style property '" << property << "' was read "
                                        << "before finishing the read of another style property. "
                                        << "This is likely to cause a style glitch.";
            }
        }
        QScopedValueRollback rollback(s_isReadingProperty, true);

        PropertyPathIds ids;
        ids.property = pathId(group, property, PathId::ExcludeSubType);
        ids.alternative = pathId(group, alternative, PathId::ExcludeSubType);

        if (subType != QQSK::PropertyGroup::NoGroup) {
            ids.subTypeProperty = pathId(group, property, PathId::IncludeSubType);
            ids.subTypeAlternative = pathId(group, alternative, PathId::IncludeSubType);
        } else {
            ids.subTypeProperty = PropertyPathId();
            ids.subTypeAlternative = PropertyPathId();
        }

        if (!pathFlags.testFlag(QQSK::PathFlag::StyleDirect)) {
            /* A style reader can have a storage that contains local property overrides (that is,
             * interpolated values from an ongoing transition). When searching for a property, we
             * therefore need to check this storage first. The exception is if the property was read
             * inside the 'global' group, which means that we should read the values directly
             * from the style. */
            if (subType != QQSK::PropertyGroup::NoGroup) {
                const QVariant value = readPropertyInStorageForState(
                    ids.subTypeProperty, ids.subTypeAlternative, styleReader, QQSK::StateFlag::Normal);
                if (value.isValid())
                    return value;
            }
            const QVariant value = readPropertyInStorageForState(
                ids.property, ids.alternative, styleReader, QQSK::StateFlag::Normal);
            if (value.isValid())
                return value;
        }

        style->setPalette(styleReader->palette());
        cacheReaderState(styleReader->controlState());
        const QVariant value = readProperty(ids, styleReader, style);
        return value;
    }

    Q_UNREACHABLE();
    return {};
}

bool QQStyleKitPropertyResolver::writeStyleProperty(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property,
    const QVariant &value)
{
    // While readStyleProperty() takes propagation into account, writeStyleProperty() doesn't.
    // Instead it writes \a value directly to the storage that the group belongs to.
    Q_ASSERT(group);
    auto [controlProperties, subType, pathFlags] = group->inspectGroupPath();
    const QQSK::Subclass subclass = controlProperties->subclass();
    const PropertyPathId propertyPathId = pathId(group, property, PathId::IncludeSubType);
    // The subType is only used when reading a property using propagation
    Q_UNUSED(subType);

    if (pathFlags.testFlag(QQSK::PathFlag::StyleDirect)) {
        qmlWarning(controlProperties) << "Properties inside 'global' are read-only!";
        return false;
    }

    if (subclass == QQSK::Subclass::QQStyleKitReader) {
        // This is a write to a StyleKitReader, probably from an ongoing transition
        QQStyleKitReader *reader = controlProperties->asQQStyleKitReader();
        const PropertyStorageId key = propertyPathId.storageId(QQSK::StateFlag::Normal);
        const QVariant currentValue = reader->readStyleProperty(key);
        const bool valueChanged = currentValue != value;
        if (valueChanged) {
            reader->writeStyleProperty(key, value);
            QQStyleKitDebug::notifyPropertyWrite(group, property, reader, QQSK::StateFlag::Normal, key, value);
        }
        return valueChanged;
    }

    if (subclass == QQSK::Subclass::QQStyleKitState) {
        // This is a write to a control inside the StyleKit style
        const auto [control, nestedState] = controlProperties->asQQStyleKitState()->controlAndState();
        const PropertyStorageId key = propertyPathId.storageId(nestedState);
        const QVariant currentValue = control->readStyleProperty(key);
        const bool valueChanged = currentValue != value;
        if (valueChanged) {
            control->m_writtenStates |= nestedState;
            control->writeStyleProperty(key, value);
            QQStyleKitDebug::notifyPropertyWrite(group, property, control, nestedState, key, value);
        }
        return valueChanged;
    }

    Q_UNREACHABLE();
    return false;
}

bool QQStyleKitPropertyResolver::hasLocalStyleProperty(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property)
{
    Q_ASSERT(group);
    auto [controlProperties, subType, pathFlags] = group->inspectGroupPath();
    const QQSK::Subclass subclass = controlProperties->subclass();
    const PropertyPathId propertyPathId = pathId(group, property, PathId::IncludeSubType);
    Q_UNUSED(subType);

    if (pathFlags.testFlag(QQSK::PathFlag::StyleDirect))
        return false;

    if (subclass == QQSK::Subclass::QQStyleKitReader) {
        const PropertyStorageId key = propertyPathId.storageId(QQSK::StateFlag::Normal);
        return controlProperties->asQQStyleKitReader()->readStyleProperty(key).isValid();
    }

    if (subclass == QQSK::Subclass::QQStyleKitState) {
        const auto [control, nestedState] = controlProperties->asQQStyleKitState()->controlAndState();
        const PropertyStorageId key = propertyPathId.storageId(nestedState);
        return control->readStyleProperty(key).isValid();
    }

    Q_UNREACHABLE();
    return false;
}

QT_END_NAMESPACE

#include "moc_qqstylekitpropertyresolver_p.cpp"
