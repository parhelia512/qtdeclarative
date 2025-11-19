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

const QList<int> QQStyleKitPropertyResolver::baseTypesForType(int exactType)
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
        static QList<int> t = { QQStyleKitReader::AbstractButton, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::Menu:
    case QQStyleKitReader::Dialog: {
        static QList<int> t = { QQStyleKitReader::Popup, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::Page:
    case QQStyleKitReader::Frame: {
        static QList<int> t = { QQStyleKitReader::Pane, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::TextField:
    case QQStyleKitReader::TextArea: {
        static QList<int> t = { QQStyleKitReader::TextInput, QQStyleKitReader::Control };
        return t; }
    default: {
        static QList<int> t = { QQStyleKitReader::Control };
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

void QQStyleKitPropertyResolver::rebuildParentChainForReader(QQStyleKitReader *styleReader)
{
    /* Build up the parent chain of readers from \a styleReader. If we encounter a
     * reader that already has m_parentChainDirty set to false, we can return early. This
     * means that the execution of this function will go faster and faster for each call. */
    styleReader->m_parentChainDirty = false;
    QQStyleKitReader *childReader = styleReader;
    QObject *parentObj = styleReader->parent();
    QQuickItem *parentItem = nullptr;

    while (parentObj) {
        // TODO: change code to look for attached prop instead of styleReader prop?
        parentItem = qobject_cast<QQuickItem *>(parentObj);
        if (parentItem) {
            const QVariant readerAsVariant = parentItem->property("styleReader");
            if (readerAsVariant.isValid()) {
                if (auto *reader = qvariant_cast<QQStyleKitReader *>(readerAsVariant)) {
                    /* Whether the reader has a parent reader or not (which we find out in
                     * the next iteration), we now anyway mark it as resolved. */
                    reader->m_parentChainDirty = false;
                    /* Some controls use several style readers (e.g RangeSlider), which are
                     * typically siblings of each other. So ensure we don't make a sibling
                     * reader the parent of another sibling reader. */
                    if (reader->parent() != childReader->parent())
                        childReader->m_parentReader = reader;
                    if (reader->m_parentReader)
                        break;

                    childReader = reader;
                }
            }
            parentItem = parentItem->parentItem();
        }
        parentObj = parentItem ? parentItem : parentObj->parent();
    }
}

void QQStyleKitPropertyResolver::rebuildVariationsForReader(
    QQStyleKitReader *styleReader, const QQStyleKitStyle *style)
{
    Q_ASSERT(styleReader->m_effectiveVariationsDirty);
    styleReader->m_effectiveVariationsDirty = false;

    if (styleReader->m_parentChainDirty)
        rebuildParentChainForReader(styleReader);

    // We make the id's static, since they shouldn't change depending on the styleReader
    static PropertyPathIds ids;
    if (ids.property.property() == QQSK::Property::NoProperty) {
        ids.property = pathId(styleReader, QQSK::Property::Variations, PathId::IncludeSubType);
        ids.alternative = pathId(styleReader, QQSK::Property::NoProperty, PathId::IncludeSubType);
        ids.subTypeProperty = PropertyPathId();
        ids.subTypeAlternative = PropertyPathId();
    }

    /* Go through all the parent readers of \a styleReader (if any), and for each one,
     * add the variations attached to them that has the potential to affect the
     * type/control represented by styleReader to m_effectiveVariations. The variations
     * that are closest to styleReader in the hierarchy will be added first and take
     * precendence over the ones added last. */
    styleReader->m_effectiveVariations.clear();
    QList<int> affectedTypes = baseTypesForType(styleReader->type());
    affectedTypes.prepend(styleReader->type());
    QQStyleKitReader *parentReader = styleReader->m_parentReader;
    QQStyleKitReader *lastParentWithVariations = nullptr;

    while (parentReader) {
        if (!parentReader->m_hierarchyHasVariations)
            break;

        /* Note: when we read the variations property from the style, it returns the varations
         * set in the most specific storage/state/control (because of propagation). And those
         * are the only ones that will take effect. This means that even if there are variations
         * in the fallback style for the requested type (control) that overrides some properties,
         * and the style/theme has variations that overrides something else, the variations in
         * the fallback style will, in that case, be ignored for that type. The properties _not
         * set_ in the effective variations will instead propagate back to be read from the
         * type in the theme or style, like all other properties. */
        const int parentType = parentReader->type();
        const QList<int> baseTypes = baseTypesForType(parentType);
        const QVariant variationVariant = readPropertyInStyle(
            ids, parentType, baseTypes, parentReader, style);

        if (variationVariant.isValid()) {
            auto variationsInStyle = *qvariant_cast<QList<QQStyleKitVariation *> *>(variationVariant);
            for (auto *variation : std::as_const(variationsInStyle)) {
                if (!variation) {
                    // Ignore unsupported elements (text, numbers, etc), added to the array from QML
                    continue;
                }

                lastParentWithVariations = parentReader;

                for (int type : affectedTypes) {
                    if (variation->getControl(type)) {
                        styleReader->m_effectiveVariations.append(variation);
                        // qDebug() << "Found variation" << variation
                        //          << "for" << styleReader->typeAsControlType()
                        //          << "in" << parentReader->typeAsControlType();
                        break;
                    }
                }
            }
        }

        parentReader = parentReader->m_parentReader;
    }

    if (lastParentWithVariations) {
        /* This function will be called for all style readers / controls after a style or theme
         * change. So as an optimization, we do an extra pass in the end where we mark all the
         * readers we found above from the parent of lastParentWithVariations and up to the root
         * as not having variations. This allows us to return early for all the remaining style
         * readers that shares the same, or parts of the same, parent chain. */
        QQStyleKitReader *parentReader = lastParentWithVariations->m_parentReader;
        while (parentReader) {
            if (!parentReader->m_hierarchyHasVariations)
                break;
            parentReader->m_hierarchyHasVariations = false;
            parentReader = parentReader->m_parentReader;
        }
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
    const int exactType, const QList<int> baseTypes)
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
    const PropertyPathIds &ids, const int exactType, const QList<int> baseTypes,
    const QQStyleKitReader *styleReader, const QQStyleKitStyle *style)
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
            fallbackStyle->setPalette(styleReader->palette());
            value = readPropertyInStyle(ids, exactType, baseTypes, styleReader, fallbackStyle);
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
    const PropertyPathIds &ids, QQStyleKitReader *styleReader, const QQStyleKitStyle *style)
{
    if (styleReader->m_effectiveVariationsDirty)
        rebuildVariationsForReader(styleReader, style);

    const int exactType = styleReader->type();
    const QList<int> baseTypes = baseTypesForType(exactType);

    QVariant value;

    while (true) {
        for (const auto *variation : styleReader->m_effectiveVariations) {
            value = readPropertyInRelevantControls(variation, ids, exactType, baseTypes);
            if (value.isValid())
                break;
        }
        if (value.isValid())
            break;

        value = readPropertyInStyle(ids, exactType, baseTypes, styleReader, style);
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
