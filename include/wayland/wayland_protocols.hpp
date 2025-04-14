#ifndef PACMANGUI_WAYLAND_PROTOCOLS_HPP
#define PACMANGUI_WAYLAND_PROTOCOLS_HPP

#include <QString>
#include <QStringList>

namespace pacmangui {
namespace wayland {

/**
 * @brief The WaylandProtocols class provides support for various Wayland protocols
 */
class WaylandProtocols {
public:
    /**
     * @brief Initializes the Wayland protocols
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize();
    
    /**
     * @brief Checks for supported protocols on the current Wayland compositor
     * @return A list of supported protocol names
     */
    static QStringList getSupportedProtocols();
    
    /**
     * @brief Checks if a specific protocol is supported
     * @param protocolName The name of the protocol to check
     * @return true if the protocol is supported, false otherwise
     */
    static bool isProtocolSupported(const QString& protocolName);
    
    /**
     * @brief Cleans up protocol resources
     */
    static void cleanup();
};

} // namespace wayland
} // namespace pacmangui

#endif // PACMANGUI_WAYLAND_PROTOCOLS_HPP