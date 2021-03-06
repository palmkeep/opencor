/*******************************************************************************

Copyright (C) The University of Auckland

OpenCOR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenCOR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

//==============================================================================
// Simulation
//==============================================================================

#include "cellmlfilemanager.h"
#include "cellmlfileruntime.h"
#include "combinefilemanager.h"
#include "filemanager.h"
#include "interfaces.h"
#include "sedmlfilemanager.h"
#include "simulation.h"
#include "simulationworker.h"

//==============================================================================

#include <QThread>

//==============================================================================

namespace OpenCOR {
namespace SimulationSupport {

//==============================================================================

SimulationObject::SimulationObject(Simulation *pSimulation) :
    mSimulation(pSimulation)
{
}

//==============================================================================

Simulation * SimulationObject::simulation() const
{
    // Return our simulation

    return mSimulation;
}

//==============================================================================

SimulationData::SimulationData(Simulation *pSimulation) :
    SimulationObject(pSimulation),
    mDelay(0),
    mStartingPoint(0.0),
    mEndingPoint(1000.0),
    mPointInterval(1.0),
    mOdeSolverName(QString()),
    mOdeSolverProperties(Solver::Solver::Properties()),
    mDaeSolverName(QString()),
    mDaeSolverProperties(Solver::Solver::Properties()),
    mNlaSolverName(QString()),
    mNlaSolverProperties(Solver::Solver::Properties()),
    mData(QMap<DataStore::DataStore *, double *>())
{
    // Create our various arrays

    createArrays();
}

//==============================================================================

SimulationData::~SimulationData()
{
    // Delete some internal objects

    deleteArrays();
}

//==============================================================================

void SimulationData::reload()
{
    // Reload ourselves by deleting and recreating our arrays

    deleteArrays();
    createArrays();
}

//==============================================================================

double * SimulationData::constants() const
{
    // Return our constants array

    return mConstants;
}

//==============================================================================

double * SimulationData::rates() const
{
    // Return our rates array

    return mRates;
}

//==============================================================================

double * SimulationData::states() const
{
    // Return our states array

    return mStates;
}

//==============================================================================

double * SimulationData::algebraic() const
{
    // Return our algebraic array

    return mAlgebraic;
}

//==============================================================================

double * SimulationData::data(DataStore::DataStore *pDataStore) const
{
    // Return our corresponding data array

    return mData.value(pDataStore);
}

//==============================================================================

void SimulationData::importData(DataStore::DataStoreImportData *pImportData)
{
    // Associate an array of doubles to the given data store

    mData.insert(pImportData->importDataStore(), pImportData->resultsValues());
}

//==============================================================================

const quint64 * SimulationData::delay() const
{
    // Return our delay

    return &mDelay;
}

//==============================================================================

void SimulationData::setDelay(quint64 pDelay)
{
    // Set our delay

    mDelay = pDelay;
}

//==============================================================================

double SimulationData::startingPoint() const
{
    // Return our starting point

    return mStartingPoint;
}

//==============================================================================

void SimulationData::setStartingPoint(double pStartingPoint, bool pRecompute)
{
    // Set our starting point

    mStartingPoint = pStartingPoint;

    // Recompute our 'computed constants' and 'variables', i.e. reset ourselves
    // witout initialisation (hence passing false to reset())

    if (pRecompute)
        reset(false);
}

//==============================================================================

double SimulationData::endingPoint() const
{
    // Return our ending point

    return mEndingPoint;
}

//==============================================================================

void SimulationData::setEndingPoint(double pEndingPoint)
{
    // Set our ending point

    mEndingPoint = pEndingPoint;
}

//==============================================================================

double SimulationData::pointInterval() const
{
    // Return our point interval

    return mPointInterval;
}

//==============================================================================

void SimulationData::setPointInterval(double pPointInterval)
{
    // Set our point interval

    mPointInterval = pPointInterval;
}

//==============================================================================

SolverInterface * SimulationData::solverInterface(const QString &pSolverName) const
{
    // Return the named solver interface, if any

    for (auto solverInterface : Core::solverInterfaces()) {
        if (!solverInterface->solverName().compare(pSolverName))
            return solverInterface;
    }

    return nullptr;
}

//==============================================================================

SolverInterface * SimulationData::odeSolverInterface() const
{
    // Return our ODE solver interface, if any

    return solverInterface(odeSolverName());
}

//==============================================================================

QString SimulationData::odeSolverName() const
{
    // Return our ODE solver name

    return mSimulation->runtime()?mOdeSolverName:QString();
}

//==============================================================================

void SimulationData::setOdeSolverName(const QString &pOdeSolverName)
{
    // Set our ODE solver name and reset its properties

    if (pOdeSolverName.compare(mOdeSolverName) && mSimulation->runtime()) {
        mOdeSolverName = pOdeSolverName;

        mOdeSolverProperties.clear();
    }
}

//==============================================================================

Solver::Solver::Properties SimulationData::odeSolverProperties() const
{
    // Return our ODE solver's properties

    return mSimulation->runtime()?mOdeSolverProperties:Solver::Solver::Properties();
}

//==============================================================================

void SimulationData::addOdeSolverProperty(const QString &pName,
                                          const QVariant &pValue)
{
    // Add an ODE solver property

    if (mSimulation->runtime())
        mOdeSolverProperties.insert(pName, pValue);
}

//==============================================================================

SolverInterface * SimulationData::nlaSolverInterface() const
{
    // Return our NLA solver interface, if any

    return solverInterface(nlaSolverName());
}

//==============================================================================

QString SimulationData::nlaSolverName() const
{
    // Return our NLA solver name

    return (   mSimulation->runtime()
            && mSimulation->runtime()->needNlaSolver())?mNlaSolverName:QString();
}

//==============================================================================

void SimulationData::setNlaSolverName(const QString &pNlaSolverName,
                                      bool pReset)
{
    // Set our NLA solver name and reset its properties

    if (   pNlaSolverName.compare(mNlaSolverName)
        && mSimulation->runtime() && mSimulation->runtime()->needNlaSolver()) {
        mNlaSolverName = pNlaSolverName;

        mNlaSolverProperties.clear();

        // Reset our parameter values, if required
        // Note: to only recompute our 'computed constants' and 'variables' is
        //       not sufficient since some constants may also need to be
        //       reinitialised...

        if (pReset)
            reset();
    }
}

//==============================================================================

Solver::Solver::Properties SimulationData::nlaSolverProperties() const
{
    // Return our NLA solver's properties

    return (   mSimulation->runtime()
            && mSimulation->runtime()->needNlaSolver())?mNlaSolverProperties:Solver::Solver::Properties();
}

//==============================================================================

void SimulationData::addNlaSolverProperty(const QString &pName,
                                          const QVariant &pValue, bool pReset)
{
    // Add an NLA solver property

    if (mSimulation->runtime() && mSimulation->runtime()->needNlaSolver()) {
        mNlaSolverProperties.insert(pName, pValue);

        // Reset our parameter values, if required
        // Note: to only recompute our 'computed constants' and 'variables' is
        //       not sufficient since some constants may also need to be
        //       reinitialised...

        if (pReset)
            reset();
    }
}

//==============================================================================

void SimulationData::reset(bool pInitialize, bool pAll)
{
    // Reset our parameter values which means both initialising our 'constants'
    // and computing our 'computed constants' and 'variables'
    // Note #1: we must check whether our runtime needs NLA solver and, if so,
    //          then retrieve an instance of our NLA solver since some of the
    //          resetting may require solving one or several NLA systems...
    // Note #2: recomputeComputedConstantsAndVariables() will let people know
    //          that our data has changed...

    CellMLSupport::CellmlFileRuntime *runtime = mSimulation->runtime();
    Solver::NlaSolver *nlaSolver = nullptr;

    if (runtime->needNlaSolver()) {
        // Set our NLA solver
        // Note: we unset it at the end of this method...

        nlaSolver = static_cast<Solver::NlaSolver *>(nlaSolverInterface()->solverInstance());

        Solver::setNlaSolver(runtime->address(), nlaSolver);

        // Keep track of any error that might be reported by our NLA solver

        connect(nlaSolver, &Solver::NlaSolver::error,
                this, &SimulationData::error);

        // Initialise our NLA solver

        nlaSolver->setProperties(mNlaSolverProperties);
    }

    // Keep track of our constants (in case we don't want to reset them)

    double *currentConstants = new double[runtime->constantsCount()] {};

    if (!pAll)
        memcpy(currentConstants, mConstants, size_t(runtime->constantsCount()*Solver::SizeOfDouble));

    // Reset our parameter values

    if (pInitialize) {
        memset(mConstants, 0, size_t(runtime->constantsCount()*Solver::SizeOfDouble));
        memset(mRates, 0, size_t(runtime->ratesCount()*Solver::SizeOfDouble));
        memset(mStates, 0, size_t(runtime->statesCount()*Solver::SizeOfDouble));
        memset(mAlgebraic, 0, size_t(runtime->algebraicCount()*Solver::SizeOfDouble));

        runtime->initializeConstants()(mConstants, mRates, mStates);
    }

    // Recompute our computed constants and variables

    recomputeComputedConstantsAndVariables(mStartingPoint, pInitialize);

    // Keep track of our various initial values

    if (pInitialize)
        updateInitialValues();

    // Use our "current" constants, if needed

    if (!pAll)
        memcpy(mConstants, currentConstants, size_t(runtime->constantsCount()*Solver::SizeOfDouble));

    delete[] currentConstants;

    // Recompute our computed constants and variables, if we are using our
    // "current" constants

    if (!pAll)
        recomputeComputedConstantsAndVariables(mStartingPoint, pInitialize);

    // Delete our NLA solver, if any

    if (nlaSolver) {
        delete nlaSolver;

        Solver::unsetNlaSolver(runtime->address());
    }

    // Let people know whether our data is clean, i.e. not modified, and ask our
    // simulation worker to reset itself

    if (!pInitialize) {
        emit modified(isModified());

        if (mSimulation->worker())
            mSimulation->worker()->reset();
    }
}

//==============================================================================

void SimulationData::updateInitialValues()
{
    // Update our initial constants and states

    memcpy(mInitialConstants, mConstants, size_t(mSimulation->runtime()->constantsCount()*Solver::SizeOfDouble));
    memcpy(mInitialStates, mStates, size_t(mSimulation->runtime()->statesCount()*Solver::SizeOfDouble));
}

//==============================================================================

void SimulationData::recomputeComputedConstantsAndVariables(double pCurrentPoint,
                                                            bool pInitialize)
{
    // Recompute our 'computed constants', some 'constant' algebraic variables
    // and our 'variables'

    CellMLSupport::CellmlFileRuntime *runtime = mSimulation->runtime();

    runtime->computeComputedConstants()(pCurrentPoint, mConstants, mRates, pInitialize?mStates:mDummyStates, mAlgebraic);
    runtime->computeRates()(pCurrentPoint, mConstants, mRates, mStates, mAlgebraic);
    runtime->computeVariables()(pCurrentPoint, mConstants, mRates, mStates, mAlgebraic);

    // Let people know that our data has been updated

    emit updated(pCurrentPoint);
}

//==============================================================================

void SimulationData::recomputeVariables(double pCurrentPoint)
{
    // Recompute our 'variables'

    mSimulation->runtime()->computeVariables()(pCurrentPoint, mConstants, mRates, mStates, mAlgebraic);
}

//==============================================================================

bool SimulationData::doIsModified(bool pCheckConstants) const
{
    // Check whether any of our constants (if requested) or states has been
    // modified, if possible
    // Note: we start with our states since they are more likely to be modified
    //       than our constants...

    CellMLSupport::CellmlFileRuntime *runtime = mSimulation->runtime();

    if (runtime) {
        for (int i = 0, iMax = runtime->statesCount(); i < iMax; ++i) {
            if (!qFuzzyCompare(mStates[i], mInitialStates[i]))
                return true;
        }

        if (pCheckConstants) {
            for (int i = 0, iMax = runtime->constantsCount(); i < iMax; ++i) {
                if (!qFuzzyCompare(mConstants[i], mInitialConstants[i]))
                    return true;
            }
        }
    }

    return false;
}

//==============================================================================

bool SimulationData::isStatesModified() const
{
    // Check whether any of our states has been modified

    return doIsModified(false);
}

//==============================================================================

bool SimulationData::isModified() const
{
    // Check whether any of our constants or states has been modified

    return doIsModified(true);
}

//==============================================================================

void SimulationData::checkForModifications()
{
    // Let people know whether any of our constants or states has been modified

    emit modified(isModified());
}

//==============================================================================

void SimulationData::createArrays()
{
    // Create our various arrays, if possible

    CellMLSupport::CellmlFileRuntime *runtime = mSimulation->runtime();

    if (runtime) {
        // Create our various arrays to compute our model

        mConstants = new double[runtime->constantsCount()] {};
        mRates = new double[runtime->ratesCount()] {};
        mStates = new double[runtime->statesCount()] {};
        mDummyStates = new double[runtime->statesCount()] {};
        mAlgebraic = new double[runtime->algebraicCount()] {};

        // Create our various arrays to keep track of our various initial values

        mInitialConstants = new double[runtime->constantsCount()] {};
        mInitialStates = new double[runtime->statesCount()] {};
    } else {
        mConstants = mRates = mStates = mDummyStates = mAlgebraic = nullptr;
        mInitialConstants = mInitialStates = nullptr;
    }
}

//==============================================================================

void SimulationData::deleteArrays()
{
    // Delete our various arrays

    for (auto data : mData.values())
        delete[] data;

    mData.clear();

    delete[] mConstants;
    delete[] mRates;
    delete[] mStates;
    delete[] mDummyStates;
    delete[] mAlgebraic;

    delete[] mInitialConstants;
    delete[] mInitialStates;
}

//==============================================================================

SimulationResults::SimulationResults(Simulation *pSimulation) :
    SimulationObject(pSimulation),
    mDataStore(nullptr),
    mPoints(nullptr),
    mConstants(DataStore::DataStoreVariables()),
    mRates(DataStore::DataStoreVariables()),
    mStates(DataStore::DataStoreVariables()),
    mAlgebraic(DataStore::DataStoreVariables()),
    mData(QMap<double *, DataStore::DataStoreVariables>()),
    mDataDataStores(QMap<double *, DataStore::DataStore *>())
{
    // Create our data store

    createDataStore();
}

//==============================================================================

SimulationResults::~SimulationResults()
{
    // Delete some internal objects

    deleteDataStore();
}

//==============================================================================

QString SimulationResults::uri(const QStringList &pComponentHierarchy,
                               const QString &pName)
{
    // Generate an URI using the given component hierarchy and name

    QString res = pComponentHierarchy.join('/')+"/"+pName;

    return res.replace('\'', "/prime");
}

//==============================================================================

void SimulationResults::createDataStore()
{
    // Make sure that we have a runtime and a VOI

    CellMLSupport::CellmlFileRuntime *runtime = mSimulation->runtime();

    if (!runtime || !runtime->voi())
        return;

    // Create our data store

    SimulationData *data = mSimulation->data();

    mDataStore = new DataStore::DataStore(mSimulation->cellmlFile()->xmlBase());

    mPoints = mDataStore->voi();

    mConstants = mDataStore->addVariables(data->constants(), runtime->constantsCount());
    mRates = mDataStore->addVariables(data->rates(), runtime->ratesCount());
    mStates = mDataStore->addVariables(data->states(), runtime->statesCount());
    mAlgebraic = mDataStore->addVariables(data->algebraic(), runtime->algebraicCount());

    // Customise our VOI, as well as our constant, rate, state and algebraic
    // variables

    for (auto parameter : runtime->parameters()) {
        DataStore::DataStoreVariable *variable = nullptr;

        switch (parameter->type()) {
        case CellMLSupport::CellmlFileRuntimeParameter::Voi:
            mPoints->setType(parameter->type());
            mPoints->setUri(uri(runtime->voi()->componentHierarchy(),
                                runtime->voi()->name()));
            mPoints->setLabel(runtime->voi()->name());
            mPoints->setUnit(runtime->voi()->unit());

            break;
        case CellMLSupport::CellmlFileRuntimeParameter::Constant:
        case CellMLSupport::CellmlFileRuntimeParameter::ComputedConstant:
            variable = mConstants[parameter->index()];

            break;
        case CellMLSupport::CellmlFileRuntimeParameter::Rate:
            variable = mRates[parameter->index()];

            break;
        case CellMLSupport::CellmlFileRuntimeParameter::State:
            variable = mStates[parameter->index()];

            break;
        case CellMLSupport::CellmlFileRuntimeParameter::Algebraic:
            variable = mAlgebraic[parameter->index()];

            break;
        default:
            // Not a relevant type, so do nothing

            ;
        }

        if (variable) {
            variable->setType(parameter->type());
            variable->setUri(uri(parameter->componentHierarchy(), parameter->formattedName()));
            variable->setLabel(parameter->formattedName());
            variable->setUnit(parameter->formattedUnit(runtime->voi()->unit()));
        }
    }

    // Reimport our data, if any, and update their array so that it contains the
    // computed values for our start point

    for (auto data : mDataDataStores.keys()) {
        DataStore::DataStore *importDataStore = mDataDataStores.value(data);
        DataStore::DataStoreVariables variables = mDataStore->addVariables(data, importDataStore->variables().count());

        mData.insert(data, variables);

        DataStore::DataStoreVariable *importVoi = importDataStore->voi();
        DataStore::DataStoreVariables importVariables = importDataStore->variables();

        for (int i = 0, iMax = importVariables.count(); i < iMax; ++i)
            data[i] = realValue(mSimulation->currentPoint(), importVoi, importVariables[i]);
    }

    // Let people know that our (imported) data, if any, has been updated

    if (mDataDataStores.count())
        emit data->updated(mSimulation->currentPoint());
}

//==============================================================================

void SimulationResults::deleteDataStore()
{
    // Delete our data store

    delete mDataStore;

    // Reset our data store and our different data store variable/s
    // Note: this is in case we are not able to recreate a data store...

    mDataStore = nullptr;

    mPoints = nullptr;

    mConstants = DataStore::DataStoreVariables();
    mRates = DataStore::DataStoreVariables();
    mStates = DataStore::DataStoreVariables();
    mAlgebraic = DataStore::DataStoreVariables();

    mData.clear();
}

//==============================================================================

void SimulationResults::reload()
{
    // Reload ourselves by resetting ourselves, this after having cleared all of
    // our imported data stores (since we don't keep track of imported data when
    // reloading a file)

    mDataDataStores.clear();

    reset();
}

//==============================================================================

void SimulationResults::reset()
{
    // Reset our data store by deleting it and then recreating it

    deleteDataStore();
    createDataStore();
}

//==============================================================================

void SimulationResults::importData(DataStore::DataStoreImportData *pImportData)
{
    // Make sure that we have a runtime and a VOI

    CellMLSupport::CellmlFileRuntime *runtime = mSimulation->runtime();

    if (!runtime || !runtime->voi())
        return;

    // Ask our data and results objects to import the given data

    DataStore::DataStore *importDataStore = pImportData->importDataStore();
    double *resultsValues = pImportData->resultsValues();
    DataStore::DataStoreVariables resultsVariables = pImportData->resultsVariables();

    mData.insert(resultsValues, resultsVariables);
    mDataDataStores.insert(resultsValues, importDataStore);

    // Customise our imported data

    for (auto parameter : runtime->dataParameters(resultsValues)) {
        DataStore::DataStoreVariable *variable = mData.value(parameter->data())[parameter->index()];

        variable->setType(parameter->type());
        variable->setUri(uri(parameter->componentHierarchy(), parameter->formattedName()));
        variable->setLabel(parameter->formattedName());
        variable->setUnit(parameter->formattedUnit(runtime->voi()->unit()));
    }

    // Compute the values of our imported data, so we can plot it straightaway
    // along our other simulation results, if any

    int runsCount = pImportData->runSizes().count();
    DataStore::DataStoreVariable *importVoi = importDataStore->voi();
    DataStore::DataStoreVariables importVariables = pImportData->importVariables();

    if (runsCount) {
        DataStore::DataStoreVariable *resultsVoi = pImportData->resultsDataStore()->voi();

        for (int i = 0; i < runsCount; ++i) {
            // Add the value of our imported data to our the corresponding run

            double *voiValues = resultsVoi->values(i);

            for (quint64 j = 0, jMax = resultsVoi->size(i); j < jMax; ++j) {
                double realPoint = SimulationResults::realPoint(voiValues[j], i);

                for (int k = 0, kMax = resultsVariables.count(); k < kMax; ++k)
                    resultsVariables[k]->addValue(realValue(realPoint, importVoi, importVariables[k]), i);
            }
        }

        // Update our imported data array so that it contains our latest
        // computed values

        quint64 lastPosition = size()-1;

        for (int i = 0, iMax = resultsVariables.count(); i < iMax; ++i)
            resultsValues[i] = resultsVariables[i]->value(lastPosition);
    } else {
        // There are no runs, so update our imported data array so that it
        // contains the computed values for our start point

        for (int i = 0, iMax = importVariables.count(); i < iMax; ++i)
            resultsValues[i] = realValue(mSimulation->currentPoint(), importVoi, importVariables[i]);
    }
}

//==============================================================================

int SimulationResults::runsCount() const
{
    // Return the number of runs held by our data store

    return mDataStore?mDataStore->runsCount():0;
}

//==============================================================================

bool SimulationResults::addRun()
{
    // Ask our data store to add a run to itself
    // Note: we consider things to be fine if our data store has had no problems
    //       adding a run to itself or if the simulation size is zero...

    quint64 simulationSize = mSimulation->size();

    if (simulationSize)
        return mDataStore->addRun(simulationSize);
    else
        return true;
}

//==============================================================================

double SimulationResults::realPoint(double pPoint, int pRun) const
{
    // Determine the real value of the given point, if we didn't have several
    // runs, but only one

    double res = 0.0;

    for (int i = 0, iMax = (pRun == -1)?runsCount()-1:pRun; i < iMax; ++i) {
        if (mDataStore->voi()->size(i))
            res += mDataStore->voi()->value(mDataStore->voi()->size(i)-1, i);
    }

    res += pPoint;

    return res;
}

//==============================================================================

double SimulationResults::realValue(double pPoint,
                                    DataStore::DataStoreVariable *pVoi,
                                    DataStore::DataStoreVariable *pVariable) const
{
    // Return the value of the given variable at the given point, doing a linear
    // interpolation, if needed

    quint64 first = 0;
    quint64 last = pVoi->size()-1;

    if (   (pPoint < pVoi->value(first))
        || (pPoint > pVoi->value(last))) {
        return qQNaN();
    } else {
        quint64 middle = 0;
        double middleVoiValue = 0.0;

        while (first <= last) {
            middle = (first+last) >> 1;
            middleVoiValue = pVoi->value(middle);

            if (middleVoiValue < pPoint)
                first = middle+1;
            else if (middleVoiValue > pPoint)
                last = middle-1;
            else
                break;
        }

        if (middleVoiValue < pPoint) {
            double afterVoiValue = pVoi->value(middle+1);
            double afterDataValue = pVariable->value(middle+1);

            return afterDataValue-(afterVoiValue-pPoint)*(afterDataValue-pVariable->value(middle))/(afterVoiValue-middleVoiValue);
        } else if (middleVoiValue > pPoint) {
            double beforeVoiValue = pVoi->value(middle-1);
            double beforeDataValue = pVariable->value(middle-1);

            return beforeDataValue+(pPoint-beforeVoiValue)*(pVariable->value(middle)-beforeDataValue)/(middleVoiValue-beforeVoiValue);
        } else {
            return pVariable->value(middle);
        }
    }
}

//==============================================================================

void SimulationResults::addPoint(double pPoint)
{
    // Make sure that all our variables are up to date

    mSimulation->data()->recomputeVariables(pPoint);

    // Make sure that we have the correct imported data values for the given
    // point, keeping in mind that we may have several runs

    double realPoint = SimulationResults::realPoint(pPoint);

    for (auto data : mDataDataStores.keys()) {
        DataStore::DataStore *dataStore = mDataDataStores.value(data);
        DataStore::DataStoreVariable *voi = dataStore->voi();
        DataStore::DataStoreVariables variables = dataStore->variables();

        for (int i = 0, iMax = variables.count(); i < iMax; ++i)
            data[i] = realValue(realPoint, voi, variables[i]);
    }

    // Now that we are all set, we can add the data to our data store

    mDataStore->addValues(pPoint);
}

//==============================================================================

quint64 SimulationResults::size(int pRun) const
{
    // Return the size of our data store for the given run

    return mDataStore?mDataStore->size(pRun):0;
}

//==============================================================================

DataStore::DataStore * SimulationResults::dataStore() const
{
    // Return our data store

    return mDataStore;
}

//==============================================================================

double * SimulationResults::points(int pRun) const
{
    // Return our points for the given run

    return mPoints?mPoints->values(pRun):nullptr;
}

//==============================================================================

double * SimulationResults::constants(int pIndex, int pRun) const
{
    // Return our constants data at the given index and for the given run

    return mConstants.isEmpty()?nullptr:mConstants[pIndex]->values(pRun);
}

//==============================================================================

double * SimulationResults::rates(int pIndex, int pRun) const
{
    // Return our rates data at the given index and for the given run

    return mRates.isEmpty()?nullptr:mRates[pIndex]->values(pRun);
}

//==============================================================================

double * SimulationResults::states(int pIndex, int pRun) const
{
    // Return our states data at the given index and for the given run

    return mStates.isEmpty()?nullptr:mStates[pIndex]->values(pRun);
}

//==============================================================================

double * SimulationResults::algebraic(int pIndex, int pRun) const
{
    // Return our algebraic data at the given index and for the given run

    return mAlgebraic.isEmpty()?nullptr:mAlgebraic[pIndex]->values(pRun);
}

//==============================================================================

double * SimulationResults::data(double *pData, int pIndex, int pRun) const
{
    // Return our algebraic data at the given index and for the given run

    DataStore::DataStoreVariables data = mData.value(pData);

    return data.isEmpty()?nullptr:data[pIndex]->values(pRun);
}

//==============================================================================

SimulationImportData::SimulationImportData(Simulation *pSimulation) :
    SimulationObject(pSimulation),
    mDataStores(QList<DataStore::DataStore *>())
{
}

//==============================================================================

SimulationImportData::~SimulationImportData()
{
    // Delete some internal objects

    for (auto dataStore : mDataStores)
        delete dataStore;
}

//==============================================================================

DataStore::DataStore * SimulationImportData::addDataStore()
{
    // Add a data store to our list and return it

    DataStore::DataStore *dataStore = new DataStore::DataStore();

    if (dataStore)
        mDataStores << dataStore;

    return dataStore;
}

//==============================================================================

Simulation::Simulation(const QString &pFileName) :
    mFileName(pFileName),
    mRuntime(nullptr),
    mWorker(nullptr)
{
    // Retrieve our file details

    retrieveFileDetails();

    // Create our data and results objects, now that we are all set

    mData = new SimulationData(this);
    mResults = new SimulationResults(this);
    mImportData = new SimulationImportData(this);

    // Keep track of any error occurring in our data

    connect(mData, &SimulationData::error,
            this, &Simulation::error);

    // Keep track of files being managed

    connect(Core::FileManager::instance(), &Core::FileManager::fileManaged,
            this, &Simulation::fileManaged);
}

//==============================================================================

Simulation::~Simulation()
{
    // Stop our worker

    stop();

    // Delete some internal objects

    delete mRuntime;

    delete mImportData;
    delete mResults;
    delete mData;
}

//==============================================================================

void Simulation::retrieveFileDetails(bool pRecreateRuntime)
{
    // Retrieve our CellML and SED-ML files, as well as COMBINE archive

    mCellmlFile = CellMLSupport::CellmlFileManager::instance()->cellmlFile(mFileName);
    mSedmlFile = mCellmlFile?nullptr:SEDMLSupport::SedmlFileManager::instance()->sedmlFile(mFileName);
    mCombineArchive = mSedmlFile?nullptr:COMBINESupport::CombineFileManager::instance()->combineArchive(mFileName);

    // Determine the type of our file

    mFileType = mCellmlFile?CellmlFile:mSedmlFile?SedmlFile:CombineArchive;

    // We have a COMBINE archive, so we need to retrieve its corresponding
    // SED-ML file

    if (mCombineArchive)
        mSedmlFile = mCombineArchive->sedmlFile();

    // We have a SED-ML file (either a direct one or through a COMBINE archive),
    // so we need to retrieve its corresponding CellML file

    if (mSedmlFile)
        mCellmlFile = mSedmlFile->cellmlFile();

    // Get a (new) runtime or update it, if possible
    // Note: updating our runtime is essential when saving a file under a new
    //       name...

    if (pRecreateRuntime) {
        delete mRuntime;

        mRuntime = mCellmlFile?mCellmlFile->runtime(true):nullptr;
    } else {
        if (mCellmlFile)
            mRuntime->update(mCellmlFile, false);
    }
}

//==============================================================================

QString Simulation::fileName() const
{
    // Return our file name

    return mFileName;
}

//==============================================================================

void Simulation::save()
{
    // Retrieve our file details

    bool needReloading = !mRuntime;

    retrieveFileDetails(false);

    // Ask our data and results to update themselves, if needed
    // Note: this is, for example, needed when we open an invalid file (in which
    //       case mRuntime is null), fix it (resulting in a valid mRuntime
    //       value) and then save it...

    if (needReloading) {
        mData->reload();
        mResults->reload();
    }

    // Make sure that our initial values are up to date and check for
    // modifications (which results in people being told that there are no
    // modifications, meaning that a view like the Simulation Experiment view
    // will disable its reset buttons)

    mData->updateInitialValues();
    mData->checkForModifications();
}

//==============================================================================

void Simulation::reload()
{
    // Stop our worker
    // Note: we don't need to delete mWorker since it will be done as part of
    //       its thread being stopped...

    stop();

    // Retrieve our file details

    retrieveFileDetails();

    // Ask our data and results to update themselves

    mData->reload();
    mResults->reload();
}

//==============================================================================

void Simulation::rename(const QString &pFileName)
{
    // Rename ourselves by simply updating our file name

    mFileName = pFileName;
}

//==============================================================================

CellMLSupport::CellmlFileRuntime * Simulation::runtime() const
{
    // Return our runtime

    return mRuntime;
}

//==============================================================================

SimulationWorker * Simulation::worker() const
{
    // Return our worker

    return mWorker;
}

//==============================================================================

Simulation::FileType Simulation::fileType() const
{
    // Return our file type

    return mFileType;
}

//==============================================================================

CellMLSupport::CellmlFile * Simulation::cellmlFile() const
{
    // Return our CellML file object

    return mCellmlFile;
}

//==============================================================================

SEDMLSupport::SedmlFile * Simulation::sedmlFile() const
{
    // Return our SED-ML file object

    return mSedmlFile;
}

//==============================================================================

COMBINESupport::CombineArchive * Simulation::combineArchive() const
{
    // Return our COBMINE archive object

    return mCombineArchive;
}

//==============================================================================

SimulationData * Simulation::data() const
{
    // Return our data

    return mData;
}

//==============================================================================

SimulationResults * Simulation::results() const
{
    // Return our results

    return mResults;
}

//==============================================================================

SimulationImportData * Simulation::importData() const
{
    // Return our imported data

    return mImportData;
}

//==============================================================================

void Simulation::importData(DataStore::DataStoreImportData *pImportData)
{
    // Make sure that we have a runtime

    if (!mRuntime)
        return;

    // Ask our data and results objects to import the given data

    mData->importData(pImportData);
    mResults->importData(pImportData);

    // Ask our runtime to import the given data

    QStringList hierarchy = pImportData->hierarchy();
    double *resultsValues = pImportData->resultsValues();

    for (int i = 0, iMax = pImportData->nbOfVariables(); i < iMax; ++i)
        mRuntime->importData(QString("data_%1").arg(i+1), hierarchy, i, resultsValues);
}

//==============================================================================

int Simulation::runsCount() const
{
    // Return the number of runs held by our results

    return mResults?mResults->runsCount():0;
}

//==============================================================================

quint64 Simulation::runSize(int pRun) const
{
    // Return the size of the data store for the given run

    return mResults?mResults->size(pRun):0;
}

//==============================================================================

bool Simulation::addRun()
{
    // Ask our results to add a run

    return mResults?mResults->addRun():false;
}

//==============================================================================

bool Simulation::isRunning() const
{
    // Return whether we are running

    return mWorker?mWorker->isRunning():false;
}

//==============================================================================

bool Simulation::isPaused() const
{
    // Return whether we are paused

    return mWorker?mWorker->isPaused():false;
}

//==============================================================================

double Simulation::currentPoint() const
{
    // Return our current point

    return mWorker?mWorker->currentPoint():data()->startingPoint();
}

//==============================================================================

const quint64 * Simulation::delay() const
{
    // Return our delay

    return mData->delay();
}

//==============================================================================

void Simulation::setDelay(quint64 pDelay)
{
    // Set our delay

    mData->setDelay(pDelay);
}

//==============================================================================

bool Simulation::simulationSettingsOk(bool pEmitSignal)
{
    // Check and return whether our simulation settings are sound

    if (qFuzzyCompare(mData->startingPoint(), mData->endingPoint())) {
        if (pEmitSignal)
            emit error(tr("the starting and ending points cannot have the same value"));

        return false;
    } else if (mData->startingPoint() > mData->endingPoint()) {
        if (pEmitSignal)
            emit error(tr("the starting point cannot be greater than the ending point"));

        return false;
    } else {
        return true;
    }
}

//==============================================================================

quint64 Simulation::size()
{
    // Return the size of our simulation (i.e. the number of data points that
    // should be generated), if possible

    if (simulationSettingsOk(false))
        return quint64(ceil((mData->endingPoint()-mData->startingPoint())/mData->pointInterval())+1.0);
    else
        return 0;
}

//==============================================================================

void Simulation::run()
{
    // Make sure that we have a runtime

    if (!mRuntime)
        return;

    // Initialise our worker, if we don't already have one and if the
    // simulation settings we were given are sound

    if (!mWorker && simulationSettingsOk()) {
        // Create and move our worker to a thread

        QThread *thread = new QThread();
        mWorker = new SimulationWorker(this, thread, mWorker);

        mWorker->moveToThread(thread);

        connect(thread, &QThread::started,
                mWorker, &SimulationWorker::run);

        connect(mWorker, &SimulationWorker::running,
                this, &Simulation::running);
        connect(mWorker, &SimulationWorker::paused,
                this, &Simulation::paused);

        connect(mWorker, &SimulationWorker::done,
                this, &Simulation::done);
        connect(mWorker, &SimulationWorker::done,
                thread, &QThread::quit);
        connect(mWorker, &SimulationWorker::done,
                mWorker, &SimulationWorker::deleteLater);

        connect(mWorker, &SimulationWorker::error,
                this, &Simulation::error);

        connect(thread, &QThread::finished,
                thread, &QThread::deleteLater);

        // Start our worker by starting the thread in which it is

        thread->start();
    }
}

//==============================================================================

void Simulation::pause()
{
    // Pause our worker

    if (mWorker)
        mWorker->pause();
}

//==============================================================================

void Simulation::resume()
{
    // Resume our worker

    if (mWorker)
        mWorker->resume();
}

//==============================================================================

void Simulation::stop()
{
    // Stop our worker

    if (mWorker)
        mWorker->stop();
}

//==============================================================================

void Simulation::reset(bool pAll)
{
    // Reset our data

    mData->reset(true, pAll);

    // Reset our worker

    if (mWorker)
        mWorker->reset();
}

//==============================================================================

void Simulation::fileManaged(const QString &pFileName)
{
    // A file is being managed, so update our internals by retrieving our file
    // details, if we are that file
    // Note: this will happen when saving a file under a new name, in which case
    //       we need to update our internals...

    if (!pFileName.compare(mFileName))
        retrieveFileDetails(false);
}

//==============================================================================

}   // namespace SimulationSupport
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
