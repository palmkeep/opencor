/*******************************************************************************

Copyright The University of Auckland

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************/

//==============================================================================
// Python wrapper plugin
//==============================================================================

#pragma once

//==============================================================================

#include "plugininfo.h"
#include "plugininterface.h"

//==============================================================================

#include "PythonQt.h"

//==============================================================================

namespace OpenCOR {

//==============================================================================

namespace SimulationExperimentView {
    class SimulationExperimentViewWidget;
};

//==============================================================================

namespace PythonWrapper {

//==============================================================================

class PythonWrapperCore;
class PythonWrapperDataStore;
class PythonWrapperSimulationExperimentView;
class PythonWrapperSolver;

//==============================================================================

PLUGININFO_FUNC PythonWrapperPluginInfo();

//==============================================================================

class PythonWrapperPlugin : public QObject, public PluginInterface
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "OpenCOR.PythonWrapperPlugin" FILE "pythonwrapperplugin.json")

    Q_INTERFACES(OpenCOR::PluginInterface)

public:
    explicit PythonWrapperPlugin();

#include "plugininterface.inl"

    static PythonWrapperPlugin *instance();

    SimulationExperimentView::SimulationExperimentViewWidget *simulationExperimentViewWidget();

private:
    PythonQtObjectPtr mOpenCORModule;

    PythonWrapperCore *mPythonWrapperCore;
    PythonWrapperDataStore *mPythonWrapperDataStore;
    PythonWrapperSimulationExperimentView *mPythonWrapperSimulationExperimentView;
    PythonWrapperSolver *mPythonWrapperSolver;

    SimulationExperimentView::SimulationExperimentViewWidget *mSimulationExperimentViewWidget;
};

//==============================================================================

}   // namespace PythonWrapper
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
