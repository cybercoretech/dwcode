import QtQuick
QtObject {
    readonly property var data: workspaceBackend.theme
    readonly property color background: data.surfaces.window
    readonly property color canvas: data.surfaces.editor
    readonly property color shell: data.surfaces.shell
    readonly property color rail: data.surfaces.window
    readonly property color sidebar: data.surfaces.sidebar
    readonly property color bottomPanel: data.surfaces.editor
    readonly property color elevated: data.surfaces.raised
    readonly property color border: data.surfaces.border
    readonly property color cyan: data.accents.primary
    readonly property color magenta: data.accents.secondary
    readonly property color green: data.accents.success
    readonly property color foreground: data.text.primary
    readonly property color secondaryText: data.text.secondary
    readonly property color mutedText: data.text.muted
}
