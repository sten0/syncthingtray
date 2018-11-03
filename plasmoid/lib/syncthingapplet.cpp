#include "./syncthingapplet.h"
#include "./settingsdialog.h"

#include "../../connector/syncthingservice.h"
#include "../../connector/utils.h"

#include "../../widgets/misc/direrrorsdialog.h"
#include "../../widgets/misc/internalerrorsdialog.h"
#include "../../widgets/misc/otherdialogs.h"
#include "../../widgets/misc/textviewdialog.h"
#include "../../widgets/settings/settings.h"
#include "../../widgets/settings/settingsdialog.h"
#include "../../widgets/webview/webviewdialog.h"

#include "../../model/syncthingicons.h"

#include "../../connector/utils.h"

#include "resources/config.h"

#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/resources/resources.h>

#include <c++utilities/application/argumentparser.h>

#include <KConfigGroup>

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QNetworkReply>
#include <QQmlEngine>
#include <QStringBuilder>

#include <iostream>

using namespace std;
using namespace Data;
using namespace Plasma;
using namespace Dialogs;
using namespace QtGui;
using namespace ChronoUtilities;

namespace Plasmoid {

SyncthingApplet::SyncthingApplet(QObject *parent, const QVariantList &data)
    : Applet(parent, data)
    , m_aboutDlg(nullptr)
    , m_connection()
    , m_notifier(m_connection)
    , m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_downloadModel(m_connection)
    , m_settingsDlg(nullptr)
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    , m_webViewDlg(nullptr)
#endif
    , m_currentConnectionConfig(-1)
    , m_initialized(false)
{
    m_notifier.setService(&m_service);
    qmlRegisterUncreatableMetaObject(Data::staticMetaObject, "martchus.syncthingplasmoid", 0, 6, "Data", QStringLiteral("only enums"));
}

SyncthingApplet::~SyncthingApplet()
{
    delete m_settingsDlg;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    delete m_webViewDlg;
#endif
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    SyncthingService::setMainInstance(nullptr);
#endif
}

void SyncthingApplet::init()
{
    LOAD_QT_TRANSLATIONS;

    Applet::init();

    // connect signals and slots
    connect(&m_notifier, &SyncthingNotifier::statusChanged, this, &SyncthingApplet::handleConnectionStatusChanged);
    connect(&m_notifier, &SyncthingNotifier::syncComplete, &m_dbusNotifier, &DBusStatusNotifier::showSyncComplete);
    connect(&m_notifier, &SyncthingNotifier::disconnected, &m_dbusNotifier, &DBusStatusNotifier::showDisconnect);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &SyncthingApplet::handleDevicesChanged);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &SyncthingApplet::handleDevicesChanged);
    connect(&m_connection, &SyncthingConnection::error, this, &SyncthingApplet::handleInternalError);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &SyncthingApplet::trafficChanged);
    connect(&m_connection, &SyncthingConnection::newNotification, this, &SyncthingApplet::handleNewNotification);
    connect(&m_notifier, &SyncthingNotifier::newDevice, &m_dbusNotifier, &DBusStatusNotifier::showNewDev);
    connect(&m_notifier, &SyncthingNotifier::newDir, &m_dbusNotifier, &DBusStatusNotifier::showNewDir);
    connect(&m_dbusNotifier, &DBusStatusNotifier::connectRequested, &m_connection,
        static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    connect(&m_dbusNotifier, &DBusStatusNotifier::dismissNotificationsRequested, this, &SyncthingApplet::dismissNotifications);
    connect(&m_dbusNotifier, &DBusStatusNotifier::showNotificationsRequested, this, &SyncthingApplet::showNotificationsDialog);
    connect(&m_dbusNotifier, &DBusStatusNotifier::errorDetailsRequested, this, &SyncthingApplet::showInternalErrorsDialog);
    connect(&m_dbusNotifier, &DBusStatusNotifier::webUiRequested, this, &SyncthingApplet::showWebUI);

    // restore settings
    Settings::restore();

    // initialize systemd service support
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    SyncthingService::setMainInstance(&m_service);
    m_service.setUnitName(Settings::values().systemd.syncthingUnit);
    connect(&m_service, &SyncthingService::systemdAvailableChanged, this, &SyncthingApplet::handleSystemdStatusChanged);
    connect(&m_service, &SyncthingService::stateChanged, this, &SyncthingApplet::handleSystemdStatusChanged);
    connect(&m_service, &SyncthingService::errorOccurred, this, &SyncthingApplet::handleSystemdServiceError);
#endif

    // load primary connection config
    m_currentConnectionConfig = config().readEntry<int>("selectedConfig", 0);

    // apply settings and connect according to settings
    handleSettingsChanged();

    m_initialized = true;
}

QIcon SyncthingApplet::statusIcon() const
{
    return m_statusInfo.statusIcon();
}

QString SyncthingApplet::incomingTraffic() const
{
    return trafficString(m_connection.totalIncomingTraffic(), m_connection.totalIncomingRate());
}

QString SyncthingApplet::outgoingTraffic() const
{
    return trafficString(m_connection.totalOutgoingTraffic(), m_connection.totalOutgoingRate());
}

