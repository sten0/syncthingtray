#ifndef SYNCTHINGAPPLET_H
#define SYNCTHINGAPPLET_H

#include <syncthingwidgets/misc/dbusstatusnotifier.h>
#include <syncthingwidgets/misc/statusinfo.h>
#include <syncthingwidgets/webview/webviewdefs.h>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingdownloadmodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>
#include <syncthingmodel/syncthingstatusselectionmodel.h>

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingnotifier.h>
#include <syncthingconnector/syncthingservice.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/models/checklistmodel.h>

// ignore Plasma deprecation warnings because I'm not sure how to fix them considering Plasma's own applets
// haven't been ported yet
#define PLASMA_NO_DEPRECATED_WARNINGS 1

#include <Plasma/Applet>
#include <Plasma/Theme>

#include <QPalette>
#include <QSize>

namespace Data {
struct SyncthingConnectionSettings;
class IconManager;
} // namespace Data

namespace QtGui {
class WebViewDialog;
}

namespace QtForkAwesome {
class QuickImageProvider;
}

namespace Plasmoid {

class SettingsDialog;

class SyncthingApplet : public Plasma::Applet {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection NOTIFY connectionChanged)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel NOTIFY dirModelChanged)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDirModel READ sortFilterDirModel NOTIFY dirModelChanged)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel NOTIFY devModelChanged)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDevModel READ sortFilterDevModel NOTIFY devModelChanged)
    Q_PROPERTY(Data::SyncthingDownloadModel *downloadModel READ downloadModel NOTIFY downloadModelChanged)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *recentChangesModel READ recentChangesModel NOTIFY recentChangesModelChanged)
    Q_PROPERTY(Data::SyncthingStatusSelectionModel *passiveSelectionModel READ passiveSelectionModel NOTIFY passiveSelectionModelChanged)
    Q_PROPERTY(Data::SyncthingService *service READ service NOTIFY serviceChanged)
    Q_PROPERTY(bool local READ isLocal NOTIFY localChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString additionalStatusText READ additionalStatusText NOTIFY connectionStatusChanged)
    Q_PROPERTY(QIcon statusIcon READ statusIcon NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString incomingTraffic READ incomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(bool hasIncomingTraffic READ hasIncomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(QString outgoingTraffic READ outgoingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(bool hasOutgoingTraffic READ hasOutgoingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(Data::SyncthingStatistics globalStatistics READ globalStatistics NOTIFY statisticsChanged)
    Q_PROPERTY(Data::SyncthingStatistics localStatistics READ localStatistics NOTIFY statisticsChanged)
    Q_PROPERTY(QStringList connectionConfigNames READ connectionConfigNames NOTIFY settingsChanged)
    Q_PROPERTY(QString currentConnectionConfigName READ currentConnectionConfigName NOTIFY currentConnectionConfigIndexChanged)
    Q_PROPERTY(int currentConnectionConfigIndex READ currentConnectionConfigIndex WRITE setCurrentConnectionConfigIndex NOTIFY
            currentConnectionConfigIndexChanged)
    Q_PROPERTY(bool startStopEnabled READ isStartStopEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool hasInternalErrors READ hasInternalErrors NOTIFY hasInternalErrorsChanged)
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(bool notificationsAvailable READ areNotificationsAvailable NOTIFY notificationsAvailableChanged)
    Q_PROPERTY(bool passive READ isPassive NOTIFY passiveChanged)
    Q_PROPERTY(QList<QtUtilities::ChecklistItem> passiveStates READ passiveStates WRITE setPassiveStates)

public:
    SyncthingApplet(QObject *parent, const QVariantList &data);
    ~SyncthingApplet() override;

public:
    Data::SyncthingConnection *connection() const;
    Data::SyncthingDirectoryModel *dirModel() const;
    Data::SyncthingSortFilterModel *sortFilterDirModel() const;
    Data::SyncthingDeviceModel *devModel() const;
    Data::SyncthingSortFilterModel *sortFilterDevModel() const;
    Data::SyncthingDownloadModel *downloadModel() const;
    Data::SyncthingRecentChangesModel *recentChangesModel() const;
    Data::SyncthingStatusSelectionModel *passiveSelectionModel() const;
    Data::SyncthingService *service() const;
    bool isLocal() const;
    QString statusText() const;
    QString additionalStatusText() const;
    QIcon statusIcon() const;
    QIcon syncthingIcon() const;
    QString incomingTraffic() const;
    bool hasIncomingTraffic() const;
    QString outgoingTraffic() const;
    bool hasOutgoingTraffic() const;
    Data::SyncthingStatistics globalStatistics() const;
    Data::SyncthingStatistics localStatistics() const;
    QStringList connectionConfigNames() const;
    QString currentConnectionConfigName() const;
    int currentConnectionConfigIndex() const;
    Data::SyncthingConnectionSettings *currentConnectionConfig();
    Data::SyncthingConnectionSettings *connectionConfig(int index);
    void setCurrentConnectionConfigIndex(int index);
    bool isStartStopEnabled() const;
    bool hasInternalErrors() const;
    QSize size() const;
    void setSize(const QSize &size);
    bool areNotificationsAvailable() const;
    bool isPassive() const;
    const QList<QtUtilities::ChecklistItem> &passiveStates() const;
    void setPassiveStates(const QList<QtUtilities::ChecklistItem> &passiveStates);

public Q_SLOTS:
    void init() override;
    void initEngine(QObject *object);
    void showSettingsDlg();
    void showWebUI();
    void showLog();
    void showOwnDeviceId();
    void showAboutDialog();
    void showNotificationsDialog();
    void dismissNotifications();
    void showInternalErrorsDialog();
    void showDirectoryErrors(unsigned int directoryIndex);
    void copyToClipboard(const QString &text);
    void updateStatusIconAndTooltip();
    QIcon loadForkAwesomeIcon(const QString &name, int size = 32) const;
    QString formatFileSize(quint64 fileSizeInByte) const;

Q_SIGNALS:
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void connectionChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void dirModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void devModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void downloadModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void recentChangesModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void passiveSelectionModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void serviceChanged();
    void localChanged();
    void connectionStatusChanged();
    void trafficChanged();
    void statisticsChanged();
    void settingsChanged();
    void currentConnectionConfigIndexChanged(int index);
    void hasInternalErrorsChanged(bool hasInternalErrors);
    void sizeChanged(const QSize &size);
    void notificationsAvailableChanged(bool notificationsAvailable);
    void passiveChanged(bool passive);

private Q_SLOTS:
    void handleSettingsChanged();
    void handleConnectionStatusChanged(Data::SyncthingStatus previousStatus, Data::SyncthingStatus newStatus);
    void handleDevicesChanged();
    void handleInternalError(
        const QString &errorMsg, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response);
    void handleDirStatisticsChanged();
    void handleErrorsCleared();
    void handleAboutDialogDeleted();
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    void handleWebViewDeleted();
#endif
    void handleNewNotification(CppUtilities::DateTime when, const QString &msg);
    void handleSystemdServiceError(const QString &context, const QString &name, const QString &message);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    void handleSystemdStatusChanged();
#endif
    void handleImageProviderDestroyed();
    void handleThemeChanged();
    void setPassive(bool passive);

private:
    Plasma::Theme m_theme;
    QPalette m_palette;
    Data::IconManager &m_iconManager;
    QtUtilities::AboutDialog *m_aboutDlg;
    Data::SyncthingConnection m_connection;
    Data::SyncthingOverallDirStatistics m_overallStats;
    Data::SyncthingNotifier m_notifier;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Data::SyncthingService m_service;
#endif
    QtGui::StatusInfo m_statusInfo;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingSortFilterModel m_sortFilterDirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingSortFilterModel m_sortFilterDevModel;
    Data::SyncthingDownloadModel m_downloadModel;
    Data::SyncthingRecentChangesModel m_recentChangesModel;
    Data::SyncthingStatusSelectionModel m_passiveSelectionModel;
    SettingsDialog *m_settingsDlg;
    QtGui::DBusStatusNotifier m_dbusNotifier;
    std::vector<Data::SyncthingLogEntry> m_notifications;
    QtForkAwesome::QuickImageProvider *m_imageProvider;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    QtGui::WebViewDialog *m_webViewDlg;
#endif
    int m_currentConnectionConfig;
    bool m_hasInternalErrors;
    bool m_initialized;
    QSize m_size;
};

inline Data::SyncthingConnection *SyncthingApplet::connection() const
{
    return const_cast<Data::SyncthingConnection *>(&m_connection);
}

inline Data::SyncthingDirectoryModel *SyncthingApplet::dirModel() const
{
    return const_cast<Data::SyncthingDirectoryModel *>(&m_dirModel);
}

inline Data::SyncthingSortFilterModel *SyncthingApplet::sortFilterDirModel() const
{
    return const_cast<Data::SyncthingSortFilterModel *>(&m_sortFilterDirModel);
}

inline Data::SyncthingDeviceModel *SyncthingApplet::devModel() const
{
    return const_cast<Data::SyncthingDeviceModel *>(&m_devModel);
}

inline Data::SyncthingSortFilterModel *SyncthingApplet::sortFilterDevModel() const
{
    return const_cast<Data::SyncthingSortFilterModel *>(&m_sortFilterDevModel);
}

inline Data::SyncthingDownloadModel *SyncthingApplet::downloadModel() const
{
    return const_cast<Data::SyncthingDownloadModel *>(&m_downloadModel);
}

inline Data::SyncthingRecentChangesModel *SyncthingApplet::recentChangesModel() const
{
    return const_cast<Data::SyncthingRecentChangesModel *>(&m_recentChangesModel);
}

inline Data::SyncthingStatusSelectionModel *SyncthingApplet::passiveSelectionModel() const
{
    return const_cast<Data::SyncthingStatusSelectionModel *>(&m_passiveSelectionModel);
}

inline Data::SyncthingService *SyncthingApplet::service() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    return const_cast<Data::SyncthingService *>(&m_service);
#else
    return nullptr;
#endif
}

inline QString SyncthingApplet::statusText() const
{
    return m_statusInfo.statusText();
}

inline QString SyncthingApplet::additionalStatusText() const
{
    return m_statusInfo.additionalStatusText();
}

inline bool SyncthingApplet::isLocal() const
{
    return m_connection.isLocal();
}

inline int SyncthingApplet::currentConnectionConfigIndex() const
{
    return m_currentConnectionConfig;
}

inline Data::SyncthingConnectionSettings *SyncthingApplet::currentConnectionConfig()
{
    return connectionConfig(m_currentConnectionConfig);
}

inline QSize SyncthingApplet::size() const
{
    return m_size;
}

inline void SyncthingApplet::setSize(const QSize &size)
{
    if (size != m_size) {
        emit sizeChanged(m_size = size);
    }
}

inline bool SyncthingApplet::isPassive() const
{
    return status() == Plasma::Types::PassiveStatus;
}

inline const QList<QtUtilities::ChecklistItem> &SyncthingApplet::passiveStates() const
{
    return m_passiveSelectionModel.items();
}

inline void SyncthingApplet::setPassive(bool passive)
{
    if (passive != isPassive()) {
        setStatus(passive ? Plasma::Types::PassiveStatus : Plasma::Types::ActiveStatus);
        emit passiveChanged(passive);
    }
}
} // namespace Plasmoid

#endif // SYNCTHINGAPPLET_H
