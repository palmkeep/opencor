project(HeunSolverPlugin)

# Add the plugin

add_plugin(HeunSolver
    SOURCES
        ../../i18ninterface.cpp
        ../../plugininfo.cpp
        ../../solverinterface.cpp

        src/heunsolver.cpp
        src/heunsolverplugin.cpp
    HEADERS_MOC
        ../../solverinterface.h

        src/heunsolver.h
        src/heunsolverplugin.h
    QT_MODULES
        Widgets
)
