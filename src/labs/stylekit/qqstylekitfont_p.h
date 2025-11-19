// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITFONT_H
#define QQSTYLEKITFONT_H

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
#include <QtGui/qfont.h>

QT_BEGIN_NAMESPACE

class QQStyleKitFont : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QFont systemFont READ systemFont WRITE setSystemFont NOTIFY systemFontChanged FINAL)
    Q_PROPERTY(QFont buttonFont READ buttonFont WRITE setButtonFont NOTIFY buttonFontChanged FINAL)
    Q_PROPERTY(QFont checkboxFont READ checkboxFont WRITE setCheckboxFont NOTIFY checkboxFontChanged FINAL)
    Q_PROPERTY(QFont comboBoxFont READ comboBoxFont WRITE setComboBoxFont NOTIFY comboBoxFontChanged FINAL)
    Q_PROPERTY(QFont groupBoxFont READ groupBoxFont WRITE setGroupBoxFont NOTIFY groupBoxFontChanged FINAL)
    Q_PROPERTY(QFont itemViewFont READ itemViewFont WRITE setItemViewFont NOTIFY itemViewFontChanged FINAL)
    Q_PROPERTY(QFont labelFont READ labelFont WRITE setLabelFont NOTIFY labelFontChanged FINAL)
    Q_PROPERTY(QFont listViewFont READ listViewFont WRITE setListViewFont NOTIFY listViewFontChanged FINAL)
    Q_PROPERTY(QFont menuFont READ menuFont WRITE setMenuFont NOTIFY menuFontChanged FINAL)
    Q_PROPERTY(QFont menuBarFont READ menuBarFont WRITE setMenuBarFont NOTIFY menuBarFontChanged FINAL)
    Q_PROPERTY(QFont radioButtonFont READ radioButtonFont WRITE setRadioButtonFont NOTIFY radioButtonFontChanged FINAL)
    Q_PROPERTY(QFont spinBoxFont READ spinBoxFont WRITE setSpinBoxFont NOTIFY spinBoxFontChanged FINAL)
    Q_PROPERTY(QFont switchControlFont READ switchControlFont WRITE setSwitchControlFont NOTIFY switchControlFontChanged FINAL)
    Q_PROPERTY(QFont tabBarFont READ tabBarFont WRITE setTabBarFont NOTIFY tabBarFontChanged FINAL)
    Q_PROPERTY(QFont textAreaFont READ textAreaFont WRITE setTextAreaFont NOTIFY textAreaFontChanged FINAL)
    Q_PROPERTY(QFont textFieldFont READ textFieldFont WRITE setTextFieldFont NOTIFY textFieldFontChanged FINAL)
    Q_PROPERTY(QFont toolBarFont READ toolBarFont WRITE setToolBarFont NOTIFY toolBarFontChanged FINAL)
    Q_PROPERTY(QFont toolTipFont READ toolTipFont WRITE setToolTipFont NOTIFY toolTipFontChanged FINAL)
    Q_PROPERTY(QFont tumblerFont READ tumblerFont WRITE setTumblerFont NOTIFY tumblerFontChanged FINAL)

    QML_NAMED_ELEMENT(StyleKitFont)

public:
    QQStyleKitFont(QObject *parent = nullptr);
    QFont systemFont() const;
    void setSystemFont(const QFont &font);

    QFont buttonFont() const;
    void setButtonFont(const QFont &font);

    QFont checkboxFont() const;
    void setCheckboxFont(const QFont &font);

    QFont comboBoxFont() const;
    void setComboBoxFont(const QFont &font);

    QFont groupBoxFont() const;
    void setGroupBoxFont(const QFont &font);

    QFont itemViewFont() const;
    void setItemViewFont(const QFont &font);

    QFont labelFont() const;
    void setLabelFont(const QFont &font);

    QFont listViewFont() const;
    void setListViewFont(const QFont &font);

    QFont menuFont() const;
    void setMenuFont(const QFont &font);

    QFont menuBarFont() const;
    void setMenuBarFont(const QFont &font);

    QFont radioButtonFont() const;
    void setRadioButtonFont(const QFont &font);

    QFont spinBoxFont() const;
    void setSpinBoxFont(const QFont &font);

    QFont switchControlFont() const;
    void setSwitchControlFont(const QFont &font);

    QFont tabBarFont() const;
    void setTabBarFont(const QFont &font);

    QFont textAreaFont() const;
    void setTextAreaFont(const QFont &font);

    QFont textFieldFont() const;
    void setTextFieldFont(const QFont &font);

    QFont toolBarFont() const;
    void setToolBarFont(const QFont &font);

    QFont toolTipFont() const;
    void setToolTipFont(const QFont &font);

    QFont tumblerFont() const;
    void setTumblerFont(const QFont &font);

signals:
    void systemFontChanged();
    void buttonFontChanged();
    void checkboxFontChanged();
    void comboBoxFontChanged();
    void groupBoxFontChanged();
    void itemViewFontChanged();
    void labelFontChanged();
    void listViewFontChanged();
    void menuFontChanged();
    void menuBarFontChanged();
    void radioButtonFontChanged();
    void spinBoxFontChanged();
    void switchControlFontChanged();
    void tabBarFontChanged();
    void textAreaFontChanged();
    void textFieldFontChanged();
    void toolBarFontChanged();
    void toolTipFontChanged();
    void tumblerFontChanged();

private:
    Q_DISABLE_COPY(QQStyleKitFont)

    mutable std::unique_ptr<QFont> m_systemFont;
    mutable std::unique_ptr<QFont> m_buttonFont;
    mutable std::unique_ptr<QFont> m_checkboxFont;
    mutable std::unique_ptr<QFont> m_comboBoxFont;
    mutable std::unique_ptr<QFont> m_groupBoxFont;
    mutable std::unique_ptr<QFont> m_itemViewFont;
    mutable std::unique_ptr<QFont> m_labelFont;
    mutable std::unique_ptr<QFont> m_listViewFont;
    mutable std::unique_ptr<QFont> m_menuFont;
    mutable std::unique_ptr<QFont> m_menuBarFont;
    mutable std::unique_ptr<QFont> m_radioButtonFont;
    mutable std::unique_ptr<QFont> m_spinBoxFont;
    mutable std::unique_ptr<QFont> m_switchControlFont;
    mutable std::unique_ptr<QFont> m_tabBarFont;
    mutable std::unique_ptr<QFont> m_textAreaFont;
    mutable std::unique_ptr<QFont> m_textFieldFont;
    mutable std::unique_ptr<QFont> m_toolBarFont;
    mutable std::unique_ptr<QFont> m_toolTipFont;
    mutable std::unique_ptr<QFont> m_tumblerFont;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITFONT_H
