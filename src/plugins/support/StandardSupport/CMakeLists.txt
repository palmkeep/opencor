project(StandardSupportPlugin)

# Add the plugin

add_plugin(StandardSupport
    SOURCES
        ../../plugininfo.cpp

        src/standardfile.cpp
        src/standardfilemanager.cpp
        src/standardsupportplugin.cpp
    HEADERS_MOC
        src/standardfile.h
        src/standardfilemanager.h
        src/standardsupportplugin.h
    PLUGINS
        Core
)
