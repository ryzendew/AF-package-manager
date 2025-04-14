#ifndef PACMANGUI_WAYLAND_SECURITY_HPP
#define PACMANGUI_WAYLAND_SECURITY_HPP

#include <QString>

namespace pacmangui {
namespace wayland {

/**
 * @brief The WaylandSecurity class provides security features for Wayland integration
 */
class WaylandSecurity {
public:
    /**
     * @brief Initializes Wayland security features
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize();
    
    /**
     * @brief Enables or disables security features
     * @param enable True to enable security features, false to disable
     * @return true if the operation was successful, false otherwise
     */
    static bool enableSecurityFeatures(bool enable);
    
    /**
     * @brief Gets the current security status
     * @return A string describing the current security status
     */
    static QString getSecurityStatus();
    
    /**
     * @brief Cleans up security resources
     */
    static void cleanup();
};

} // namespace wayland
} // namespace pacmangui

#endif // PACMANGUI_WAYLAND_SECURITY_HPP