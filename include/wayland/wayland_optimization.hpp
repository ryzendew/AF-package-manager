#ifndef PACMANGUI_WAYLAND_OPTIMIZATION_HPP
#define PACMANGUI_WAYLAND_OPTIMIZATION_HPP

#include <QString>

namespace pacmangui {
namespace wayland {

/**
 * @brief The WaylandOptimization class provides performance optimizations for Wayland
 */
class WaylandOptimization {
public:
    /**
     * @brief Initializes Wayland optimizations
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize();
    
    /**
     * @brief Enables or disables various optimization features
     * @param enable True to enable optimizations, false to disable
     * @return true if the operation was successful, false otherwise
     */
    static bool enableOptimizations(bool enable);
    
    /**
     * @brief Gets the current optimization status
     * @return A string describing the current optimization status
     */
    static QString getOptimizationStatus();
    
    /**
     * @brief Cleans up optimization resources
     */
    static void cleanup();
};

} // namespace wayland
} // namespace pacmangui

#endif // PACMANGUI_WAYLAND_OPTIMIZATION_HPP