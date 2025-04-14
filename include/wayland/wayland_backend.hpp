#ifndef PACMANGUI_WAYLAND_BACKEND_HPP
#define PACMANGUI_WAYLAND_BACKEND_HPP

#include <QString>

namespace pacmangui {
namespace wayland {

/**
 * @brief The WaylandBackend class provides integration with Wayland display server
 */
class WaylandBackend {
public:
    /**
     * @brief Initializes the Wayland backend
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize();
    
    /**
     * @brief Checks if Wayland is available on the system
     * @return true if Wayland is available, false otherwise
     */
    static bool isWaylandAvailable();
    
    /**
     * @brief Gets the current Wayland display information
     * @return A string containing display information or an empty string if not available
     */
    static QString getDisplayInfo();
    
    /**
     * @brief Cleans up Wayland resources
     */
    static void cleanup();
};

} // namespace wayland
} // namespace pacmangui

#endif // PACMANGUI_WAYLAND_BACKEND_HPP