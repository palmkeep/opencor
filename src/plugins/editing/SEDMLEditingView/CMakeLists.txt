project(SEDMLEditingViewPlugin)

# Add the plugin

add_plugin(SEDMLEditingView
    SOURCES
        ../../filehandlinginterface.cpp
        ../../guiinterface.cpp
        ../../i18ninterface.cpp
        ../../plugin.cpp
        ../../plugininfo.cpp
        ../../plugininterface.cpp
        ../../pluginmanager.cpp

        src/sedmleditingviewinterface.cpp
        src/sedmleditingviewplugin.cpp
        src/sedmleditingviewwidget.cpp
    HEADERS_MOC
        ../../plugin.h
        ../../pluginmanager.h

        src/sedmleditingviewplugin.h
        src/sedmleditingviewwidget.h
    PLUGINS
        EditingView
        SEDMLSupport
)
