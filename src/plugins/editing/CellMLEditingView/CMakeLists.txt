project(CellMLEditingViewPlugin)

# Add the plugin

add_plugin(CellMLEditingView
    SOURCES
        ../../filehandlinginterface.cpp
        ../../guiinterface.cpp
        ../../i18ninterface.cpp
        ../../plugin.cpp
        ../../plugininfo.cpp
        ../../plugininterface.cpp
        ../../pluginmanager.cpp

        src/cellmleditingviewinterface.cpp
        src/cellmleditingviewplugin.cpp
        src/cellmleditingviewwidget.cpp
    HEADERS_MOC
        ../../plugin.h
        ../../pluginmanager.h

        src/cellmleditingviewplugin.h
        src/cellmleditingviewwidget.h
    PLUGINS
        CellMLSupport
        EditingView
        MathMLViewerWidget
)
