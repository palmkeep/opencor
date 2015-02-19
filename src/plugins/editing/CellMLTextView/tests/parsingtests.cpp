/*******************************************************************************

Licensed to the OpenCOR team under one or more contributor license agreements.
See the NOTICE.txt file distributed with this work for additional information
regarding copyright ownership. The OpenCOR team licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this file
except in compliance with the License. You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.

*******************************************************************************/

//==============================================================================
// CellML Text view parsing tests
//==============================================================================

#include "cellmltextviewparser.h"
#include "parsingtests.h"

//==============================================================================

#include <QtTest/QtTest>

//==============================================================================

void ParsingTests::parsingTests()
{
    OpenCOR::CellMLTextView::CellmlTextViewParser parser;

    QVERIFY(!parser.execute(QString()));
    QVERIFY(parser.hasError());
    QCOMPARE(parser.messages().first().message(), QString("'def' is expected, but the end of the file was found instead."));

    QVERIFY(!parser.execute(QString("def")));
    QVERIFY(parser.hasError());
    QCOMPARE(parser.messages().first().message(), QString("'model' is expected, but the end of the file was found instead."));
}

//==============================================================================

QTEST_GUILESS_MAIN(ParsingTests)

//==============================================================================
// End of file
//==============================================================================