QStringList SyncthingApplet::connectionConfigNames() const
{
    const auto &settings = Settings::values().connection;
    QStringList names;
    names.reserve(static_cast<int>(settings.secondary.size() + 1));
    names << settings.primary.label;
    for (const auto &setting : settings.secondary) {
        names << setting.label;
    }
    return names;
}

QString SyncthingApplet::currentConnectionConfigName() const
{
    const auto &settings = Settings::values().connection;
    if (m_currentConnectionConfig == 0) {
        return settings.primary.label;
    } else if (m_currentConnectionConfig > 0 && static_cast<unsigned>(m_currentConnectionConfig) <= settings.secondary.size()) {
        return settings.secondary[static_cast<unsigned>(m_currentConnectionConfig) - 1].label;
    }
    return QString();
}

Data::SyncthingConnectionSettings *SyncthingApplet::connectionConfig(int index)
{
    auto &connectionSettings = Settings::values().connection;
    if (index >= 0 && static_cast<unsigned>(index) <= connectionSettings.secondary.size()) {
        return index == 0 ? &connectionSettings.primary : &connectionSettings.secondary[static_cast<unsigned>(index) - 1];
    }
    return nullptr;
}

void SyncthingApplet::setCurrentConnectionConfigIndex(int index)
{
    auto &settings = Settings::values();
    bool reconnectRequired = false;
    if (index != m_currentConnectionConfig && index >= 0 && static_cast<unsigned>(index) <= settings.connection.secondary.size()) {
        auto &selectedConfig = index == 0 ? settings.connection.primary : settings.connection.secondary[static_cast<unsigned>(index) - 1];
        reconnectRequired = m_connection.applySettings(selectedConfig);
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
        if (m_webViewDlg) {
            m_webViewDlg->applySettings(selectedConfig, false);
        }
#endif
        config().writeEntry<int>("selectedConfig", index);
        emit currentConnectionConfigIndexChanged(m_currentConnectionConfig = index);
        emit localChanged();
    }

    // apply systemd settings, reconnect if required and possible
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    settings.systemd.apply(m_connection, currentConnectionConfig(), reconnectRequired);
#else
    if (reconnectRequired || !m_connection.isConnected()) {
        m_connection.reconnect();
    }
#endif
}

bool SyncthingApplet::isStartStopEnabled() const
{
    return Settings::values().systemd.showButton;
}

bool SyncthingApplet::areNotificationsAvailable() const
{
    return !m_notifications.empty();
}

void SyncthingApplet::setPassiveStates(const QList<Models::ChecklistItem> &passiveStates)
{
    m_passiveSelectionModel.setItems(passiveStates);
    const auto currentState = static_cast<int>(m_connection.status());
    setPassive(currentState >= 0 && currentState < passiveStates.size() && passiveStates.at(currentState).isChecked());
}

void SyncthingApplet::updateStatusIconAndTooltip()
{
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
    emit connectionStatusChanged();
}

void SyncthingApplet::showSettingsDlg()
{
    if (!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(*this);
        // ensure settings take effect when applied
        connect(m_settingsDlg, &Dialogs::SettingsDialog::applied, this, &SyncthingApplet::handleSettingsChanged);
        // save plasmoid specific settings to disk when applied
        connect(m_settingsDlg, &Dialogs::SettingsDialog::applied, this, &SyncthingApplet::configChanged);
        // save global/general settings to disk when applied
        connect(m_settingsDlg, &Dialogs::SettingsDialog::applied, &Settings::save);
    }
    Dialogs::centerWidget(m_settingsDlg);
    m_settingsDlg->show();
    m_settingsDlg->activateWindow();
}

void SyncthingApplet::showWebUI()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    if (Settings::values().webView.disabled) {
#endif
        QDesktopServices::openUrl(m_connection.syncthingUrl());
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    } else {
        if (!m_webViewDlg) {
            m_webViewDlg = new WebViewDialog;
            if (const auto *connectionConfig = currentConnectionConfig()) {
                m_webViewDlg->applySettings(*connectionConfig, true);
            }
            connect(m_webViewDlg, &WebViewDialog::destroyed, this, &SyncthingApplet::handleWebViewDeleted);
        }
        m_webViewDlg->show();
        m_webViewDlg->activateWindow();
    }
#endif
}

void SyncthingApplet::showLog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::showOwnDeviceId()
{
    auto *const dlg = ownDeviceIdDialog(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::showAboutDialog()
{
    if (!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(nullptr, QStringLiteral(APP_NAME), QStringLiteral(APP_AUTHOR "\nSyncthing icons from Syncthing project"),
            QStringLiteral(APP_VERSION), ApplicationUtilities::dependencyVersions2, QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION),
            QImage(statusIcons().scanninig.pixmap(128).toImage()));
        m_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        m_aboutDlg->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
        m_aboutDlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_aboutDlg, &QObject::destroyed, this, &SyncthingApplet::handleAboutDialogDeleted);
    }
    centerWidget(m_aboutDlg);
    m_aboutDlg->show();
    m_aboutDlg->activateWindow();
}

