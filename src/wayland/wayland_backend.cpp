#include "wayland/wayland_backend.hpp"
#include <QDebug>

namespace pacmangui {
namespace wayland {

bool WaylandBackend::initialize() {
    qDebug() << "WaylandBackend::initialize - Wayland backend initialized";
    return true;
}

bool WaylandBackend::isWaylandAvailable() {
    // Check if WAYLAND_DISPLAY environment variable is set
    return !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
}

QString WaylandBackend::getDisplayInfo() {
    if (!isWaylandAvailable()) {
        return QString();
    }
    
    QString displayName = qEnvironmentVariable("WAYLAND_DISPLAY");
    return QString("Wayland display: %1").arg(displayName);
}

void WaylandBackend::cleanup() {
    qDebug() << "WaylandBackend::cleanup - Wayland backend cleaned up";
}

} // namespace wayland
} // namespace pacmangui 