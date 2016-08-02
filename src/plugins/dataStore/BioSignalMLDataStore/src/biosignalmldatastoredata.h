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
// BioSignalML data store data
//==============================================================================

#pragma once

//==============================================================================

#include "datastoreinterface.h"

//==============================================================================

namespace OpenCOR {
namespace BioSignalMLDataStore {

//==============================================================================

class BiosignalmlDataStoreData : public DataStore::DataStoreData
{
public:
    explicit BiosignalmlDataStoreData(const QString &pFileName,
                                      const QString &pName,
                                      const QString &pAuthor,
                                      const QString &pDescription,
                                      const QString &pComment,
                                      const DataStore::DataStoreVariables &pSelectedVariables);

    QString name() const;
    QString author() const;
    QString description() const;
    QString comment() const;

private:
    QString mName;
    QString mAuthor;
    QString mDescription;
    QString mComment;
};

//==============================================================================

}   // namespace BioSignalMLDataStore
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
