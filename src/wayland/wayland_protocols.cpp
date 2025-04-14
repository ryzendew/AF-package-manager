#include "wayland/wayland_protocols.hpp"
#include <QDebug>
#include <QProcess>

namespace pacmangui {
namespace wayland {

bool WaylandProtocols::initialize() {
    qDebug() << "WaylandProtocols::initialize - Wayland protocols initialized";
    return true;
}

QStringList WaylandProtocols::getSupportedProtocols() {
    QStringList protocols;
    protocols << "xdg-shell" << "xdg-decoration" << "idle-inhibit" << "pointer-constraints";
    return protocols;
}

bool WaylandProtocols::isProtocolSupported(const QString& protocolName) {
    return getSupportedProtocols().contains(protocolName);
}

void WaylandProtocols::cleanup() {
    qDebug() << "WaylandProtocols::cleanup - Wayland protocols cleaned up";
}

} // namespace wayland
} // namespace pacmangui 