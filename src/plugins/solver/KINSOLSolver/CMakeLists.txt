project(KINSOLSolverPlugin)

# Add the plugin

add_plugin(KINSOLSolver
    SOURCES
        ../../i18ninterface.cpp
        ../../plugininfo.cpp
        ../../solverinterface.cpp

        src/kinsolsolver.cpp
        src/kinsolsolverplugin.cpp
    HEADERS_MOC
        ../../solverinterface.h

        src/kinsolsolver.h
        src/kinsolsolverplugin.h
    PLUGINS
        SUNDIALS
    QT_MODULES
        Widgets
)