void SyncthingApplet::showNotificationsDialog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_notifications, tr("New notifications"));
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
    dismissNotifications();
}

void SyncthingApplet::dismissNotifications()
{
    m_connection.considerAllNotificationsRead();
    if (!m_notifications.empty()) {
        m_notifications.clear();
        emit notificationsAvailableChanged(false);
        // update status as well because having or not having notifications is relevant for status text/icon
        updateStatusIconAndTooltip();
    }
}

void SyncthingApplet::showInternalErrorsDialog()
{
    auto *const errorViewDlg = InternalErrorsDialog::instance();
    connect(errorViewDlg, &InternalErrorsDialog::errorsCleared, this, &SyncthingApplet::handleErrorsCleared);
    centerWidget(errorViewDlg);
    errorViewDlg->show();
}

void SyncthingApplet::showDirectoryErrors(unsigned int directoryIndex)
{
    const auto &dirs = m_connection.dirInfo();
    if (directoryIndex >= dirs.size()) {
        return;
    }

    const auto &dir(dirs[directoryIndex]);
    m_connection.requestDirPullErrors(dir.id);

    auto *const dlg = new DirectoryErrorsDialog(m_connection, dir);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

/*!
 * \brief Ensures settings take effect when applied via the settings dialog.
 * \remarks Does not save the settings to disk. This is done in Settings::save() and Applet::configChanged().
 */
void SyncthingApplet::handleSettingsChanged()
{
    const KConfigGroup config(this->config());
    const auto &settings(Settings::values());

    // apply notifiction settings
    settings.apply(m_notifier);

    // apply appearance settings
    setSize(config.readEntry<QSize>("size", QSize(25, 25)));
    const bool brightColors = config.readEntry<bool>("brightColors", false);
    m_dirModel.setBrightColors(brightColors);
    m_devModel.setBrightColors(brightColors);
    m_downloadModel.setBrightColors(brightColors);

    // restore selected states
    // note: The settings dialog writes this to the Plasmoid's config like the other settings. However, it
    //       is simpler and more efficient to assign the states directly. Of course this is only possible if
    //       the dialog has already been shown.
    if (m_settingsDlg) {
        setPassiveStates(m_settingsDlg->appearanceOptionPage()->passiveStatusSelection()->items());
    } else {
        m_passiveSelectionModel.applyVariantList(config.readEntry("passiveStates", QVariantList()));
    }

    // apply connection config
    const int currentConfig = m_currentConnectionConfig;
    m_currentConnectionConfig = -1; // force update
    setCurrentConnectionConfigIndex(currentConfig);

    emit settingsChanged();
}

void SyncthingApplet::handleConnectionStatusChanged(Data::SyncthingStatus previousStatus, Data::SyncthingStatus newStatus)
{
    Q_UNUSED(previousStatus)
    if (!m_initialized) {
        return;
    }

    // update whether passive
    setPassive(static_cast<int>(newStatus) < passiveStates().size() && passiveStates().at(static_cast<int>(newStatus)).isChecked());

    // update status icon and tooltip text
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
    emit connectionStatusChanged();
}

void SyncthingApplet::handleDevicesChanged()
{
    m_statusInfo.updateConnectedDevices(m_connection);
    emit connectionStatusChanged();
}

void SyncthingApplet::handleInternalError(
    const QString &errorMsg, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(m_connection, category, networkError)) {
        return;
    }
    InternalError error(errorMsg, request.url(), response);
    m_dbusNotifier.showInternalError(error);
    InternalErrorsDialog::addError(move(error));
}

void SyncthingApplet::handleErrorsCleared()
{
}

void SyncthingApplet::handleAboutDialogDeleted()
{
    m_aboutDlg = nullptr;
}

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
void SyncthingApplet::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}
#endif

void SyncthingApplet::handleNewNotification(DateTime when, const QString &msg)
{
    m_notifications.emplace_back(QString::fromLocal8Bit(when.toString(DateTimeOutputFormat::DateAndTime, true).data()), msg);
    if (Settings::values().notifyOn.syncthingErrors) {
        m_dbusNotifier.showSyncthingNotification(when, msg);
    }
    if (m_notifications.size() == 1) {
        emit notificationsAvailableChanged(true);
        // update status as well because having or not having notifications is relevant for status text/icon
        updateStatusIconAndTooltip();
    }
}

void SyncthingApplet::handleSystemdServiceError(const QString &context, const QString &name, const QString &message)
{
    handleInternalError(tr("D-Bus error - unable to ") % context % QChar('\n') % name % QChar(':') % message, SyncthingErrorCategory::SpecificRequest,
        QNetworkReply::NoError, QNetworkRequest(), QByteArray());
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
void SyncthingApplet::handleSystemdStatusChanged()
{
    Settings::values().systemd.apply(m_connection, currentConnectionConfig());
}
#endif

} // namespace Plasmoid

K_EXPORT_PLASMA_APPLET_WITH_JSON(syncthing, Plasmoid::SyncthingApplet, "metadata.json")

#include "syncthingapplet.moc"
