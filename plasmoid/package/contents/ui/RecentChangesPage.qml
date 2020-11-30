import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import QtQuick.Controls 2.15 as QQC2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    property alias view: recentChangesView
    anchors.fill: parent
    objectName: "RecentChangesPage"

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        TopLevelView {
            id: recentChangesView
            width: parent.width
            model: plasmoid.nativeInterface.recentChangesModel
            delegate: TopLevelItem {
                width: recentChangesView.width
                ColumnLayout {
                    width: parent.width
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: actionIcon
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            text: extendedAction
                        }
                        Item {
                            width: units.smallSpacing
                        }
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: "change-date-symbolic"
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: eventTime
                        }
                        Item {
                            width: units.smallSpacing
                        }
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: "network-server-symbolic"
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: modifiedBy
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: itemType === "file" ? "view-refresh-symbolic" : "folder-sync"
                        }
                        PlasmaComponents3.Label {
                            text: directoryId + ": "
                            font.weight: Font.DemiBold
                        }
                        PlasmaComponents3.Label {
                            Layout.fillWidth: true
                            text: path
                            elide: Text.ElideRight
                        }
                    }
                }

                function copyPath() {
                    plasmoid.nativeInterface.copyToClipboard(path)
                }
                function copyDeviceId() {
                    plasmoid.nativeInterface.copyToClipboard(modifiedBy)
                }
            }

            QQC2.Menu {
                id: contextMenu
                QQC2.MenuItem {
                    text: qsTr("Copy path")
                    icon.name: "edit-copy"
                    onTriggered: recentChangesView.currentItem.copyPath()
                }
                QQC2.MenuItem {
                    text: qsTr("Copy device ID")
                    icon.name: "network-server-symbolic"
                    onTriggered: recentChangesView.currentItem.copyDeviceId()
                }
            }
        }
    }
}
