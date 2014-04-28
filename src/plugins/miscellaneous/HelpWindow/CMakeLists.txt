PROJECT(HelpWindowPlugin)

# Generate the help files which will be embedded in the executable as a resource

EXECUTE_PROCESS(COMMAND ${QT_BINARY_DIR}/qcollectiongenerator ${CMAKE_SOURCE_DIR}/doc/${CMAKE_PROJECT_NAME}.qhcp
                                                           -o ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.qhc)

# Add the plugin

ADD_PLUGIN(HelpWindow
    SOURCES
        ../../guiinterface.cpp
        ../../i18ninterface.cpp
        ../../plugininfo.cpp

        src/helpwindowplugin.cpp
        src/helpwindowwidget.cpp
        src/helpwindowwindow.cpp
    HEADERS_MOC
        src/helpwindowplugin.h
        src/helpwindowwidget.h
        src/helpwindowwindow.h
    UIS
        src/helpwindowwindow.ui
    INCLUDE_DIRS
        src
    PLUGINS
        Core
    QT_MODULES
        Help
        Network
        WebKitWidgets
        XmlPatterns
    QT_LIBRARIES
        QtCore
        QtGui
        QtHelp
        QtMultimedia
        QtMultimediaWidgets
        QtNetwork
        QtOpenGL
        QtPositioning
        QtPrintSupport
        QtQml
        QtQuick
        QtSensors
        QtWebKit
        QtWebKitWidgets
        QtWidgets
        QtXmlPatterns
)