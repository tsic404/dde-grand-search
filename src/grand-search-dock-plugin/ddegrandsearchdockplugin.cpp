// SPDX-FileCopyrightText: 2021 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddegrandsearchdockplugin.h"
#include "gui/grandsearchwidget.h"
#include "gui/tipswidget.h"

#include <DApplication>

#include <QGSettings>
#include <QLabel>

#define GrandSearchPlugin "grand-search"
#define MenuOpenSetting "menu_open_setting"
#define GrandSearchApp "dde-grand-search"

#define SchemaId "com.deepin.dde.dock.module.grand-search"
#define SchemaPath "/com/deepin/dde/dock/module/grand-search/"
#define SchemaKeyMenuEnable "menuEnable"

using namespace GrandSearch;
DWIDGET_USE_NAMESPACE

DdeGrandSearchDockPlugin::DdeGrandSearchDockPlugin(QObject *parent)
    : QObject(parent)
    , m_searchWidget(nullptr)
{
}

DdeGrandSearchDockPlugin::~DdeGrandSearchDockPlugin()
{
}

const QString DdeGrandSearchDockPlugin::pluginName() const
{
    return GrandSearchPlugin;
}

const QString DdeGrandSearchDockPlugin::pluginDisplayName() const
{
    return tr("Grand Search");
}

void DdeGrandSearchDockPlugin::init(PluginProxyInterface *proxyInter)
{
    // 加载翻译
    QString appName = qApp->applicationName();
    qApp->setApplicationName(GrandSearchApp);
    qApp->loadTranslator();
    qApp->setApplicationName(appName);

    m_proxyInter = proxyInter;

    if (m_searchWidget.isNull())
        m_searchWidget.reset(new GrandSearchWidget);

    if (m_tipsWidget.isNull())
        m_tipsWidget.reset(new TipsWidget);

    // 如果插件没有被禁用则在初始化插件时才添加主控件到面板上
    if (!pluginIsDisable()) {
        m_proxyInter->itemAdded(this, pluginName());
    }

    if (QGSettings::isSchemaInstalled(SchemaId)) {
        m_gsettings.reset(new QGSettings(SchemaId, SchemaPath));
        connect(m_gsettings.data(), &QGSettings::changed, this, &DdeGrandSearchDockPlugin::onGsettingsChanged);
    } else {
        qWarning() << "no such schema id" << SchemaId;
    }
}

QWidget *DdeGrandSearchDockPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_searchWidget.data();
}

QWidget *DdeGrandSearchDockPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    // 设置/刷新 tips 中的信息
    m_tipsWidget->setText(tr("Grand Search"));
    return m_tipsWidget.data();
}

bool DdeGrandSearchDockPlugin::pluginIsAllowDisable()
{
    // 插件允许禁用
    return true;
}

bool DdeGrandSearchDockPlugin::pluginIsDisable()
{
    // 第二个参数 “disabled” 表示存储这个值的键（所有配置都是以键值对的方式存储）
    // 第三个参数表示默认值，即默认不禁用
    return m_proxyInter->getValue(this, "disabled", false).toBool();
}

void DdeGrandSearchDockPlugin::pluginStateSwitched()
{
    // 获取当前禁用状态的反值作为新的状态值
    const bool disabledNew = !pluginIsDisable();
    // 存储新的状态值
    m_proxyInter->saveValue(this, "disabled", disabledNew);

    // 根据新的禁用状态值处理主控件的加载和卸载
    if (disabledNew) {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        m_proxyInter->itemAdded(this, pluginName());
    }
}

const QString DdeGrandSearchDockPlugin::itemCommand(const QString &itemKey)
{
    if (GrandSearchPlugin == itemKey)
        return m_searchWidget->itemCommand(itemKey);

    return QString();
}

int DdeGrandSearchDockPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, 0).toInt();
}

void DdeGrandSearchDockPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

const QString DdeGrandSearchDockPlugin::itemContextMenu(const QString &itemKey)
{
    if (itemKey != GrandSearchPlugin) {
        return QString();
    }

    QList<QVariant> items;
    items.reserve(1);

    QMap<QString, QVariant> item;
    item["itemId"] = MenuOpenSetting;
    item["itemText"] = tr("Search settings");
    item["isActive"] = true;
    items.push_back(item);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void DdeGrandSearchDockPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(checked)

    if (itemKey != GrandSearchPlugin)
        return;

    if (menuId == MenuOpenSetting) {
        QProcess::startDetached("dde-grand-search", QStringList() << "--setting");
    }
}

void DdeGrandSearchDockPlugin::onGsettingsChanged(const QString &key)
{
    Q_ASSERT(m_gsettings);

    qDebug() << "gsettings changed,and key:" << key << "    value:" << m_gsettings->get(key);
    if (key == SchemaKeyMenuEnable) {
        bool enable = m_gsettings->get(key).toBool();
        qInfo() << "The status of whether the grand search right-click menu is enabled changes to:" << enable;
    }
}
